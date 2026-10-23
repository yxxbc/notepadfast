#include "ccnotepad.h"
#include "nddsetting.h"
#include "styleset.h"


#include <QtWidgets/QApplication>
#include <QTextCodec>
#include <QMessageBox>
#include <QSharedMemory>
#include <QFile>
#include <QStatusBar>
#include <qobject.h>
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QTimer>

#ifdef Q_OS_UNIX
#include <QStyleFactory>
#include <signal.h>
#include <unistd.h>
#include <QDebug>
#include <QWidget>
#endif

#ifdef Q_OS_WIN
#pragma comment(lib, "user32.lib")
#if _DEBUG
#pragma comment(lib, "qmyedit_qt5d.lib")
#else
#pragma comment(lib, "qmyedit_qt5.lib")
#endif
#include <qt_windows.h>
const ULONG_PTR CUSTOM_TYPE = 10000;
const ULONG_PTR OPEN_NOTEPAD_TYPE = 10001;
const ULONG_PTR CUSTOM_TYPE_FILE_LINENUM = 10002;
bool s_isAdminAuth = false;
#endif

const QString c_strTitle = "Ndd";


#ifdef Q_OS_UNIX
#if defined(Q_OS_MAC)
QSharedMemory shared("CCNotebook123");;//mac下面后面带一个版本号，避免新的打不开
#else
QSharedMemory shared("CCNotebook");
#endif
QSharedMemory nddShared("notepadfast");

static void sig_usr(int signo)
{
    if(signo  == SIGUSR1)
    {
        qlonglong winId;
        shared.lock();
        memcpy(&winId,shared.data(),sizeof(qlonglong));
        shared.unlock();

         QWidget *pMain = QWidget::find((WId)winId);
         CCNotePad* pNotePad = dynamic_cast<CCNotePad*>(pMain);
         if(pNotePad != nullptr)
         {
             QString filePath((char*)nddShared.data()+4);
             if(!filePath.isEmpty())
             {
             pNotePad->openFile(filePath);
             }
             pNotePad->activateWindow();
             pNotePad->showNormal();
             qDebug() << "sig_usr" << filePath;
         }
    }
}

#if !defined(Q_OS_MAC)
//下面就是单纯的linux 系统。

//https://github.com/yxxbc/notepadfast/pulls/110/files
//共享内存可能是上次异常退出残留的，其中的 pid 可能为 0 或已被其他进程复用。
//kill(0, SIGUSR1) 会把信号发给整个进程组（SIGUSR1 默认动作是终止），
//从桌面启动时会连同会话进程一起杀掉，导致用户被注销。发信号前必须校验。
static bool isRunningNddProcess(pid_t pid)
{
    if (pid <= 1)
    {
        return false;
    }

	// 比较 exe 路径（最可靠）
	QFileInfo selfExe("/proc/self/exe");
	QFileInfo otherExe(QString("/proc/%1/exe").arg(pid));

	if (selfExe.exists() && otherExe.exists()) {
		return selfExe.symLinkTarget() == otherExe.symLinkTarget();
	}

    QFile selfComm("/proc/self/comm");
    if (!selfComm.open(QIODevice::ReadOnly))
    {
        //没有 /proc 时无法进一步校验，只依赖 pid 的合法性判断
        return true;
    }

    QFile otherComm(QString("/proc/%1/comm").arg(pid));
    if (!otherComm.open(QIODevice::ReadOnly))
    {
        return false;
    }
	return selfComm.readAll().trimmed() == otherComm.readAll().trimmed();
}
#endif
#endif


#ifdef Q_OS_MAC
#include <libproc.h>

//pid 是否真的是"本程序的另一个实例"：进程不存在，或 pid 已被别的进程复用，都算残留。
static bool isOurProcess(pid_t pid)
{
	if (pid <= 0 || 0 != kill(pid, 0))
	{
		return false;
	}

	char pathBuf[PROC_PIDPATHINFO_MAXSIZE] = {0};
	if (proc_pidpath(pid, pathBuf, sizeof(pathBuf)) <= 0)
	{
		return false;
	}

	const QString other = QString::fromLocal8Bit(pathBuf);
	const QString self = QCoreApplication::applicationFilePath();
	return QFileInfo(other).fileName() == QFileInfo(self).fileName();
}

//布局（create 32 字节）：[0..8) pid，[16..24) 最近心跳时间(ms)
static const int kHeartbeatOffset = 16;

