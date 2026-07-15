#ifndef RWND_DEFECHOSAVECONFIG_H
#define RWND_DEFECHOSAVECONFIG_H
#include<RMModuleRewriting>
#include<QWidget>
#include"SignalCoreDefine.h"
#include<RGTimeSystem.h>
namespace SignalCore
{
    class RWnd_DefEchoSaveConfig : public QWidget
    {
        Q_OBJECT
    public:
        RWnd_DefEchoSaveConfig(DefEchoSaveConfig* par, QWidget* parent  = nullptr, QString title = "");
        bool variableApplication() ;
        bool internalSignalConnect();
        bool exec() ;
        void RFSendSignals();
        void update();
        bool getSignalValid() const;
        void setSignalValid(bool value);
    signals:
        void SignalWndChanged();
    private:
        DefEchoSaveConfig* par = nullptr;
        QString title = "";
        bool SignalValid = false;
        RCheckBox* mIsSaveRawData_RCheckBox = nullptr;//是否保存
        RComboBox* mDataSaveBy_RComboBox = nullptr;//保存类型
        RDoubleSpinBox* azi_deg_min_RDoubleSpinBox = nullptr;// 方位下限
        RDoubleSpinBox* azi_deg_max_RDoubleSpinBox = nullptr;// 方位上限
        RDoubleSpinBox* ele_deg_min_RDoubleSpinBox = nullptr;// 俯仰下限
        RDoubleSpinBox* ele_deg_max_RDoubleSpinBox = nullptr;// 俯仰上限
        RPushButton* rootFolder_RPushButton = nullptr;//数据录取根目录

    };
}
#endif
