#include "SignalCoreGlobalVal.h"
#include "SignalCoreGlobalSignal.h"
namespace SignalCore
{
    SignalCoreGlobalVal::SignalCoreGlobalVal()
    {
        mSavePcEcho = true;
        // 时域回波默认关闭，避免一开始就显著增大单点协议负载。
        mSavePcTimeEcho = false;
        mSaveMtdEcho = true;
        // 默认导出目标中心左右各 2 个距离单元，即总计 5 个距离单元。
        mExportRangeHalfWidth = 2;
        // 默认导出时域脉冲中心窗半长为 32，即总计 65 个脉冲位置。
        mExportTimePulseHalfLen = 32;
        if( QString( STR( myTargetString )) == QString( STR1( SignalCore )) )
        {
            qDebug() << "主程序";
            mIsMainProgram = true;
        }
        else
        {
            mIsMainProgram = false;
        }
        if( this->mIsMainProgram == false )
        {
            mConfigPath = QString("./config/%0_参数配置.db").arg( STR( SignalCore ) );
        }
        this->LoadConfig();
        for(auto info  : mWaveSaveLongTime.array)
        {
            this->mWaveBaseInfoLst[info] = this->mWaveBaseInfoLst.size() + 2;
        }
        this->occupySoftwareName()->value = IRadarV51::UniversalDataHeader::software_category_from_str("02B00");
        this->occupySoftwareVersionP()->fields.main_ver = 0x01;
        this->occupySoftwareVersionP()->fields.sub_ver = 15;
    }

    SignalCoreGlobalVal::~SignalCoreGlobalVal()
    {
        this->SaveConfig();
    }

    void SignalCoreGlobalVal::LoadConfig()
    {
        MacLoadConfig(PointBLH, mRadarGPS);
        MacLoadConfig(CNetworkAddr, mDataSubAddr);
        MacLoadConfig(CNetworkAddr, mDataPubAddr);
        MacLoadConfig(Common::SP_para_Extern, mThroldDebug);
//        MacFrmData( mDataSubThemeLst );
        MacFrmData( mRadarDataSaveInfo );
        MacFrmData(mWaveSaveLongTime)
        MacLoadConfig(bool, mSavePcEcho);
        MacLoadConfig(bool, mSavePcTimeEcho);
        MacLoadConfig(bool, mSaveMtdEcho);
        MacLoadConfig(int, mExportRangeHalfWidth);
        MacLoadConfig(int, mExportTimePulseHalfLen);
        if( mWaveSaveLongTime.array.size() >= 10 )
        {
            mWaveSaveLongTime.array.clear();
        }
        mRadarDataSaveInfo.mIsSaveRawData = 0; // 软件每次启动均强制关闭回波录取，避免沿用上次运行的保存状态。
        // 允许配置为 0，表示仅导出目标中心距离单元/中心脉冲位置。
        if(mExportRangeHalfWidth < 0)
        {
            mExportRangeHalfWidth = 0;
        }
        // 时域脉冲半长同样允许配置为 0，此时只导出单个脉冲切片。
        if(mExportTimePulseHalfLen < 0)
        {
            mExportTimePulseHalfLen = 0;
        }
        //
        mDataSubThemeLst.array[0].topic = "ZDP_Trks";
//        mDataSubThemeLst.array[1].topic = "ZDP_Trks_UAV";
//        mDataSubThemeLst.array[2].topic = "RadarCtrl";
        mDataSubThemeLst.array[mDataSubThemeLst.array.size()].topic = "/config/down/FF000";
        mDataSubThemeLst.array[mDataSubThemeLst.array.size()].topic = "/config/down/03C00";
        mDataSubThemeLst.array[mDataSubThemeLst.array.size()].topic = "/config/down/02B00";
        for(auto v : mDataSubThemeLst.array.values())
        {
            mThemeRecv[v.topic.toStdString()] = true;
        }
    }

    void SignalCoreGlobalVal::SaveConfig()
    {
        MacSaveConfig(PointBLH, mRadarGPS, "雷达本地GPS" );
        MacSaveConfig(CNetworkAddr, mDataSubAddr, "订阅地址" );
        MacSaveConfig(CNetworkAddr, mDataPubAddr, "发布地址" );
        MacSaveConfig(Common::SP_para_Extern, mThroldDebug, "" );
        MacToData( mDataSubThemeLst, "订阅主题" );
        MacToData( mRadarDataSaveInfo, "回波数据录取" );
        MacToData( mWaveSaveLongTime, "波形信息保存" );
        MacSaveConfig(bool, mSavePcEcho, "保存PC回波");
        MacSaveConfig(bool, mSavePcTimeEcho, "保存时域回波");
        MacSaveConfig(bool, mSaveMtdEcho, "保存MTD回波");
        MacSaveConfig(int, mExportRangeHalfWidth, "导出距离半宽");
        MacSaveConfig(int, mExportTimePulseHalfLen, "导出时域脉冲半长");
    }



    void SignalCoreGlobalVal::LoadConfigFrm(QString fileName)
    {
        if( fileName.isEmpty() == false )
        {
            QString old_path = this->mConfigPath;
            this->mConfigPath = fileName;
            this->LoadConfig();
            this->mConfigPath = old_path;
        }
    }

