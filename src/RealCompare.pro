TEMPLATE = app
LANGUAGE = C++

TARGET = NotepadFast

CONFIG += qt warn_on release

QT += core gui widgets concurrent network

# QScintilla 的 MacPasteboardMime 依赖 QtMacExtras（仅 Qt5）
macx: QT += macextras

HEADERS += *.h \
           cceditor/ccnotepad.h \
           cceditor/filemanager.h

SOURCES += *.cpp \
           cceditor/ccnotepad.cpp \
           cceditor/filemanager.cpp

FORMS += *.ui \
         cceditor/ccnotepad.ui

RESOURCES += RealCompare.qrc

INCLUDEPATH += $$PWD/../third_party/qscint/src
INCLUDEPATH += $$PWD/../third_party/qscint/src/Qsci
INCLUDEPATH += $$PWD/../third_party/qscint/scintilla/include
INCLUDEPATH += cceditor

# 生成物统一扔进 build/，否则 moc_*.cpp / ui_*.h 会落在 src/，
# 再被上面 SOURCES += *.cpp 的 wildcard 收进来 → duplicate symbol
MOC_DIR     = $$PWD/../build/moc
OBJECTS_DIR = $$PWD/../build/obj
UI_DIR      = $$PWD/../build/ui
RCC_DIR     = $$PWD/../build/rcc

DEFINES += QSCINTILLA_DLL

TRANSLATIONS += realcompare_zh.ts

# 只维护 macOS (Apple Silicon / arm64)。
# 先编译 QScintilla 静态库：cd third_party/qscint/src && qmake qscintilla.pro && make
DESTDIR = $$PWD/../build/bin
LIBS += -L$$PWD/../build/lib -lqmyedit_qt5
PRE_TARGETDEPS += $$PWD/../build/lib/libqmyedit_qt5.a

# Markdown 渲染：github/cmark-gfm（brew install cmark-gfm）
macx {
    #注意：不能用 /opt/homebrew 前缀，那里有 qtbase(Qt6) 的头文件，
    #-I 进去会让 <QtCore/QtCore> 命中 Qt6 的伞头文件，和 Qt5 打架
    CMARK_PREFIX = /opt/homebrew/opt/cmark-gfm
    INCLUDEPATH += $$CMARK_PREFIX/include
    LIBS += $$CMARK_PREFIX/lib/libcmark-gfm-extensions.a $$CMARK_PREFIX/lib/libcmark-gfm.a
}

ICON = mac.icns

macx {
    QMAKE_APPLE_DEVICE_ARCHS = arm64
}
