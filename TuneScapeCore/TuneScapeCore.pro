QT       += core gui network multimedia concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/AppConfig.cpp \
    src/Country.cpp \
    src/IceCastXmlData.cpp \
    src/Menu.cpp \
    src/StreamRecorder.cpp \
    src/Theme.cpp \
    src/TrayIcon.cpp \
    src/audioprocessor.cpp \
    src/customcolordelegate.cpp \
    src/fft.cpp \
    src/flowlayout.cpp \
    src/miniplayer.cpp \
    src/radioinfo.cpp \
    src/streamReader.cpp \
    src/jsonlistprocessor.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/radioaudiomanager.cpp \
    src/radioexplorer.cpp \
    src/radiolist.cpp \
    src/radiostations.cpp

HEADERS += \
    include/AppConfig.h \
    include/Country.h \
    include/IceCastXmlData.h \
    include/Menu.h \
    include/RadioBrowserApi.h \
    include/StreamRecorder.h \
    include/Theme.h \
    include/TrayIcon.h \
    include/audioprocessor.h \
    include/container.h \
    include/customcolordelegate.h \
    include/fft.h \
    include/flowlayout.h \
    include/miniplayer.h \
    include/radioinfo.h \
    include/streamReader.h \
    include/jsonlistprocessor.h \
    include/mainwindow.h \
    include/radioaudiomanager.h \
    include/radioexplorer.h \
    include/radiolist.h \
    include/radiostations.h


FORMS += \
    mainwindow.ui \
    mini_player.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES +=
