#ifndef SIGNALCOREGLOBALSIGNAL_H
#define SIGNALCOREGLOBALSIGNAL_H

#include<QObject>
#include<Singleton.h>

#include"SignalCoreDefine.h"
namespace SignalCore
{
    class SignalCoreGlobalSignal : public QObject, public Singleton<SignalCore::SignalCoreGlobalSignal>
    {
        Q_OBJECT
        SignalCoreGlobalSignal();
        friend class Singleton;
    public:

    signals:
        void SignalWaveScheduleReload();
        void SignalResetRawSavePath();
        void SignalReadRawEchoData(QString dataName, uint64_t dataNumber, uint64_t dataNumber_max);
        void SignalSpeedRead(double speed);
        void SignalSpeedSend(double speed);
        void SignalSoftwareErrorQuit();
        //回放
        void SignalReplayInfo(QString path, int64_t id, int version);
        void SignalRestart();


    };
    typedef SignalCoreGlobalSignal GlobalSignal;
}

#endif // GLOBALSIGNAL_H
