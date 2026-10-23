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
        /**
         * @brief 接收并分派外部通信数据。
         * @details 除既有通用消息处理外，负责处理回波数据录取配置的 Query 和 Command，完成报文校验、配置持久化、生效和响应发送。
         * @param[in] data 输入报文首地址。
         * @param[in] size 输入报文长度，单位为字节。
         * @param[in] theme 接收主题。
         * @return 无。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        void RFReadDataPragram(char* data, int size, std::string theme);
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