static qint64 readHeartbeat()
{
	qint64 ts = 0;
	nddShared.lock();
	if (nddShared.data() != nullptr && nddShared.size() >= kHeartbeatOffset + (int)sizeof(qint64))
	{
		memcpy(&ts, (char*)nddShared.data() + kHeartbeatOffset, sizeof(qint64));
	}
	nddShared.unlock();
	return ts;
}

static void writeHeartbeat()
{
	if (!nddShared.isAttached())
	{
		return;
	}
	nddShared.lock();
	if (nddShared.data() != nullptr && nddShared.size() >= kHeartbeatOffset + (int)sizeof(qint64))
	{
		const qint64 ts = QDateTime::currentMSecsSinceEpoch();
		memcpy((char*)nddShared.data() + kHeartbeatOffset, &ts, sizeof(qint64));
	}
	nddShared.unlock();
}
#endif

#ifdef Q_OS_MAC
#include <QFileOpenEvent>

//主窗口，创建前为 nullptr。Finder 双击关联文件时，FileOpen 事件可能在窗口
//创建之前就到达（冷启动），此时只能先记下路径，等窗口建好再打开。
static CCNotePad* g_pMainNotepad = nullptr;

static void openfile(const QString& filePath)
{
    if (g_pMainNotepad == nullptr)
    {
        return;
    }

    if (!filePath.isEmpty())
    {
        g_pMainNotepad->openFile(filePath);
    }
    //从 Finder 唤起时窗口可能被压在其它应用后面
    g_pMainNotepad->raise();
    g_pMainNotepad->activateWindow();
}

class MyApplication : public QApplication
{
   public:
       MyApplication(int &argc, char **argv)
           : QApplication(argc, argv)
       {
       }

       bool event(QEvent *event)
       {
           if (event->type() == QEvent::FileOpen) {
               QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
               qDebug() << "Open file" << openEvent->file();
               if (g_pMainNotepad != nullptr)
               {
                   openfile(openEvent->file());
               }
               else
               {
                   //冷启动：窗口还没建好，先攒着，main() 里补开
                   s_openfile = openEvent->file();
               }
           }

           return QApplication::event(event);
       }
       QString s_openfile;
   };
#endif

