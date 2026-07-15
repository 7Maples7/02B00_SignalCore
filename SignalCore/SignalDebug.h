#ifndef SIGNALDEBUG_H
#define SIGNALDEBUG_H

#include <QWidget>
#include"SignalCoreDefine.h"
#include<RMModuleRewriting>
#include "SignalCoreGlobalSignal.h"
#include "CharAndNumProcessing.h"
using namespace RObject;
namespace Ui
{
    class SignalDebug;
}
#include<CUTypeDefine.h>
#include<Singleton.h>
#include<RGTimeSystem.h>
#include<ConnectionPool.h>
namespace SignalCore
{
    class SignalDebug : public QWidget, public BuildStatus, public Singleton<SignalDebug>
    {
        Q_OBJECT

    public:
        explicit SignalDebug(QWidget* parent = nullptr);
        ~SignalDebug();
        // BuildStatus interface
        virtual bool variableApplication() override;
        virtual bool internalSignalConnect() override;
        virtual bool exec() override;
        virtual void destroy() override;
        virtual void active() override;
        void run();

    protected slots:
        void SlotLoadSingleEcho();
        void SlotLoadAllEcho();


    private:
        QList<DefReplayInfo> mReplayInfoList;
        Ui::SignalDebug* ui;
        QString title = "";
        RPushButton* LoadRawDataFrmSQL_RPushButton = nullptr;//加载原始数据-单包
        RPushButton* LoadAllDataFrmSQL_RPushButton = nullptr;//加载所有回波
        QTimer mTimer4SendReplayInfo;
    };
}
#endif // SIGNALDEBUG_H
