#-------------------------------------------------
#
# Project created by QtCreator 2013-09-05T10:11:20
#
#-------------------------------------------------

QT       -= core gui

TARGET = rtbase
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    src/rtobject.cpp \
    src/rtbase.cpp \
    src/rtobjecttree.cpp

HEADERS += \
    include/rtobject.h \
    include/rtobjecttree.h \
    include/rtbase.h
unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

INCLUDEPATH += $$quote(include) \
               $$quote(../libcore/include)
