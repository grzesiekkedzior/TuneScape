QT += testlib
QT += widgets
QT += network
QT += multimedia
QT += concurrent
QT += gui
CONFIG += qt warn_on depend_includepath testcase
CONFIG += no_testcase_installs
include(../common.pri)
TEMPLATE = app

SOURCES +=  tst_tunescapetests.cpp

DISTFILES += \
    application.properties
