
#include <QApplication>
#include<GeographicCoordinateConversion.h>
#include<RWnd_PointBLH.h>
#include <QtMath>
#include<CharAndNumProcessing.h>
#include<MQTTRecvSentMng.h>
#include"SignalCore/SignalCoreMainInterface.h"
#include"SignalCore/SignalCoreBuilder.h"



int main(int argc, char* argv[])
{
    initialDebug();
    g_globalStringReceiver = std::bind(&SignalCore::GlobalVal::RFReceiveDebugString,
                                       SignalCore::GlobalVal::GetInstance(),
                                       std::placeholders::_1
                                      );
    bool is_using_window = true;
    if(argc == 1)
    {
//        is_using_window = false;
    }
    else
    {
        if(argc == 2 && (( strcmp( argv[1], "interface" ) ) == 0))
        {
            is_using_window = true;
        }
        else if(argc == 2 && (( strcmp( argv[1], "console" ) ) == 0))
        {
            is_using_window = false;
        }
        else
        {
        }
    }
    SignalCore::SignalCoreBuilder::GetInstance( true, is_using_window, argc, argv);
    return 0x00;
}
