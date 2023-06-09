#-------------------------------------------------
#
# Project created by QtCreator 2013-09-05T10:20:57
#
#-------------------------------------------------

QT       -= core gui

TARGET = cmn
TEMPLATE = lib

DEFINES += LIBCMN_LIBRARY

SOURCES += \
    src/PortBases.cpp \
    src/ProtocolBase.cpp \
    src/Channel.cpp \
    src/PortUdp.cpp \
    src/Communication.cpp \
    src/ChannelConfig.cpp \
    src/PortTcp.cpp \
    src/CmnInterface.cpp \
    src/PortTcpServer.cpp \
    src/Devices.cpp \
    src/ChannelEngine.cpp \
    src/GlobalCfg.cpp \
    src/PortTcpClient.cpp \
    src/PortCom.cpp \
    src/Device.cpp \
    src/PollingEngine.cpp \
    src/Protocol.cpp \
    src/DispatchEngine.cpp \
    src/CmnRtInterface.cpp \
    src/PortBase.cpp \
    src/PortTask.cpp \
    src/DataCache.cpp

HEADERS += \
    include/GlobalCfg.h \
    include/PortTcpClient.h \
    include/PollingEngine.h \
    include/ChannelConfig.h \
    include/Device.h \
    include/Channel.h \
    include/ChannelEngine.h \
    include/PortBases.h \
    include/Communication.h \
    include/PortTcp.h \
    include/Devices.h \
    include/PortUdp.h \
    include/PortBase.h \
    include/CmnInterface.h \
    include/DispatchEngine.h \
    include/ProtocolBase.h \
    include/PortTask.h \
    include/PortTcpServer.h \
    include/CmnRtInterface.h \
    include/PortCom.h \
    include/Protocol.h \
    include/DataCache.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE22ACF87
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = libcmn.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}

INCLUDEPATH += $$quote(include) \
               $$quote(../libcore/include) \
               $$quote(../libxml/include)  \
               $$quote(../librtbase/include)



win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../libcore-build/ -llibcore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../libcore-build/ -llibcore
else:unix: LIBS += -L$$PWD/../libcore-build/ -llibcore

INCLUDEPATH += $$PWD/../libcore-build
DEPENDPATH += $$PWD/../libcore-build

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../libxml-build/ -llibxml
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../libxml-build/ -llibxml
else:unix: LIBS += -L$$PWD/../libxml-build/ -llibxml

INCLUDEPATH += $$PWD/../libxml-build
DEPENDPATH += $$PWD/../libxml-build

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../librtbase-build/ -llibrtbase
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../librtbase-build/ -llibrtbase
else:unix: LIBS += -L$$PWD/../librtbase-build/ -llibrtbase

INCLUDEPATH += $$PWD/../librtbase-build
DEPENDPATH += $$PWD/../librtbase-build


