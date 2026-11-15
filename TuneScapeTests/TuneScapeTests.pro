QT += testlib
QT += widgets
QT += network
QT += multimedia
QT += gui
QT += svg
QT += svgwidgets
CONFIG += qt warn_on depend_includepath testcase c++17
CONFIG += no_testcase_installs
include(../common.pri)
TEMPLATE = app

SOURCES +=  tst_tunescapetests.cpp

DISTFILES += \
    application.properties
