INCLUDEPATH += $$PWD/
INCLUDEPATH += $$PWD/SignalCore

HEADERS += \
    $$PWD/SignalCore/RWnd_DefEchoSaveConfig.h \
    $$PWD/SignalCore/RWnd_EchoExportConfig.h \
    $$PWD/SignalCore/RadarEchoDataAdms.h \
    $$PWD/SignalCore/SignalCoreBuilder.h \
    $$PWD/SignalCore/SignalCoreDefine.h \
    $$PWD/SignalCore/SignalCoreGlobalSignal.h \
    $$PWD/SignalCore/SignalCoreGlobalVal.h \
    $$PWD/SignalCore/SignalCoreMainInterface.h \
    $$PWD/SignalCore/SignalCoreRGCommunicateCore.h \
    $$PWD/SignalCore/SignalDebug.h

SOURCES += \
    $$PWD/SignalCore/RWnd_DefEchoSaveConfig.cpp \
    $$PWD/SignalCore/RWnd_EchoExportConfig.cpp \
    $$PWD/SignalCore/RadarEchoDataAdms.cpp \
    $$PWD/SignalCore/SignalCoreBuilder.cpp \
    $$PWD/SignalCore/SignalCoreDefine.cpp \
    $$PWD/SignalCore/SignalCoreGlobalSignal.cpp \
    $$PWD/SignalCore/SignalCoreGlobalVal.cpp \
    $$PWD/SignalCore/SignalCoreMainInterface.cpp \
    $$PWD/SignalCore/SignalCoreRGCommunicateCore.cpp \
    $$PWD/SignalCore/SignalDebug.cpp

RESOURCES += \
    $$PWD/rc.qrc

FORMS += \
    $$PWD/SignalCore/SignalDebug.ui




