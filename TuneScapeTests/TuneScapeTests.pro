QT += testlib
QT += widgets
QT += network
QT += multimedia
QT += concurrent
QT += gui
CONFIG += qt warn_on depend_includepath testcase
include(../common.pri)
TEMPLATE = app

SOURCES +=  tst_tunescapetests.cpp
