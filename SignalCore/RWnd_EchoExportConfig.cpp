#include "RWnd_EchoExportConfig.h"

namespace SignalCore
{
    RWnd_EchoExportConfig::RWnd_EchoExportConfig(QWidget* parent)
        : QWidget(parent)
    {
        this->variableApplication();
        this->internalSignalConnect();
        this->syncFromGlobal();
    }

    void RWnd_EchoExportConfig::variableApplication()
    {
        RGroupBox* mainwnd = new RGroupBox("回波录取配置", this);
        RVBoxLayout* mainLayoutV = new RVBoxLayout(this);
        mainLayoutV->addWidget(mainwnd);
        mainLayoutV->addSpacerItem();

        RGridLayout* mainLayout = new RGridLayout(mainwnd);
        qint32 row = 0;
        qint32 col = 0;

        RLabel* savePcEcho_RLabel = new RLabel("录取PC回波", this);
        this->savePcEcho_RCheckBox = new RCheckBox("启用", this);
        mainLayout->addWidget(savePcEcho_RLabel, row, col);
        mainLayout->addWidget(this->savePcEcho_RCheckBox, row, col + 1, 1, 1);
        row++;

        // 时域回波对应“脉压前、平台补偿后”的复数回波中心窗。
        // 该数据通常用于目标识别、微动分析或离线重构，不直接替代脉压/MTD 数据。
        RLabel* savePcTimeEcho_RLabel = new RLabel("录取时域回波", this);
        this->savePcTimeEcho_RCheckBox = new RCheckBox("启用", this);
        mainLayout->addWidget(savePcTimeEcho_RLabel, row, col);
        mainLayout->addWidget(this->savePcTimeEcho_RCheckBox, row, col + 1, 1, 1);
        row++;

        RLabel* saveMtdEcho_RLabel = new RLabel("录取MTD回波", this);
        this->saveMtdEcho_RCheckBox = new RCheckBox("启用", this);
        mainLayout->addWidget(saveMtdEcho_RLabel, row, col);
        mainLayout->addWidget(this->saveMtdEcho_RCheckBox, row, col + 1, 1, 1);
        row++;

        RLabel* exportDepth_RLabel = new RLabel("录取距离半宽", this);
        this->exportDepth_RSpinBox = new RSpinBox(this);
        this->exportDepth_RSpinBox->setMinimum(0);
        this->exportDepth_RSpinBox->setMaximum(32);
        mainLayout->addWidget(exportDepth_RLabel, row, col);
        mainLayout->addWidget(this->exportDepth_RSpinBox, row, col + 1, 1, 1);
        row++;

        // 时域回波在脉冲维使用中心窗方式提取。
        // 例如半长=32时，实际导出长度为 65 个脉冲位置。
        RLabel* exportTimePulseHalfLen_RLabel = new RLabel("时域脉冲半长", this);
        this->exportTimePulseHalfLen_RSpinBox = new RSpinBox(this);
        this->exportTimePulseHalfLen_RSpinBox->setMinimum(0);
        this->exportTimePulseHalfLen_RSpinBox->setMaximum(512);
        mainLayout->addWidget(exportTimePulseHalfLen_RLabel, row, col);
        mainLayout->addWidget(this->exportTimePulseHalfLen_RSpinBox, row, col + 1, 1, 1);
        row++;

        RLabel* tip_RLabel = new RLabel("实际录取距离单元数 = 2 * 半宽 + 1", this);
        mainLayout->addWidget(tip_RLabel, row, col, 1, 2);
        row++;

        RLabel* timeTip_RLabel = new RLabel("实际时域脉冲长度 = 2 * 半长 + 1，越界位置补0", this);
        mainLayout->addWidget(timeTip_RLabel, row, col, 1, 2);
        row++;
    }

    void RWnd_EchoExportConfig::internalSignalConnect()
    {
        // 以下控件修改后直接写入全局配置并立即保存，
        // 这样信号处理线程在后续保点时可直接读取最新参数，无需重启程序。
        connect(this->savePcEcho_RCheckBox, &RCheckBox::clicked, [this](bool checked)
        {
            GlobalVal::GetInstance()->setSavePcEcho(checked);
            GlobalVal::GetInstance()->SaveConfig();
        });
        connect(this->savePcTimeEcho_RCheckBox, &RCheckBox::clicked, [this](bool checked)
        {
            GlobalVal::GetInstance()->setSavePcTimeEcho(checked);
            GlobalVal::GetInstance()->SaveConfig();
        });
        connect(this->saveMtdEcho_RCheckBox, &RCheckBox::clicked, [this](bool checked)
        {
            GlobalVal::GetInstance()->setSaveMtdEcho(checked);
            GlobalVal::GetInstance()->SaveConfig();
        });
        connect(this->exportDepth_RSpinBox, &RSpinBox::textChanged, [this]()
        {
            GlobalVal::GetInstance()->setExportRangeHalfWidth(this->exportDepth_RSpinBox->value());
            GlobalVal::GetInstance()->SaveConfig();
        });
        connect(this->exportTimePulseHalfLen_RSpinBox, &RSpinBox::textChanged, [this]()
        {
            GlobalVal::GetInstance()->setExportTimePulseHalfLen(this->exportTimePulseHalfLen_RSpinBox->value());
            GlobalVal::GetInstance()->SaveConfig();
        });
    }

    void RWnd_EchoExportConfig::syncFromGlobal()
    {
        this->savePcEcho_RCheckBox->setChecked(GlobalVal::GetInstance()->getSavePcEcho());
        this->savePcTimeEcho_RCheckBox->setChecked(GlobalVal::GetInstance()->getSavePcTimeEcho());
        this->saveMtdEcho_RCheckBox->setChecked(GlobalVal::GetInstance()->getSaveMtdEcho());
        this->exportDepth_RSpinBox->setValue(GlobalVal::GetInstance()->getExportRangeHalfWidth());
        this->exportTimePulseHalfLen_RSpinBox->setValue(GlobalVal::GetInstance()->getExportTimePulseHalfLen());
    }
}
