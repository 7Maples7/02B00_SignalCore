#ifndef SIGNALCOREGLOBALVAL_H
#define SIGNALCOREGLOBALVAL_H

#include<VConfigSaveAndLoad.h>
#include"SignalCoreDefine.h"
#include<Singleton.h>
#include<QFileDialog>
#include<QString>
#include<DMQTopicList.h>
#include<CNetworkAddr.h>
#include<EquipmentIndex.h>
#include<GeographicCoordinateConversion.h>
#include<DefExchangeWithSP.h>
#include<concurrentqueue.h>
using namespace Common;
namespace SignalCore
{

    class SignalCoreGlobalVal : public VConfigSaveAndLoad, public Singleton<SignalCore::SignalCoreGlobalVal>
    {
        friend class Singleton;
        SignalCoreGlobalVal();
        ~SignalCoreGlobalVal();
    public:


        // VConfigSaveAndLoad interface
    public:
        virtual void LoadConfig() override;
        virtual void SaveConfig() override;

        /**
         * @brief 持久化并生效回波数据录取配置。
         * @details 先将候选配置写入当前配置数据库；仅写入成功后才替换内存中的生效配置，避免持久化失败时改变运行行为。
         * @param[in] config 已完成业务校验的完整回波数据录取配置。
         * @return 0 表示持久化并生效成功；非 0 表示数据库写入失败。
         * @ref
         * @date      2026-08-18 00:00:00
         * @author    roiade
         * @see
         * @note
         */
        int ApplyEchoDataCaptureConfig(const DefEchoSaveConfig& config);

        void LoadConfigFrm(QString fileName);
        void SaveConfigAs(QString fileName);
        // BuildStatus interface
    public:
        PointBLH radarGPS() const;
        PointBLH* occupyRadarGPS();
        void setRadarGPS(const PointBLH& radarGPS);

        CNetworkAddr dataSubAddr() const;
        CNetworkAddr* occupyDataSubAddr();
        void setDataSubAddr(const CNetworkAddr& dataSubAddr);

        CNetworkAddr dataPubAddr() const;

        CNetworkAddr* occupyDataPubAddr();
        void setDataPubAddr(const CNetworkAddr& dataPubAddr);

        ThemeLst dataSubThemeLst() const;
        ThemeLst* occupyDataSubThemeLst();
        void setDataSubThemeLst(const ThemeLst& dataSubThemeLst);

        bool isThemeRecv(string theme);
        /************************************| |************************************/
        QMap<WaveBaseInfo, int> waveBaseInfoLst();

        void setWaveBaseInfoLst(const QMap<WaveBaseInfo, int>& waveBaseInfoLst);

        bool isExistWaveBaseInfo(WaveBaseInfo& info);

        void appendWaveInfo(WaveBaseInfo& info);


        void pushEchoDataPkg(shared_ptr<RawData2SP>& ptr);

        int popEchoDataPkg(shared_ptr<RawData2SP>& ptr);

        QList<IRadar_V4_1::RadarTrkSingleTrk> getDronInfoTrks()
        {
            return mDronInfoTrks.values();
        }

    private:
        QMap<int64_t, IRadar_V4_1::RadarTrkSingleTrk> mDronInfoTrks;
        PointBLH mRadarGPS;//雷达本地GPS
        CNetworkAddr mDataSubAddr;//订阅地址
        CNetworkAddr mDataPubAddr;//发布地址
        ThemeLst     mDataSubThemeLst;//主题列表
        QMap<std::string, bool> mThemeRecv;
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(bool, IsMainProgram)
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(bool, IsUsingWindows)
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(EquipmentIndex, RadarIndex)
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(DefEchoSaveConfig, RadarDataSaveInfo)
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(Common::SP_para_Extern, ThroldDebug)
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(Common::WaveBaseInfoPkg, WaveSaveLongTime);
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(bool, SavePcEcho)
        // 是否在信号处理点协议中附带“脉压前/平台补偿后”的时域复数回波中心窗。
        // 该数据用于目标识别或离线分析，和 pcFreqData / mtdData 一样按点独立携带。
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(bool, SavePcTimeEcho)
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(bool, SaveMtdEcho)
        // 频域/时域导出共用的距离邻域半宽 N，实际导出距离单元数为 2*N+1。
        // 当目标落在距离边缘时，越界位置统一补 0。
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(int, ExportRangeHalfWidth)
        // 时域回波在脉冲维上的半长 L，实际导出脉冲数为 2*L+1。
        // 当前实现以 CPI 中心脉冲为中心构建时域中心窗，越界位置统一补 0。
        DECLARE_VARIABLE_WITH_GET_SET_OCCUPY(int, ExportTimePulseHalfLen)
        /************************************| |************************************/
        QMap<WaveBaseInfo, int > mWaveBaseInfoLst;//保存波形数据-主要给信号处理用，在程序起来时，需要对其进行初始化

        RWLock mWaveBaseInfoLstLocker;
        moodycamel::ConcurrentQueue<shared_ptr<RawData2SP> > mEchoInfoLst;//保存回波数据
    };
    typedef SignalCoreGlobalVal GlobalVal;
}
#endif // GLOBALVAL_H
