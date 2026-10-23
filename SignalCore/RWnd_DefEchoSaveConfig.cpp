#include"RWnd_DefEchoSaveConfig.h"
using namespace SignalCore;

RWnd_DefEchoSaveConfig::RWnd_DefEchoSaveConfig(DefEchoSaveConfig* par, QWidget* parent, QString title ) : QWidget(parent)
{
    assert(par);
    this->title = title;
    this->par = par;
    this->variableApplication();
    this->update();
    this->internalSignalConnect();
    this->exec();
}
bool RWnd_DefEchoSaveConfig::variableApplication()
{
    RGroupBox* mainwnd = new RGroupBox(this->title, this);
    RVBoxLayout* mainLayoutV = new RVBoxLayout(this);
    mainLayoutV->addWidget(mainwnd);
    mainLayoutV->addSpacerItem();
    RGridLayout* mainLayout = new RGridLayout(mainwnd);
    qint32 row = 0;
    qint32 col = 0;
    mainLayout->addSpacerItem(100, 100, 1);
    /********************| 是否保存 |********************/
    this->mIsSaveRawData_RCheckBox = new RCheckBox("是否保存", this);
    RLabel* mIsSaveRawData_lable = new RLabel("是否保存", this);
    mainLayout->addWidget(mIsSaveRawData_lable, row, col);
    mainLayout->addWidget(this->mIsSaveRawData_RCheckBox, row, col + 1);
    row++;
    /********************| 保存类型 |********************/
    RLabel* mDataSaveBy_RLabel = new RLabel("保存类型");
    this->mDataSaveBy_RComboBox = new RComboBox(this);
    QMap<int, QString> strItem;
    strItem[0x00] = "无效";
    strItem[0x01] = "阵面坐标";
    strItem[0x02] = "站心坐标";
    strItem[0x03] = "跟踪波束";
    strItem[0x04] = "识别波束";
    strItem[0x05] = "无人机";
    strItem[0x06] = "ADSB";
    ComboxUnionMng<int>::getInstance()->RFComboxInitial( this->mDataSaveBy_RComboBox, strItem );
    mainLayout->addWidget(mDataSaveBy_RLabel, row, col, 1, 1);
    mainLayout->addWidget(mDataSaveBy_RComboBox, row, col + 1, 1, 1);
    row++;
    /********************| 方位下限 |********************/
    RLabel* azi_deg_min_RLabel = new RLabel("方位下限", this);
    this->azi_deg_min_RDoubleSpinBox = new RDoubleSpinBox(this);
    this->azi_deg_min_RDoubleSpinBox->setMaximum(1e6);
    this->azi_deg_min_RDoubleSpinBox->setMinimum(-1e6);
    this->azi_deg_min_RDoubleSpinBox->setSingleStep(1);
    mainLayout->addWidget(azi_deg_min_RLabel, row, col, 1, 1);
    mainLayout->addWidget(azi_deg_min_RDoubleSpinBox, row, col + 1, 1, 1);
    row++;
    /********************| 方位上限 |********************/
    RLabel* azi_deg_max_RLabel = new RLabel("方位上限", this);
    this->azi_deg_max_RDoubleSpinBox = new RDoubleSpinBox(this);
    this->azi_deg_max_RDoubleSpinBox->setMaximum(1e6);
    this->azi_deg_max_RDoubleSpinBox->setMinimum(-1e6);
    this->azi_deg_max_RDoubleSpinBox->setSingleStep(1);
    mainLayout->addWidget(azi_deg_max_RLabel, row, col, 1, 1);
    mainLayout->addWidget(azi_deg_max_RDoubleSpinBox, row, col + 1, 1, 1);
    row++;
    /********************| 俯仰下限 |********************/
    RLabel* ele_deg_min_RLabel = new RLabel("俯仰下限", this);
    this->ele_deg_min_RDoubleSpinBox = new RDoubleSpinBox(this);
    this->ele_deg_min_RDoubleSpinBox->setMaximum(1e6);
    this->ele_deg_min_RDoubleSpinBox->setMinimum(-1e6);
    this->ele_deg_min_RDoubleSpinBox->setSingleStep(1);
    mainLayout->addWidget(ele_deg_min_RLabel, row, col, 1, 1);
    mainLayout->addWidget(ele_deg_min_RDoubleSpinBox, row, col + 1, 1, 1);
    row++;
    /********************| 俯仰上限 |********************/
    RLabel* ele_deg_max_RLabel = new RLabel("俯仰上限", this);
    this->ele_deg_max_RDoubleSpinBox = new RDoubleSpinBox(this);
    this->ele_deg_max_RDoubleSpinBox->setMaximum(1e6);
    this->ele_deg_max_RDoubleSpinBox->setMinimum(-1e6);
    this->ele_deg_max_RDoubleSpinBox->setSingleStep(1);
    mainLayout->addWidget(ele_deg_max_RLabel, row, col, 1, 1);
    mainLayout->addWidget(ele_deg_max_RDoubleSpinBox, row, col + 1, 1, 1);
    row++;
    /********************| 数据录取根目录 |********************/
    this->rootFolder_RPushButton = new RPushButton("数据录取根目录", this);
    mainLayout->addWidget(rootFolder_RPushButton, row, col, 1, 2);
    row++;
    return true;
}




