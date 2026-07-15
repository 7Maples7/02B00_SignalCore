#ifndef SIGNALCOREMAININTERFACE_H
#define SIGNALCOREMAININTERFACE_H

#include<DockMainWindowSwitch.h>
#include<CUTypeDefine.h>
#include<Singleton.h>
#include"SignalCoreGlobalVal.h"
#include"SignalCoreRGCommunicateCore.h"
#include<RWnd_PointBLH.h>
#include<MQTTRecvSentMng.h>
#include"RWnd_DefEchoSaveConfig.h"
#include"RWnd_EchoExportConfig.h"
#include"QtMaterialCircularProgressCustom.h"
#include"RadarEchoDataAdms.h"
#include<RWTransmissionSpeedDispDial.h>
#include"SignalCoreGlobalSignal.h"
#include"RWnd_Struct_SP_para.h"
#include<CommonGlobalVal.h>
#include"SignalDebug.h"

namespace SignalCore
{
    class SignalCoreMainInterface : public DockMainWindowSwitch,  public BuildStatus, public Singleton<SignalCore::SignalCoreMainInterface>
    {
        Q_OBJECT
        friend class Singleton;

#if defined( myTargetString)
        SignalCoreMainInterface( bool isAddWndCtrlButton = true,  QString cfg_path =  "./config", QString title = STR( myTargetString ), QWidget* parent = nullptr);
#else
        SignalCoreMainInterface( bool isAddWndCtrlButton = true,  QString cfg_path =  "./config", QString title = "SoftMng", QWidget* parent = nullptr);
#endif
        ~SignalCoreMainInterface();

    public:

    public slots:
        void SlotSpeedRead(double speed);
        void SlotSpeedSend(double speed);
        // BuildStatus interface
    public:
        virtual bool variableApplication() override;
        virtual bool internalSignalConnect() override;
        virtual bool exec() override;
        virtual void destroy() override;
        virtual void active() override;
    private:
        void RFReadDataPragram( char* /*data*/, int /*size*/, std::string /*theme*/ );
    private:
        MQTTRecvSentMng* mDataCommunicateCore = nullptr;
        RWnd_EchoExportConfig* mEchoExportConfigWnd = nullptr;
        TransmissionSpeedDispDial speedRecv;
        TransmissionSpeedDispDial speedSend;


    };
    typedef SignalCoreMainInterface MainInterface;
}
#endif // MAININTERFACE_H
