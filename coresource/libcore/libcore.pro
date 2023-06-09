#-------------------------------------------------
#
# Project created by QtCreator 2013-09-05T09:26:59
#
#-------------------------------------------------

QT       -= core gui

TARGET = core
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    src/datatype.cpp \
    src/sysmalloc.cpp \
    src/syslogger.cpp \
    src/sysmutex.cpp \
    src/hashtable.cpp \
    src/sysstring.cpp \
    src/systhread.cpp

HEADERS += \
    include/sysstring.h \
    include/linkedlist.h \
    include/syslogger.h \
    include/systhread.h \
    include/hashtable.h \
    include/datatype.h \
    include/sysmutex.h \
    include/sysmalloc.h \
    include/sysgenlist.h \
    include/sysqueue.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}


INCLUDEPATH += $$quote(include) \
