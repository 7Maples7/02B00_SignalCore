#ifndef RWND_ECHOEXPORTCONFIG_H
#define RWND_ECHOEXPORTCONFIG_H

#include <QWidget>
#include <RMModuleRewriting>
#include <Singleton.h>
#include "SignalCoreGlobalVal.h"

using namespace RObject;

namespace SignalCore
{
    class RWnd_EchoExportConfig : public QWidget
    {
        Q_OBJECT
    public:
        explicit RWnd_EchoExportConfig(QWidget* parent = nullptr);

    private:
        void variableApplication();
        void internalSignalConnect();
        void syncFromGlobal();

    private:
        RCheckBox* savePcEcho_RCheckBox = nullptr;
        RCheckBox* savePcTimeEcho_RCheckBox = nullptr;
        RCheckBox* saveMtdEcho_RCheckBox = nullptr;
        RSpinBox* exportDepth_RSpinBox = nullptr;
        RSpinBox* exportTimePulseHalfLen_RSpinBox = nullptr;
    };
}

#endif // RWND_ECHOEXPORTCONFIG_H
