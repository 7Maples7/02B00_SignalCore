#include "SignalCoreMainInterface.h"
namespace SignalCore
{
    SignalCoreMainInterface::SignalCoreMainInterface( bool isAddWndCtrlButton, QString cfg_path, QString title, QWidget* parent)
        : DockMainWindowSwitch( isAddWndCtrlButton, cfg_path, title, parent )
    {
        if(SignalCoreGlobalVal::GetInstance()->getIsMainProgram())
        {
            qDebug() << "主程序";
        }
        else
        {
            title = STR1( SignalCore );
            qDebug() << "不是主程序，当前title:" << title;
        }
        this->active();
        this->DockLoadConfigDefault();
    }

    SignalCoreMainInterface::~SignalCoreMainInterface()
    {
        this->destroy();
    }

    void SignalCoreMainInterface::SlotSpeedRead(double speed)
    {
        this->speedRecv.setValue(speed);
    }

    void SignalCoreMainInterface::SlotSpeedSend(double speed)
    {
        this->speedSend.setValue(speed);
    }

    bool SignalCoreMainInterface::variableApplication()
    {
        if(SignalCoreGlobalVal::GetInstance()->getIsMainProgram())
        {
            this->setCenterTitle("信号处理核");
        }
        else
        {
            this->setCenterTitle("***");
        }
        this->addWidget(SignalDebug::GetInstance(), "回波回放" );
        this->addWidget(new Common::RWnd_Struct_SP_para(SignalCore::GlobalVal::GetInstance()->occupyThroldDebug()), "调试门限");
        this->addWidget( new  RWnd_PointBLH(SignalCoreGlobalVal::GetInstance()->occupyRadarGPS()), "雷达GPS" );
        this->addWidget( new  RWnd_DefEchoSaveConfig(SignalCoreGlobalVal::GetInstance()->occupyRadarDataSaveInfo()), "回波数据录取" );
        this->mEchoExportConfigWnd = new RWnd_EchoExportConfig();
        this->mEchoExportConfigWnd->setWindowTitle("回波录取配置");
        this->mEchoExportConfigWnd->setWindowFlag(Qt::Window, true);
        this->mEchoExportConfigWnd->resize(420, 220);
        //如果不是主程序，则不能有界面的通信管理
        if(SignalCoreGlobalVal::GetInstance()->getIsMainProgram())
        {
            mDataCommunicateCore = new MQTTRecvSentMng(
                SignalCoreGlobalVal::GetInstance()->occupyDataSubAddr(),
                SignalCoreGlobalVal::GetInstance()->occupyDataPubAddr(),
                SignalCoreGlobalVal::GetInstance()->occupyDataSubThemeLst(),
                std::bind( &SignalCoreMainInterface::RFReadDataPragram, this, placeholders::_1, placeholders::_2, placeholders::_3 )
            );
            this->addWidget( this->mDataCommunicateCore, "通信管理"  );
            RGCommunicateCore::GetInstance()->setWriteDataPragram( std::bind( &MQTTRecvSentMng::writeDatagram,
                    mDataCommunicateCore,
                    placeholders::_1,
                    placeholders::_2,
                    placeholders::_3
                                                                            )  );
        }
        speedRecv.setRange(0, 256);
        speedSend.setRange(0, 256);
        speedRecv.setUnit("个/s");
        speedSend.setUnit("个/s");
        speedRecv.setInstructions("接收速率");
        speedSend.setInstructions("发送速率");
        QGroupBox* box = new QGroupBox(this);
        box->setFixedSize( 500, 314 );
        RHBoxLayout* layout = new RHBoxLayout(box);
        layout->addWidget(&this->speedRecv);
        layout->addWidget(&this->speedSend);
        this->addWidget( box, "速率展示" );
        return true;
    }

