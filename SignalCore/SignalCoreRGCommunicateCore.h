#ifndef SIGNALCORERGCOMMUNICATECORE_H
#define SIGNALCORERGCOMMUNICATECORE_H

#include<Singleton.h>
#include<CUTypeDefine.h>
#include<ZeromqSubPub.h>
#include<RMModuleRewriting>
#include<CommonDefine.h>
#include<DefExchangeWithSP.h>
#include"CSafeShareMemoryZmq.h"
#include"SignalCoreDefine.h"
#include"SignalCoreGlobalVal.h"
#include"SignalCoreGlobalSignal.h"
#include<ZeromqSubPub.h>
#include<MD5.h>
#include<MacCommunicate.h>
#include<ScndLvlDefn/DSignalProcPointProtocol.h>
using namespace Common;
namespace SignalCore
{
    class SignalCoreRGCommunicateCore : public BuildStatus, public Singleton<SignalCoreRGCommunicateCore>
    {
        friend class Singleton;
        SignalCoreRGCommunicateCore();
        ~SignalCoreRGCommunicateCore();

    public:
        /** @brief 外界数据输入接口，需要处理好多线程数据安全   2024-10-04 20:50:51 .*/
        void RFReadDataPragram( char* /*data*/, int /*size*/, std::string /*theme*/ );
        /** @brief 数据写入接口   2024-10-06 17:01:32 .*/
        int64_t RFWriteDataPragram( char* data, int size, std::string theme );
        void    setWriteDataPragram(const function<int64 (char* data, int size, std::string theme)>& writeDataPragram);

    protected:
        // BuildStatus interface
        virtual bool variableApplication() override;
        virtual bool internalSignalConnect() override;
        virtual bool exec() override;
        virtual void destroy() override;
        virtual void active() override;
        void run();
    private:
        function<int64( char* data, int size, std::string theme )> mWriteDataPragram = nullptr;
        ZeromqSocket* mSocketSub = nullptr;
        ZeromqServer* mSocketPub = nullptr;

    public:
        /** @brief 波形检测   2024-11-22 10:28:53 .*/
        bool RFWaveInfoCheck(Common::DefWaveInfo& waveInfo);
    public:
        void RFSendMicroDopplerData(DefMicroDopplerMatrix& dopCls);
        int  RFSendPoints(shared_ptr<Common::RawData2SP> echoInfo,
                          QList<IRadarV51::SignalProcPoint>& pointsFrmSP,

                          bool angleIsDeg);
    private:        /** @brief 发送和接收信号处理数据的共享内存通信   2025-01-18 16:11:43 .*/
        ZeromqServer* mSocketSendPoints = nullptr;
//        ZeromqServer* zmqPub = nullptr;
    };
    typedef SignalCoreRGCommunicateCore RGCommunicateCore;
}
#endif // RGCOMMUNICATECORE_H
