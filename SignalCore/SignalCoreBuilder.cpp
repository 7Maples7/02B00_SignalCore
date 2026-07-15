#include "SignalCoreBuilder.h"

namespace SignalCore
{

    SignalCoreBuilder::SignalCoreBuilder( bool is_main_program, bool is_using_window, int argc, char* argv[])
    {
        this->is_using_window = is_using_window;
        this->is_main_program = is_main_program;
        GlobalVal::GetInstance()->setIsMainProgram( is_main_program );
        GlobalVal::GetInstance()->setIsUsingWindows(is_using_window);
        iniSlotRestart(argc, argv);
        if(is_main_program)
        {
            if( is_using_window )
            {
                QApplication a(argc, argv);
                Common::GlobalVal::GetInstance();
                this->active();
                SignalCoreMainInterface::GetInstance()->show();
                SignalCoreMainInterface::GetInstance()->setWindowIcon(QIcon(":/rc/SignalCore.png"));
                a.exec();
                this->destroy();
            }
            else
            {
                QCoreApplication a(argc, argv);
                this->active();
                a.exec();
                this->destroy();
            }
        }
        else
        {
            this->active();
        }
    }

    QWidget* SignalCoreBuilder::getMainWindow()
    {
        assert( this->is_using_window == true );
        if(this->is_using_window)
        {
            return SignalCoreMainInterface::GetInstance();
        }
        return nullptr;
    }

    SignalCoreBuilder::~SignalCoreBuilder()
    {
        Common::GlobalVal::deleteInstance();
        this->destroy();
    }

    void SignalCoreBuilder::initialWindow()
    {
        if( is_using_window )
        {
            bool isAddWndCtrlButton = true;
            QString cfg_path =  "./config";
            QString title = STR( myTargetString );
            QWidget* parent = nullptr;
            if( SignalCoreGlobalVal::GetInstance()->getIsMainProgram()  == false)
            {
                title = STR1(SignalCore);
            }
            SignalCoreMainInterface::GetInstance( isAddWndCtrlButton, cfg_path, title, parent );
        }
    }

    bool SignalCoreBuilder::variableApplication()
    {
        GlobalVal::GetInstance();
        RGCommunicateCore::GetInstance();
        GlobalSignal::GetInstance();
#ifdef linux
        CSignalProcessThreadMng::GetInstance();
#endif
        return true;
    }

    bool SignalCoreBuilder::internalSignalConnect()
    {
        connect(GlobalSignal::GetInstance(), &SignalCoreGlobalSignal::SignalSoftwareErrorQuit, this, &SignalCoreBuilder::SlotQuit);
        return true;
    }

    bool SignalCoreBuilder::exec()
    {
        std::thread t( std::bind( &SignalCoreBuilder::run, this ) );
        t.detach();
        return true;
    }

    void SignalCoreBuilder::destroy()
    {
        this->isStop = true;
        while (this->mIsRuning)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
#ifdef linux
        CSignalProcessThreadMng::deleteInstance();
        RGTimeSystem::GetInstance()->RFDelayMs(100);
#endif
        GlobalSignal::deleteInstance();
        RGTimeSystem::GetInstance()->RFDelayMs(100);
        RGCommunicateCore::deleteInstance();
        RGTimeSystem::GetInstance()->RFDelayMs(100);
        GlobalVal::deleteInstance();
        RGTimeSystem::GetInstance()->RFDelayMs(100);
    }

    void SignalCoreBuilder::active()
    {
        this->variableApplication();
        this->initialWindow();
        this->internalSignalConnect();
        this->exec();
    }

    void SignalCoreBuilder::run()
    {
        this->mIsRuning = true;
        while (this->isStop == false)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
        this->mIsRuning = false;
    }

    void SignalCoreBuilder::SlotQuit()
    {
        this->destroy();
        exit(0);
    }

    void SignalCoreBuilder::setWriteDataPragram(const function<int64 (char*, int, string)>& writeDataPragram)
    {
        RGCommunicateCore::GetInstance()->setWriteDataPragram( writeDataPragram );
    }

}
