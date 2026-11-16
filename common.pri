QT += core gui widgets network multimedia svg concurrent
CONFIG += c++17

TUNESCAPE_CORE = $$PWD/TuneScapeCore
INCLUDEPATH += $$quote($$TUNESCAPE_CORE)

TUNESCAPE_MANIFEST = $$quote($$TUNESCAPE_CORE/sources.list)
DISTFILES += $$TUNESCAPE_MANIFEST
TUNESCAPE_FILES = $$cat($$TUNESCAPE_MANIFEST, lines)

for(entry, TUNESCAPE_FILES) {
    projectFile = $$quote($$TUNESCAPE_CORE/$$entry)
    contains(entry, .*[.]cpp): SOURCES += $$projectFile
    contains(entry, .*[.]h): HEADERS += $$projectFile
    contains(entry, .*[.]ui): FORMS += $$projectFile
    contains(entry, .*[.]qrc): RESOURCES += $$projectFile
}