    void SignalCoreGlobalVal::SaveConfigAs(QString fileName)
    {
        if( fileName.isEmpty() == false )
        {
            QString old_path = this->mConfigPath;
            this->mConfigPath = fileName;
            this->SaveConfig();
            this->mConfigPath = old_path;
        }
    }

    PointBLH SignalCoreGlobalVal::radarGPS() const
    {
        return mRadarGPS;
    }

    PointBLH* SignalCoreGlobalVal::occupyRadarGPS()
    {
        return &mRadarGPS;
    }

    void SignalCoreGlobalVal::setRadarGPS(const PointBLH& radarGPS)
    {
        mRadarGPS = radarGPS;
    }

    CNetworkAddr SignalCoreGlobalVal::dataSubAddr() const
    {
        return mDataSubAddr;
    }

    CNetworkAddr* SignalCoreGlobalVal::occupyDataSubAddr()
    {
        return &this->mDataSubAddr;
    }

    void SignalCoreGlobalVal::setDataSubAddr(const CNetworkAddr& dataSubAddr)
    {
        mDataSubAddr = dataSubAddr;
    }

    CNetworkAddr SignalCoreGlobalVal::dataPubAddr() const
    {
        return mDataPubAddr;
    }

    CNetworkAddr* SignalCoreGlobalVal::occupyDataPubAddr()
    {
        return &this->mDataPubAddr;
    }

    void SignalCoreGlobalVal::setDataPubAddr(const CNetworkAddr& dataPubAddr)
    {
        mDataPubAddr = dataPubAddr;
    }

    ThemeLst SignalCoreGlobalVal::dataSubThemeLst() const
    {
        return mDataSubThemeLst;
    }

    ThemeLst* SignalCoreGlobalVal::occupyDataSubThemeLst()
    {
        return &mDataSubThemeLst;
    }

    void SignalCoreGlobalVal::setDataSubThemeLst(const ThemeLst& dataSubThemeLst)
    {
        mDataSubThemeLst = dataSubThemeLst;
    }

    bool SignalCoreGlobalVal::isThemeRecv(string theme)
    {
        if( this->mThemeRecv.contains(theme) )
        {
            return true;
        }
        return false;
    }

    QMap<WaveBaseInfo, int> SignalCoreGlobalVal::waveBaseInfoLst()
    {
        auto v = RWLock::make_write_guard(this->mWaveBaseInfoLstLocker);
        return mWaveBaseInfoLst;
    }

    void SignalCoreGlobalVal::setWaveBaseInfoLst(const QMap<WaveBaseInfo, int>& waveBaseInfoLst)
    {
        mWaveBaseInfoLst = waveBaseInfoLst;
    }

    bool SignalCoreGlobalVal::isExistWaveBaseInfo(WaveBaseInfo& info)
    {
        auto v = RWLock::make_write_guard(this->mWaveBaseInfoLstLocker);
        return this->mWaveBaseInfoLst.contains( info );
    }

    void SignalCoreGlobalVal::appendWaveInfo(WaveBaseInfo& info)
    {
        auto v = RWLock::make_write_guard(this->mWaveBaseInfoLstLocker);
        info.waveCode =  this->mWaveBaseInfoLst.size() + 2;
        this->mWaveBaseInfoLst[info] = this->mWaveBaseInfoLst.size() + 2;
        mWaveSaveLongTime.array.clear();
        for(auto v : this->mWaveBaseInfoLst.keys() )
        {
            this->mWaveSaveLongTime.append(v);
        }
        MacToData( mWaveSaveLongTime, "波形信息保存" );
    }

    void SignalCoreGlobalVal::pushEchoDataPkg(shared_ptr<RawData2SP>& ptr)
    {
        this->mEchoInfoLst.enqueue(ptr);
        if(this->mEchoInfoLst.size_approx() > 300)
        {
            auto v = QString("未处理的原始数据超过300[%0]]包，准备删除%1包\n")
                     .arg(  this->mEchoInfoLst.size_approx() )
                     .arg( this->mEchoInfoLst.size_approx() - 250 );
            qDebug() << v;
            while (this->mEchoInfoLst.size_approx() > 250)
            {
                shared_ptr<RawData2SP> p;
                if(this->mEchoInfoLst.try_dequeue(p))
                {
                    auto v = QString("未处理的原始数据超过300[%0]]包，准备删除一包\n").arg( this->mEchoInfoLst.size_approx() );
                    SaveDataAppend(v.toUtf8(), "./log/drop_dot_log.dat");
                    //                qDebug() << QString("未处理的原始数据超过300[%0]]包，准备删除一包").arg( this->mEchoInfoLst.size_approx() );
                }
            }
        }
    }

    int SignalCoreGlobalVal::popEchoDataPkg(shared_ptr<RawData2SP>& ptr)
    {
        if( this->mEchoInfoLst.size_approx() == 0 )
        {
            return 0x01;
        }
        bool flg = this->mEchoInfoLst.try_dequeue(ptr);
        if( flg == false )
        {
            return 0x02;
        }
        return 0x00;
    }


}