int main(int argc, char *argv[])
{
	QElapsedTimer s_startupTimer;
	s_startupTimer.start();

#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
	QApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
	#ifdef Q_OS_WIN
		HDC hdc = CreateDC(L"display", NULL, NULL, NULL);
		int ndpi = GetDeviceCaps(hdc, LOGPIXELSY);
		qputenv("QT_SCALE_FACTOR", QString::number(ndpi / 96.0).toUtf8());
	#endif // Q_OS_WIN
#endif
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

#ifdef Q_OS_MAC
    MyApplication a(argc, argv);
#else
	QApplication a(argc, argv);
#endif

#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
	a.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

	//不能开启，开启后相对路径打开文件失败
	//QDir::setCurrent(QCoreApplication::applicationDirPath());

#if defined(Q_OS_UNIX)
    QApplication::setStyle(QStyleFactory::create("fusion"));
#endif
	a.setApplicationDisplayName(c_strTitle);
	a.setApplicationName(c_strTitle);
	if (qEnvironmentVariableIsSet("NDD_BENCH_STARTUP")) { qInfo() << "bench_app_ms:" << s_startupTimer.elapsed(); }

	QStringList arguments = QCoreApplication::arguments();

	//目前就三种
	//1) ndd filepath
	//2) ndd filepath -n linenum
	//3) ndd -multi filepath
	//只有 1  2 需要处理短路径
	if ((arguments.size() == 2) || (arguments.size() == 4))
	{
		//鸿蒙/linux 下，currentPath 可能为根目录 "/"，空参数拼接会得到 "//"，
		//导致把目录当文件打开。空参数本身无意义，直接跳过不处理。
		if (!arguments[1].isEmpty())
		{
		QFileInfo fi(arguments[1]);
		if (fi.isRelative())
		{
			QString absDir = QDir::currentPath();
			//获取绝对路径
			arguments[1] = QString("%1/%2").arg(absDir).arg(arguments.at(1));
		}
	}
	}

#ifdef uos
	QFont font("Noto Sans CJK SC,9,-1,5,50,0,0,0,0,0,Regular", 9);
	QApplication::setFont(font);
#endif
#ifdef Q_OS_MAC
	//界面字体：设置里保存过就用保存的，否则用系统等宽 Menlo。
	//字体大小务必要和查找结果框的高度匹配，否则结构会拥挤。
	QFont font("Menlo", 11);
	// qDebug() << "font name mac";
	QApplication::setFont(font);
	// qDebug() << QApplication::font().toString();
#endif

bool isGotoLine = false;

#ifdef Q_OS_WIN
	QSharedMemory shared("ccnotepad");

	if (arguments.size() > 2)
	{
		//如果是多开请求，这种是从管理员权限申请后重开过来的
		if (arguments[1] == QString("-muti"))
		{
			s_isAdminAuth = true;

			QString title = QString(u8"%1 管理员").arg(c_strTitle);
			a.setApplicationDisplayName(title);
			//删除-muti这个参数
			arguments.removeAt(1);

			//管理员不占用共享标志。这样的目的是，当管理员窗口存在时
			//打开原来的文件，原来的文件可以占用共享标志，作为主窗口打开。
			//管理员窗口永远不做主窗口打开
			goto authAdmin;

		}
		else if ((arguments.size() == 4) && arguments[2] == QString("-n"))
		{
			//使用的是 file -n lineNums 方式。目前只有windows下支持 xxxfile -n linenum的格式
			isGotoLine = true;
	}

	}
#else
if ((arguments.size() == 4) && (arguments[2] == QString("-n")))
{
      //使用的是 file -n lineNums 方式。目前只有windows下支持 xxxfile -n linenum的格式
      isGotoLine = true;
}
#endif

	//attach成功表示已经存在该内存了，表示当前存在实例
	if (shared.attach())//共享内存被占用则直接返回
	{
		//发现在文件中如果存在空格时，参数不止1个，所以不能单纯用2个参数表示
		if (arguments.size() > 1)
		{
        #if defined(Q_OS_WIN)
			int tryTimes = 0;
			do {
			qlonglong hwndId;
			shared.lock();
			memcpy(&hwndId, shared.data(), sizeof(qlonglong));
			shared.unlock();

			HWND hwnd = (HWND)hwndId;

			if (::IsWindow(hwnd))
			{
					if (!isGotoLine)
					{
						//就是ndd filepath的命令行格式
				//去掉第一个参数，后续的参数拼接起来。其实参数中间有空格还是需要使用""引用起来，避免空格参数分隔为多个
				arguments.takeFirst();

				QString filename = arguments.join("");
				QByteArray data = filename.toUtf8();

				COPYDATASTRUCT copydata;
				copydata.dwData = CUSTOM_TYPE; //自定义类型
				copydata.lpData = data.data();  //数据大小
				copydata.cbData = data.size();  // 指向数据的指针

				::SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&copydata));
			}
			else
			{
						//是 filepath -n linenums 方式。不考虑filepath含有空格的情况，因为前面做了严格判断

						QString para = QString("%1|%2").arg(arguments[1]).arg(arguments[3]);
						QByteArray data = para.toUtf8();

						COPYDATASTRUCT copydata;
						copydata.dwData = CUSTOM_TYPE_FILE_LINENUM; //自定义类型
						copydata.lpData = data.data();  //数据大小
						copydata.cbData = data.size();  // 指向数据的指针

						::SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&copydata));
					}

					break;
				}
				else
				{

					//20230304 右键多个文件同时打开，比如3个。此时只有第1个可获取锁，其余2个均走这里。
					//因为第个还没有来的及写入hwnd。此时不要goto drop_old。等一下再重试
					QThread::sleep(1); 
					++tryTimes;

					//2次识别后，没法了，只能通过继续往下走。
				//失败了，此时说明前一个窗口极可能状态错误了。如果不处理，则再也打不开程序了
					if (tryTimes > 2)
					{
				goto drop_old;
			}

				}
			} while (true);

        #elif defined (Q_OS_MAC)
        {
               //mac下面不需要，有他自身的机制保证
        }
        #else
            pid_t pid;

            arguments.takeFirst();
            QString filename = arguments.join("");
            QByteArray data = filename.toUtf8();

            if (!nddShared.attach())
            {
				shared.detach();
                goto unix_goon;
            }
            nddShared.lock();
            memcpy(&pid, nddShared.data(), sizeof(pid_t));
            if (!isRunningNddProcess(pid))
            {
                nddShared.unlock();
				nddShared.detach();//必须detach一下，让之前崩溃的被释放掉。后面会重新create内存。
				shared.detach();
                goto unix_goon;
            }
            memset((char*)nddShared.data()+sizeof(pid_t),0, 1024-sizeof(pid_t));
            memcpy((char*)nddShared.data()+sizeof(pid_t),data.data(),data.size());
            nddShared.unlock();

            //if kill failed, then open a new process
            if(0 != kill(pid,SIGUSR1))
            {
				//之前的进程很可能已经崩溃掉了。但是共享内存不会被系统释放。
				nddShared.detach();//必须detach一下，让之前崩溃的被释放掉。后面会重新create内存。
				shared.detach();
                goto unix_goon;
            }
        #endif
		}
		else if (arguments.size() == 1)
		{
#if defined(Q_OS_WIN)
			//把窗口设置到最前
			qlonglong hwndId;
			shared.lock();
			memcpy(&hwndId, shared.data(), sizeof(qlonglong));
			shared.unlock();
			HWND hwnd = (HWND)hwndId;
			if (::IsWindow(hwnd))
			{
				QString filename("open");
				QByteArray data = filename.toUtf8();

				COPYDATASTRUCT copydata;
				copydata.dwData = OPEN_NOTEPAD_TYPE; //自定义类型
				copydata.lpData = data.data();  //数据大小
				copydata.cbData = data.size();  // 指向数据的指针

				::SendMessage(hwnd, WM_COPYDATA, reinterpret_cast<WPARAM>(nullptr), reinterpret_cast<LPARAM>(&copydata));
			}
			else
			{
				//失败了，此时说明前一个窗口极可能状态错误了。如果不处理，则再也打不开程序了
				//继续新开一个窗口，放弃之前的旧内容
				goto drop_old;
			}
#elif defined (Q_OS_MAC)
{
		//mac 下 QSharedMemory 是 POSIX 共享内存段，进程被强杀/崩溃时不会 unlink，
		//下次启动 attach 仍会成功。若直接 return 0，表现就是"点了没反应、窗口不出现"。
		//所以先校验里面记的 pid 是否真的还活着。
		pid_t pid = 0;
		if (!nddShared.attach())
		{
			shared.detach();
			goto mac_goon;
		}
		nddShared.lock();
		memcpy(&pid, nddShared.data(), sizeof(pid_t));
		nddShared.unlock();

		const bool stale = !isOurProcess(pid);
		if (stale)
		{
			nddShared.detach();
			shared.detach();
			goto mac_goon;
		}
}
#else
            pid_t pid;
            if (!nddShared.attach())
            {
				shared.detach();
                goto unix_goon;
            }
            nddShared.lock();
            memcpy(&pid, nddShared.data(), sizeof(pid_t));
            if (!isRunningNddProcess(pid))
            {
                nddShared.unlock();
				nddShared.detach();//必须detach一下，让之前崩溃的被释放掉。后面会重新create内存。
				shared.detach();
                goto unix_goon;
            }
            memset((char*)nddShared.data()+sizeof(pid_t),0, 1024-sizeof(pid_t));
            nddShared.unlock();
            qDebug()<<"empty file send";

            if(0 != kill(pid,SIGUSR1))
            {
				nddShared.detach();//必须detach一下，让之前崩溃的被释放掉。后面会重新create内存。
				shared.detach();
                goto unix_goon;
            }
#endif
		}
        NddSetting::close();
		return 0;
	}
