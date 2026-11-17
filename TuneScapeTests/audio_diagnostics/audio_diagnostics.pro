QT += core gui network multimedia testlib
CONFIG += console c++17 testcase no_testcase_installs
TEMPLATE = app
TARGET = TuneScapeAudioDiagnostics
SOURCES += main.cpp
win32: LIBS += -lpsapi
