#ifndef SIGNALCOREBUILDER_H
#define SIGNALCOREBUILDER_H
#include<Singleton.h>
#include"SignalCoreMainInterface.h"
#include"SignalCoreRGCommunicateCore.h"
#include"SignalCoreGlobalVal.h"
#include"SignalCoreDefine.h"
#include"SignalCoreGlobalSignal.h"
#include<CUTypeDefine.h>
#include"CommonBuilder.h"
#ifdef linux
    #include<RPSignalProcessCore_SingalArray/CSignalProcessThreadMng.h>
#endif
namespace SignalCore
{
    class SignalCoreBuilder : public QObject, public BuildStatus, public Singleton<SignalCore::SignalCoreBuilder>
    {
        Q_OBJECT
    public:
        SignalCoreBuilder( bool is_main_program = false, bool is_using_window = false, int argc = 0, char* argv[] = nullptr );
        QWidget* getMainWindow();
        void setWriteDataPragram(const function<int64 (char* data, int size, std::string theme)>& writeDataPragram);

        ~SignalCoreBuilder();
        // BuildStatus interface
        void initialWindow();
        virtual bool variableApplication() override;
        virtual bool internalSignalConnect() override;
        virtual bool exec() override;
        virtual void destroy() override;
        virtual void active() override;
        void run();
    private slots:
        void SlotQuit();
    private:
        bool is_using_window = false;
        bool is_main_program = false;
        DECLARE_RESTART_SLOT();


    };
    typedef SignalCoreBuilder Builder;
}

#endif // BUILDER_H