#if defined(Q_OS_WIN)
	shared.create(32);
#elif defined (Q_OS_MAC)
//注意：label 不能写在 {} 里面，否则上面的 goto 是跳进块作用域（UB，实测直接崩）
mac_goon:
	shared.create(32);
	nddShared.create(32);
#else

unix_goon:
    shared.create(32);
    nddShared.create(2048);

    if(signal(SIGUSR1,sig_usr) == SIG_ERR)
    {
       qDebug()<<"linux create sign failed";
    }
#endif

#if defined(Q_OS_WIN)
authAdmin:
drop_old:
#endif

	//20221009发现有小概率出现窗口没有，但是进程还在的诡异问题，加个保护一下
	QApplication::setQuitOnLastWindowClosed(true);

	NddSetting::init();

	if (qEnvironmentVariableIsSet("NDD_BENCH_STARTUP")) { qInfo() << "bench_setting_ms:" << s_startupTimer.elapsed(); }

#ifdef Q_OS_MAC
	//界面字体：设置里存过就套用（必须在 NddSetting::init() 之后，否则 settings 还没建）
	{
		const QString savedFont = NddSetting::getKeyValueFromSets(APP_FONT);
		if (!savedFont.isEmpty())
		{
			QFont f = QApplication::font();
			if (f.fromString(savedFont))
			{
				QApplication::setFont(f);
			}
		}
		if (qEnvironmentVariableIsSet("NDD_BENCH_STARTUP")) { qInfo() << "app_font:" << QApplication::font().toString(); }
	}
