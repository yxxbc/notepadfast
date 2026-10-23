TEMPLATE = app
LANGUAGE = C++

TARGET = NotepadFast

CONFIG += qt warn_on release

QT += core gui widgets concurrent network

HEADERS += *.h \
           cceditor/ccnotepad.h \
           cceditor/filemanager.h

SOURCES += *.cpp *.cc \
           cceditor/ccnotepad.cpp \
           cceditor/filemanager.cpp

FORMS += *.ui \
         cceditor/ccnotepad.ui

RESOURCES += RealCompare.qrc

INCLUDEPATH += $$PWD/../third_party/qscint/src
INCLUDEPATH += $$PWD/../third_party/qscint/src/Qsci
INCLUDEPATH += $$PWD/../third_party/qscint/scintilla/include
INCLUDEPATH += cceditor

DEFINES += QSCINTILLA_DLL

TRANSLATIONS += realcompare_zh.ts

# 只维护 macOS (Apple Silicon / arm64)。
# 先编译 QScintilla 静态库：cd third_party/qscint/src && qmake qscintilla.pro && make
DESTDIR = $$PWD/../build/bin
LIBS += -L$$PWD/../build/lib -lqmyedit_qt5
PRE_TARGETDEPS += $$PWD/../build/lib/libqmyedit_qt5.a

ICON = mac.icns

macx {
    QMAKE_APPLE_DEVICE_ARCHS = arm64
}