bool RWnd_DefEchoSaveConfig::internalSignalConnect()
{
    connect(GlobalSignal::GetInstance(),
            &GlobalSignal::SignalEchoDataCaptureConfigRefresh,
            this,
            &RWnd_DefEchoSaveConfig::update);
    /********************| 是否保存 |********************/
    connect(this->mIsSaveRawData_RCheckBox, &RCheckBox::clicked, [this](bool checked )
    {
        this->par->mIsSaveRawData = checked * RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
        RFSendSignals();
    });
    /********************| 保存类型 |********************/
    connect(this->mDataSaveBy_RComboBox, &RComboBox::currentTextChanged, [this]()
    {
        int v;
        ComboxUnionMng < int >::getInstance()->RFComboxCurrentIdxChanged(this->mDataSaveBy_RComboBox, v );
        this->par->mDataSaveBy = v;
        RFSendSignals();
    });
    /********************| 方位下限 |********************/
    connect(this->azi_deg_min_RDoubleSpinBox, &RDoubleSpinBox::textChanged, [this]()
    {
        this->par->azi_deg_min = this->azi_deg_min_RDoubleSpinBox->value();
        RFSendSignals();
    });
    /********************| 方位上限 |********************/
    connect(this->azi_deg_max_RDoubleSpinBox, &RDoubleSpinBox::textChanged, [this]()
    {
        this->par->azi_deg_max = this->azi_deg_max_RDoubleSpinBox->value();
        RFSendSignals();
    });
    /********************| 俯仰下限 |********************/
    connect(this->ele_deg_min_RDoubleSpinBox, &RDoubleSpinBox::textChanged, [this]()
    {
        this->par->ele_deg_min = this->ele_deg_min_RDoubleSpinBox->value();
        RFSendSignals();
    });
    /********************| 俯仰上限 |********************/
    connect(this->ele_deg_max_RDoubleSpinBox, &RDoubleSpinBox::textChanged, [this]()
    {
        this->par->ele_deg_max = this->ele_deg_max_RDoubleSpinBox->value();
        RFSendSignals();
    });
    /********************| 数据录取根目录 |********************/
    connect(this->rootFolder_RPushButton, &RPushButton::clicked, [ = ]()
    {
        QString fileName = QFileDialog::getExistingDirectory(this, tr("打开数据 文件"), this->par->rootFolder);
        if(fileName.isEmpty() == false)
        {
            this->par->rootFolder  =  fileName;
            this->rootFolder_RPushButton->setToolTip(fileName);
        }
    });
    return true;
}



bool RWnd_DefEchoSaveConfig::exec()
{
    this->update();
    return true;
}



void RWnd_DefEchoSaveConfig::update()
{
    bool signalValid = this->SignalValid;
    this->SignalValid = false;
    this->mIsSaveRawData_RCheckBox->setChecked(this->par->mIsSaveRawData);//是否保存
    ComboxUnionMng<int>::getInstance()->RFComboxSetCurrentIdxFrmValue( this->mDataSaveBy_RComboBox, this->par->mDataSaveBy );//保存类型
    this->azi_deg_min_RDoubleSpinBox->setValue(this->par->azi_deg_min);//方位下限
    this->azi_deg_max_RDoubleSpinBox->setValue(this->par->azi_deg_max);//方位上限
    this->ele_deg_min_RDoubleSpinBox->setValue(this->par->ele_deg_min);//俯仰下限
    this->ele_deg_max_RDoubleSpinBox->setValue(this->par->ele_deg_max);//俯仰上限
    this->rootFolder_RPushButton->setToolTip( this->par->rootFolder );
    this->SignalValid = signalValid;
}
bool RWnd_DefEchoSaveConfig::getSignalValid() const
{
    return SignalValid;
}
void RWnd_DefEchoSaveConfig::setSignalValid(bool value)
{
    SignalValid = value;
}
void RWnd_DefEchoSaveConfig::RFSendSignals()
{
    if(this->getSignalValid())
    {
        emit SignalWndChanged();
    }
}