#endif
	int id = NddSetting::getKeyValueFromNumSets(SKIN_KEY);
	StyleSet::setSkin(id);
	if (qEnvironmentVariableIsSet("NDD_BENCH_STARTUP")) { qInfo() << "bench_style_ms:" << s_startupTimer.elapsed(); }

	CCNotePad *pMainNotepad = new CCNotePad(true);
	pMainNotepad->setAttribute(Qt::WA_DeleteOnClose);
	pMainNotepad->setShareMem(&shared);
	pMainNotepad->quickshow();

	pMainNotepad->syncCurSkinToMenu(id);
	if (qEnvironmentVariableIsSet("NDD_BENCH_STARTUP")) { qInfo() << "bench_window_ms:" << s_startupTimer.elapsed(); }


#ifdef Q_OS_WIN
	//HWND hwnd = ::FindWindowA("Qt5QWindowIcon", "CCNotebook");
	//发现hwnd就是和effectiveWinId相等的，不需要查询了
	//管理员可以多开，暂时不把管理员的权限作为主窗口，因为其他用户没有权限右键菜单发送消息给管理员窗口去打开文件
	if (!s_isAdminAuth)
	{
	qlonglong winId = (qlonglong)pMainNotepad->effectiveWinId();
	shared.lock();
	memcpy(shared.data(), &winId, sizeof(qlonglong));
	shared.unlock();
	}
#else
    qlonglong winId = (qlonglong)pMainNotepad->effectiveWinId();
    shared.lock();
    memcpy(shared.data(), &winId, sizeof(qlonglong));
    shared.unlock();
    nddShared.attach();
    //get proceess id to share memory
    pid_t pid = getpid();
    nddShared.lock();
    memcpy(nddShared.data(), &pid, sizeof(pid_t));
    nddShared.unlock();
#endif // Q_OS_WIN
	//恢复上次关闭时的文件
#ifdef Q_OS_WIN
	if (!s_isAdminAuth)
	{
		if (0 == pMainNotepad->restoreLastFiles() && (arguments.size() == 1))
		{
		pMainNotepad->initTabNewOne();
	}
	}
#else
    if (0 == pMainNotepad->restoreLastFiles())
    {
    pMainNotepad->initTabNewOne();
    }
#endif
	if (arguments.size() == 2)
	{
#ifdef Q_OS_WIN
		if (!s_isAdminAuth)
		{
			pMainNotepad->openFile(arguments[1]);
		}
		else
		{
			//如果是管理员，还不能直接打开文件，需要恢复之前文件的修改内容
			//恢复不了，再直接打开
			pMainNotepad->tryRestoreFile(arguments[1]);
		}
#else
		pMainNotepad->openFile(arguments[1]);
#endif
	}
	else if (isGotoLine)
	{
		//是filepath -n xxx 格式。
		bool ok = true;
		int lineNum = arguments[3].toInt(&ok);
		if (!ok)
		{
			lineNum = -1;
		}
		pMainNotepad->openFile(arguments[1], lineNum);
	}
#ifdef Q_OS_WIN
	pMainNotepad->checkAppFont();
#endif

#ifdef Q_OS_MAC
	g_pMainNotepad = pMainNotepad;
	//Finder 双击关联文件时的冷启动路径：事件早于窗口创建，这里补开
	if (!a.s_openfile.isEmpty())
	{
		pMainNotepad->openFile(a.s_openfile);
		a.s_openfile.clear();
		pMainNotepad->raise();
		pMainNotepad->activateWindow();
	}
	if (qEnvironmentVariableIsSet("NDD_BENCH_STARTUP"))
	{
		qInfo() << "startup_ms:" << s_startupTimer.elapsed();
		QTimer::singleShot(0, qApp, &QCoreApplication::quit);
	}
#endif

	a.exec();

	NddSetting::close();

	return 0;
}
