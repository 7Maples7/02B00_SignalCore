QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14


QT514_DIR = /opt/Qt5.14.2/5.14.2/gcc_64
INCLUDEPATH += $$QT514_DIR/include                $$QT514_DIR/include/QtCore                $$QT514_DIR/include/QtGui                $$QT514_DIR/include/QtWidgets                $$QT514_DIR/include/QtNetwork                $$QT514_DIR/include/QtSql                $$QT514_DIR/include/QtPrintSupport
QMAKE_LIBDIR += $$QT514_DIR/lib
QMAKE_RPATHDIR += $$QT514_DIR/lib
LIBS += -L$$QT514_DIR/lib         $$QT514_DIR/lib/libQt5PrintSupport.so         $$QT514_DIR/lib/libQt5Widgets.so         $$QT514_DIR/lib/libQt5Gui.so         $$QT514_DIR/lib/libQt5Sql.so         $$QT514_DIR/lib/libQt5Network.so         $$QT514_DIR/lib/libQt5Core.so



QMAKE_CC += -g
QMAKE_CXX += -g
QMAKE_LINK += -g

# 启用 AddressSanitizer
#QMAKE_CXXFLAGS += -fsanitize=address
#QMAKE_LFLAGS += -fsanitize=address







# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

#TARGET = MQTTThemeInfoDisp_V1.0

DEFINES += myTargetString=\"$$TARGET\"


RC_ICONS = rc/SignalCore.ico

TEMPLATE = app

RootDir = $$PWD/../..



ReleaseName = 02B00_SignalCore
DESTDIR = $$RootDir/bin/01RadarSoftware/$$ReleaseName/
OBJECTS_DIR = $$RootDir/build/01RadarSoftware/$$ReleaseName/
TARGET = $$ReleaseName

BUILD_ROOT = $$OBJECTS_DIR
# 2. 目标文件（.o/.obj）输出路径
OBJECTS_DIR = $$BUILD_ROOT/obj
# 3. moc生成文件路径
MOC_DIR = $$BUILD_ROOT/moc
# 4. rcc生成文件路径
RCC_DIR = $$BUILD_ROOT/rcc
# 5. uic生成文件路径
UI_DIR = $$BUILD_ROOT/ui

SOURCES += \
    main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

include($$RootDir/PlatformLibraryStatic/PlatformLibraryStatic.pri)

include($$RootDir/PlatformLibraryStatic/PrimAlgorithmLib/MultiModel/MultiModel.pri)


include( SignalCore.pri)

include( ../02A00_RadarSPMngUnion/Common/Common.pri)
unix{
include( RPSignalProcessCore_SingalArray/RPSignalProcessCore_SingalArray.pri )
}



