#-------------------------------------------------
#
# Project created by QtCreator 2013-09-05T10:01:30
#
#-------------------------------------------------

QT       -= core gui

TARGET = xml
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    src/tinyxmlparser.cpp \
    src/tinyxml.cpp \
    src/tinystr.cpp \
    src/tinyxmlerror.cpp

HEADERS += \
    include/tinyxml.h \
    include/tinystr.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

INCLUDEPATH += $$quote(include) \
