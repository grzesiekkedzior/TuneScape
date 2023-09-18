QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/jsonlistprocessor.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/radioaudiomanager.cpp \
    src/radioexplorer.cpp \
    src/radiolist.cpp \
    src/radiostations.cpp

HEADERS += \
    include/jsonlistprocessor.h \
    include/mainwindow.h \
    include/radioaudiomanager.h \
    include/radioexplorer.h \
    include/radiolist.h \
    include/radiostations.h


FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    res.qrc

DISTFILES +=