    bool SignalCoreMainInterface::internalSignalConnect()
    {
        connect(this->file->addAction("当前路径"), &QAction::triggered, [ = ]()
        {
            QString path = QDir::currentPath(); //获取程序当前目录
            QDesktopServices::openUrl(QUrl(QString("file:///%0").arg(path), QUrl::TolerantMode));
        });
        connect(this->file->addAction("保存配置"), &QAction::triggered, [ = ]()
        {
            SignalCoreGlobalVal::GetInstance()->SaveConfig();
        });
        connect(this->file->addAction("加载配置"), &QAction::triggered, [ = ]()
        {
            SignalCoreGlobalVal::GetInstance()->LoadConfig();
        });
        connect(this->file->addAction("配置另存为"), &QAction::triggered, [ = ]()
        {
            QString fileName = QFileDialog::getSaveFileName( this, ("打开配置文件"), "./", ("数据文件(*.db)"));
            SignalCoreGlobalVal::GetInstance()->SaveConfigAs(fileName);
        });
        connect(this->tool->addAction("解析数据"), &QAction::triggered, [ = ]()
        {
            QString fileName = QFileDialog::getOpenFileName(this, tr("打开原始数据库 文件"), "./", tr("数据文件(*.db)"));
            if(fileName.isEmpty() == false)
            {
#ifdef linux
                double rate;
                bool is_running = false;
                QDialog dialog;
                dialog.setFixedSize(500, 309);
                QVBoxLayout layout(&dialog);
                QTimer timerCycle ;
                QLabel title;
                layout.addWidget(&title);
                timerCycle.start(1000);
                std::thread t = std::thread( std::bind( &RadarEchoDataAdms::RFExtractRawData, RadarEchoDataAdms::GetInstance(),
                                                        placeholders::_1, placeholders::_2, placeholders::_3),
                                             std::ref( rate), std::ref( is_running),  fileName);
                t.detach();
                connect(&timerCycle, &QTimer::timeout, [ &dialog, &rate, &is_running, &title ]()
                {
                    if( is_running == false )
                    {
                        dialog.accept();
                    }
                    else
                    {
                        title.setText(QString("当前进度:%0%").arg( rate * 100 ));
                    }
                });
                dialog.exec();
#else
                QtMaterialCircularProgressCustom dialog;
                dialog.runingFunc<QString>( std::bind( &RadarEchoDataAdms::RFExtractRawData, RadarEchoDataAdms::GetInstance(),
                                                       placeholders::_1, placeholders::_2, placeholders::_3),
                                            fileName
                                          );
                dialog.exec();
#endif
            }
        });
        connect(this->tool->addAction("回波录取配置"), &QAction::triggered, [this]()
        {
            if(this->mEchoExportConfigWnd)
            {
                this->mEchoExportConfigWnd->show();
                this->mEchoExportConfigWnd->raise();
                this->mEchoExportConfigWnd->activateWindow();
            }
        });
        connect(this->file->addAction("从文件加载配置"), &QAction::triggered, [ = ]()
        {
            QString fileName = QFileDialog::getOpenFileName(this, ("打开配置文件"), "./", ("数据文件(*.db)"));
            SignalCoreGlobalVal::GetInstance()->LoadConfigFrm( fileName );
        });
        connect( SignalCore::GlobalSignal::GetInstance(), &SignalCoreGlobalSignal::SignalSpeedRead, this, &SignalCoreMainInterface::SlotSpeedRead );
        connect( SignalCore::GlobalSignal::GetInstance(), &SignalCoreGlobalSignal::SignalSpeedSend, this, &SignalCoreMainInterface::SlotSpeedSend );
        return true;
    }

    bool SignalCoreMainInterface::exec()
    {
        return true;
    }

    void SignalCoreMainInterface::destroy()
    {
    }

    void SignalCoreMainInterface::active()
    {
        this->variableApplication();
        this->internalSignalConnect();
        this->exec();
    }

    void SignalCoreMainInterface::RFReadDataPragram( char* data, int size, std::string theme )
    {
        RGCommunicateCore::GetInstance()->RFReadDataPragram(data, size, theme);
    }
}
