#ifndef CSIGNALPROCESSTHREADMNG_H
#define CSIGNALPROCESSTHREADMNG_H
#include<Singleton.h>
#include<QObject>
#include<CUTypeDefine.h>
//#include<SPGlobalVar.h>
//#include<SPGlobalSignal.h>
#include<CThreadManagerPool.h>
#include<RCSignalProcessMain.h>
#include<Singleton.h>
#include<SignalCore/SignalCoreDefine.h>
#include<SignalCore/SignalCoreGlobalVal.h>
#include<SignalCore/SignalCoreGlobalSignal.h>
#include<RGTimeSystem.h>
#include<Common/CommonGlobalVal.h>
#include<Common/CommonRGCommunicateCore.h>
#include<Common/CommonRGCommunicateCore.h>
#include"SignalCore/SignalCoreRGCommunicateCore.h"
#include"SignalCore/RadarEchoDataAdms.h"
#include"SignalCore/SignalCoreGlobalSignal.h"
#include<CUTypeDefine.h>
#include"CSafeShareMemoryZmq.h"
#include<ScndLvlDefn/DSignalProcPointProtocol.h>

class CSignalProcessThreadMng : public QObject, public BuildStatus, public Singleton<CSignalProcessThreadMng>
{

    Q_OBJECT
    friend class Singleton;
    CSignalProcessThreadMng();
    ~CSignalProcessThreadMng();
public:

signals:
private slots:

    /** @brief 波形参数重载   2023-12-15 09:00:05 .*/
    void SlotWaveInfoReload();
    void RFWaveInfoReload();
    void SlotDataParSucess();
public:
    bool variableApplication();
    bool internalSignalConnect();
    bool exec();
    void destroy();
    void active();
    /************************************| |************************************/
    int RFReadShareMemory(char* data, int size);
    int RFReadShareMemoryT(char* data, int size)
    {
        Q_UNUSED(data);
        Q_UNUSED(size);
        return 0x00;
    }

    /** @brief 录取的回波数据的回放   2025-04-28 13:07:16 .*/
    int RFReplayEchoFromData(DefReplayInfo dataInfo);
    /** @brief 录取的回波数据的回放   2025-04-28 13:07:16 .*/
    int RFReplayEchoFromDataOld(DefReplayInfo datainfo);
    //1. typedefs和enums；
    //2. 常量；
    //3. 成员函数，含静态成员函数；
public:

private:
    bool validateReceivedData(const RawData2SPHead* head, int size) const;
    void run();
    void RFWaveReloadCheck()
    {
        static int64 timeWaveReload = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
        if( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - timeWaveReload >= 6000 )
        {
            timeWaveReload = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
            if( this->waveReloadCmd.size() )
            {
                auto v = RWLock::make_write_guard(this->waveReloadCmdLocker);
                waveReloadCmd.clear();
                this->RFWaveInfoReload();
            }
        }
    }
    void RFReadSendSpeedCal()
    {
        if( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() -  speed_cal_time > 1000)
        {
            auto timeDiff =  RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - this->speed_cal_time;
            timeDiff = timeDiff / 1000.0;
            emit SignalCore::GlobalSignal::GetInstance()->SignalSpeedRead( this->data_size_recv / timeDiff );
            emit SignalCore::GlobalSignal::GetInstance()->SignalSpeedSend( this->data_size_send / timeDiff );
            this->data_size_recv = 0;
            this->data_size_send = 0;
            speed_cal_time = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
        }
    }
    //4. 数据成员，含静态数据成员。
private:
    RCSignalProcessMain* mSpProcCore = nullptr;
    RWLock waveReloadCmdLocker;
    QList<int64_t> waveReloadCmd;
    //接收与发送目标速率
    double data_size_recv = 0;
    double data_size_send = 0;
    int64_t speed_cal_time = 0;
    ThreadSafetyVar<int64_t>  SPRuningFlg;
    moodycamel::ConcurrentQueue<DefReplayInfo>  mReplayTask;
    CSafeShareMemoryClient* mShareMemoryClient = nullptr;
    //杂波图
    int64_t mTimeStampStart = 0;

};

#endif // CSIGNALPROCESSTHREADMNG_H
