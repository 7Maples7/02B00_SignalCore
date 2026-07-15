#include "SignalDebug.h"
#include "ui_SignalDebug.h"
namespace SignalCore
{
    SignalDebug::SignalDebug(QWidget* parent) :
        QWidget(parent),
        ui(new Ui::SignalDebug)
    {
        ui->setupUi(this);
        this->active();
    }

    SignalDebug::~SignalDebug()
    {
        delete ui;
        this->destroy();
    }

    bool SignalDebug::variableApplication()
    {
        RGroupBox* mainwnd = new RGroupBox(this->title, this);
        RVBoxLayout* mainLayoutV = new RVBoxLayout(this);
        mainLayoutV->addWidget(mainwnd);
        mainLayoutV->addSpacerItem();
        RGridLayout* mainLayout = new RGridLayout(mainwnd);
        qint32 row = 0;
        qint32 col = 0;
        mainLayout->addSpacerItem(100, 100, 1);
        /********************| 加载原始数据-单包 |********************/
        RLabel* LoadRawDataFrmSQL_RLabel = new RLabel("加载原始数据-单包", this);
        this->LoadRawDataFrmSQL_RPushButton = new RPushButton("加载原始数据-单包", this);
        mainLayout->addWidget(LoadRawDataFrmSQL_RLabel, row, col);
        mainLayout->addWidget(LoadRawDataFrmSQL_RPushButton, row, col + 1, 1, 1);
        row++;
        /********************| 加载所有回波 |********************/
        RLabel* LoadAllDataFrmSQL_RLabel = new RLabel("加载所有回波", this);
        this->LoadAllDataFrmSQL_RPushButton = new RPushButton("加载所有回波", this);
        mainLayout->addWidget(LoadAllDataFrmSQL_RLabel, row, col);
        mainLayout->addWidget(LoadAllDataFrmSQL_RPushButton, row, col + 1, 1, 1);
        row++;
        return true;
    }

    bool SignalDebug::internalSignalConnect()
    {
        connect(this->LoadRawDataFrmSQL_RPushButton, &RPushButton::clicked, [ = ]()
        {
            this->SlotLoadSingleEcho();
        });
        /********************| 加载所有回波 |********************/
        connect(this->LoadAllDataFrmSQL_RPushButton, &RPushButton::clicked, [ = ]()
        {
            SlotLoadAllEcho();
        });
        connect( &this->mTimer4SendReplayInfo, &QTimer::timeout, [this]()
        {
            if( this->mReplayInfoList.size() )
            {
                auto v = this->mReplayInfoList.first();
                emit SignalCoreGlobalSignal::GetInstance()->SignalReplayInfo(v.db_path, v.id, v.version);
//                this->mReplayInfoList.removeFirst();
            }
        } );
        return true;
    }



    bool SignalDebug::exec()
    {
        std::thread t( std::bind( &SignalDebug::run, this ) );
        t.detach();
        mTimer4SendReplayInfo.start(500);
        return true;
    }

    void SignalDebug::destroy()
    {
        this->isStop = true;
        while (this->mIsRuning)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
    }

    void SignalDebug::active()
    {
        this->variableApplication();
        this->internalSignalConnect();
        this->exec();
    }

    void SignalDebug::run()
    {
        this->mIsRuning = true;
        while (this->isStop == false)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
        this->mIsRuning = false;
    }

    void SignalDebug::SlotLoadSingleEcho()
    {
        QString fileName;
        QByteArray data;
        if( LoadData( data, "config.cfg") )
        {
            fileName = QString::fromUtf8( data );
        }
        fileName   = QFileDialog::getOpenFileName(this, tr("打开回波数据"), QFileInfo(fileName).absoluteFilePath(),
                     tr("数据文件(*.dat);数据库文件(*.db)"));
        if( fileName.isEmpty() == false )
        {
            SaveData( fileName.toUtf8(), "config.cfg" );
        }
        if( fileName.isEmpty() == false )
        {
            int64_t identifyNbr = 0;
            int Version = 0;
            QDialog dialog;
            QTextEdit mDebugEdit;
            if( QFileInfo( fileName ).suffix() == "db" )
            {
                SqlQuery query( fileName );
                if(query.exec("SELECT _mIdentifyNbr FROM EchoDataInfo;"))
                {
                    int row = 0;
                    while (query.next())
                    {
                        row++;
                        if( row > 5 )
                        {
                            mDebugEdit.append("\n");
                            row = 0;
                        }
                        mDebugEdit.append( QString("%0 ").arg( query.value(0).toLongLong() ) );
                    }
                }
            }
            dialog.setWindowTitle("输入数据的识别号");
            dialog.resize(500, 300);
            RVBoxLayout layout(&dialog);
            layout.addSpacerItem();
            // 创建 QLineEdit 控件
            QLineEdit idxEdit;
            QComboBox VersionEdit;
            VersionEdit.addItem("0:旧版");
            VersionEdit.addItem("1:新版");
            QRegExp regExp("[0-9]*");
            // 创建验证器，并将正则表达式与之关联
            QValidator* validator = new QRegExpValidator(regExp, &idxEdit);
            // 将验证器设置给 QLineEdit 控件
            idxEdit.setValidator(validator);
            QByteArray data;
            LoadData(data, "identifyNbr.dat");
            idxEdit.setText( QString::number(* reinterpret_cast<int64_t*>(data.data()) ) );
            layout.addWidget( &idxEdit );
            layout.addWidget(&VersionEdit);
            layout.addWidget(&mDebugEdit);
            layout.addSpacerItem();
            dialog.exec();
            QString idx_text = idxEdit.text();
            bool ok;
            identifyNbr =  idx_text.toLongLong(&ok, 10);
            if(!ok)
            {
                qDebug() << "xxx";
                qDebug() << "识别号转换失败";
            }
            else
            {
                Version = VersionEdit.currentIndex();// text().toInt(&ok, 10);
                qDebug() << "选择回波数据:" << fileName << identifyNbr << Version;
                emit SignalCoreGlobalSignal::GetInstance()->SignalReplayInfo(fileName, identifyNbr, Version);
            }
        }
    }

    void SignalDebug::SlotLoadAllEcho()
    {
        QString fileName = QFileDialog::getOpenFileName(nullptr, tr("打开回波数据库文件"), "./", tr("数据文件(*.db)"));
        if( fileName.isEmpty() == false )
        {
            SqlQuery query( fileName );
            if(query.exec("SELECT _mIdentifyNbr FROM EchoDataInfo;"))
            {
                while (query.next())
                {
                    DefReplayInfo info;
                    info.id =   query.value(0).toLongLong() ;
                    info.db_path = fileName;
                    info.version = 0x01;
                    this->mReplayInfoList.push_back( info );
                }
            }
        }
    }
}
