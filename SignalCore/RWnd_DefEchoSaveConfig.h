#ifndef RWND_DEFECHOSAVECONFIG_H
#define RWND_DEFECHOSAVECONFIG_H
#include<RMModuleRewriting>
#include<QWidget>
#include"SignalCoreDefine.h"
#include"SignalCoreGlobalSignal.h"
#include<RGTimeSystem.h>
namespace SignalCore
{
    class RWnd_DefEchoSaveConfig : public QWidget
    {
        Q_OBJECT
    public:
        RWnd_DefEchoSaveConfig(DefEchoSaveConfig* par, QWidget* parent  = nullptr, QString title = "");
        bool variableApplication() ;
        /**
         * @brief 连接回波数据录取窗口的内部信号。
         * @details 接收全局配置刷新信号并在窗口线程更新控件，同时保留本地控件修改配置的既有连接。
         * @return true 表示信号连接已建立。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
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
