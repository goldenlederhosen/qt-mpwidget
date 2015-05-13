QMAKE_CXXFLAGS_DEBUG += -O0 -DCAUTION
QMAKE_LFLAGS_DEBUG += -O0  -DCAUTION
QMAKE_CXXFLAGS += -std=c++0x -W -Wall -Woverloaded-virtual

HEADERS       = \
    mainwindow.h \
    util.h \
    mpmediainfo.h \
    checkedget.h \
    mpplainvideowidget.h \
    mpprocess.h \
    mpstate.h \
    mpwidget.h \
    cropdetector.h \
    gui_overlayquit.h \
    singleqprocesssingleshot.h \
    singleqprocess.h \
    screensavermanager.h \
    qprocess_meta.h \
    capslock.h \
    dbus.h \
    xsetscreensaver.h \
    vregexp.h \
    keepfocus.h \
    focusmanager.h \
    asynckillproc.h \
    asyncreadfile.h \
    asyncreadfile_child.h \
    asynckillproc_p.h
SOURCES       = \
    mainwindow.cpp \
    util.cpp \
    mpmediainfo.cpp \
    mpplainvideowidget.cpp \
    mpprocess.cpp \
    mpstate.cpp \
    mpwidget.cpp \
    singleplayer-main.cpp \
    cropdetector.cpp \
    gui_overlayquit.cpp \
    singleqprocesssingleshot.cpp \
    singleqprocess.cpp \
    screensavermanager.cpp \
    capslock.cpp \
    dbus.cpp \
    xsetscreensaver.cpp \
    keepfocus.cpp \
    focusmanager.cpp \
    asynckillproc.cpp \
    asyncreadfile.cpp \
    asyncreadfile_child.cpp \
    asynckillproc_p.cpp

QT+=svg dbus
DEFINES += QT_NO_CAST_FROM_ASCII QT_USE_QSTRINGBUILDER QT_USE_FAST_CONCATENATION QT_USE_FAST_OPERATOR_PLUS QT_NO_CAST_FROM_BYTEARRAY
LIBS+=-lX11

# install
target.path = $$[QT_INSTALL_EXAMPLES]/widgets/singleplayer
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS singleplayer.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/widgets/singleplayer
INSTALLS += target sources

symbian: include($$QT_SOURCE_TREE/examples/symbianpkgrules.pri)

wince*: {
   DEPLOYMENT_PLUGIN += qjpeg qmng qgif
}

RESOURCES += \
    application.qrc

