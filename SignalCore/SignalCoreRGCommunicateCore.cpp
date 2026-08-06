#include "SignalCoreRGCommunicateCore.h"
namespace SignalCore
{
    SignalCoreRGCommunicateCore::SignalCoreRGCommunicateCore()
    {
        this->active();
    }

    SignalCoreRGCommunicateCore::~SignalCoreRGCommunicateCore()
    {
        qDebug() << "SignalCoreRGCommunicateCore::~SignalCoreRGCommunicateCore() start";
        this->destroy();
        qDebug() << "SignalCoreRGCommunicateCore::~SignalCoreRGCommunicateCore() end";
    }


    void SignalCoreRGCommunicateCore::RFReadDataPragram(char* data, int size, std::string theme)
    {
        if(theme == "/config/down/ff000" || theme == "/config/down/FF000")
        {
            if( size >= (int)sizeof(IRadarV51::UniversalDataHeader) )
            {
                IRadarV51::UniversalDataHeader* header = reinterpret_cast<IRadarV51::UniversalDataHeader*>(data);
                //配置文件获取
                if( header->protocolType == (uint16_t)IRadarV51::UniversalDataHeader::CommonProtocolType::ConfigPackageDownloadQuery &&
                        header->receiverCategory.value == IRadarV51::UniversalDataHeader::software_category_from_str("ff000")
                  )
                {
                    DefConfigPkg::DefConfigSingle configUnit;
                    HANDLE_RADAR_CONFIG_DOWNLOAD_RESPONSE_V5(configUnit);//配置文件
                }
            }
        }
        if(theme == QString("/config/down/%0").arg(QString::fromStdString(GlobalVal::GetInstance()->occupySoftwareName()->fields.toString())).toStdString())
        {
            if( size >= (int)sizeof(IRadarV51::UniversalDataHeader) )
            {
                IRadarV51::UniversalDataHeader* header = reinterpret_cast<IRadarV51::UniversalDataHeader*>(data);
                HANDLE_RADAR_UNIVERSAL_V5_MSG(header, data, size);
            }
        }
    }

    int64_t SignalCoreRGCommunicateCore::RFWriteDataPragram( char* data, int size, std::string theme )
    {
        if( this->mWriteDataPragram )
        {
            this->mWriteDataPragram(data, size, theme);
        }
        if(this->mSocketPub)
        {
            this->mSocketPub->writeDatagram(data, size, theme);
        }
        return 0x00;
    }


    bool SignalCoreRGCommunicateCore::variableApplication()
    {
        this->mSocketSendPoints = new ZeromqServer();
        this->mSocketSendPoints->connect("tcp://127.0.0.1:13426");
        if(GlobalVal::GetInstance()->getIsUsingWindows() == false &&
                GlobalVal::GetInstance()->getIsMainProgram() == true    )
        {
            mSocketSub = new ZeromqSocket( );
            string addr = QString("tcp://%0:%1").arg(GlobalVal::GetInstance()->dataSubAddr().IPStr()).arg( GlobalVal::GetInstance()->dataSubAddr().port()).toStdString();
            std::list<std::string> themes;
            for(auto v : GlobalVal::GetInstance()->dataSubThemeLst().array)
            {
                themes.push_back( v.topic.toStdString() );
            }
            mSocketSub->connect( addr, themes );
            //
            addr = QString("tcp://%0:%1").arg(GlobalVal::GetInstance()->dataPubAddr().IPStr()).arg( GlobalVal::GetInstance()->dataPubAddr().port()).toStdString();
            mSocketPub = new ZeromqServer();
            mSocketPub->connect(addr);
        }
        return true;
    }


    bool SignalCoreRGCommunicateCore::internalSignalConnect()
    {
        return true;
    }


    bool SignalCoreRGCommunicateCore::exec()
    {
        std::thread t( std::bind( &SignalCoreRGCommunicateCore::run, this ) );
        t.detach();
        return true;
    }


    void SignalCoreRGCommunicateCore::destroy()
    {
        this->isStop = true;
        while (this->mIsRuning)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
        this->mSocketSendPoints->stop();
    }


    void SignalCoreRGCommunicateCore::active()
    {
        this->variableApplication();
        this->internalSignalConnect();
        this->exec();
    }


    void SignalCoreRGCommunicateCore::run()
    {
        this->isStop = false;
        this->mIsRuning = true;
        while (this->isStop == false)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
            static int64_t time4SendHeartInfo = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
            if( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - time4SendHeartInfo >= 10000 )
            {
                SEND_RADAR_HEARTBEAT_V5();
                time4SendHeartInfo = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
            }
            if(this->mSocketSub != nullptr)
            {
                while(this->mSocketSub->pendingDatagramSize())
                {
                    shared_ptr<ZmqByte> data_recv;
                    if(this->mSocketSub->readDatagram(data_recv) == 0x00)
                    {
                        this->RFReadDataPragram(data_recv->data(), data_recv->size(), data_recv->getTheme());
                    }
                }
            }
        }
//        QString headrt = "心跳发送";
        this->mIsRuning = false;
//        this->mSocketSendPoints->writeDatagram( headrt.toUtf8().data(), headrt.toUtf8().size(), "heart" );
    }

    bool SignalCoreRGCommunicateCore::RFWaveInfoCheck(DefWaveInfo& waveInfo)
    {
        Common::WaveBaseInfo info;
        info.is_mix_wave = (waveInfo.mPulseWidthMix > 1e-6);
        info.pw_us = waveInfo.pulseWidthUs();
        info.prt_us = waveInfo.pRTUs();
        info.pulseNbr = waveInfo.pulseAmount();
        info.sampleNbr = waveInfo.sampleNbr();
        info.sampleFreqMHz = waveInfo.sampleFreqHz() / 1e6;
        info.waveBandMHz = waveInfo.bandwidthHz() / 1e6;
        bool flg =  GlobalVal::GetInstance()->isExistWaveBaseInfo( info );
        if( flg == false )
        {
            GlobalVal::GetInstance()->appendWaveInfo(info);
            qDebug() << "重载波形;";
            emit GlobalSignal::GetInstance()->SignalWaveScheduleReload();
            return false;
        }
        return true;
    }

    void SignalCoreRGCommunicateCore::RFSendMicroDopplerData(DefMicroDopplerMatrix& dopCls)
    {
    }

    int SignalCoreRGCommunicateCore::RFSendPoints(shared_ptr<RawData2SP> echoInfo,
            QList<IRadarV51::SignalProcPoint>& pointsFrmSP,
            bool angleIsDeg)
    {
        if( this->isStop == true)
        {
            return 0x01;
        }
        QByteArray data;
        //插入雷达类型和雷达序号
        DefSPPointsTrancPre pre;
        pre.radarType = echoInfo->head->radarType;
        pre.radarIdx = echoInfo->head->radarIndex;
        pre.freq = echoInfo->head->mWaveInfo.mRadarFreq;
        pre.atti_diff = echoInfo->head->mArrayAttitude;
        pre.atti_gd = echoInfo->head->mInertialNavAttitude;
        pre.trkingInfo = echoInfo->head->mTrkingInfo;
        data.append( reinterpret_cast<char*>(&pre), sizeof(DefSPPointsTrancPre) );
        //
        IRadarV51::SignalProcPointPackageWithHeader pointPkg;
        for(auto v : pointsFrmSP)
        {
            if( angleIsDeg  )
            {
                v.azimuth_array = v.azimuth_array * M_PI / 180.0;
                v.elevation_array = v.elevation_array * M_PI / 180.0;
                v.beamAzimuth = deg2rad(v.beamAzimuth);
                v.beamElevation = deg2rad(v.beamElevation);
            }
//            将阵面坐标系旋转为站心坐标系
            PointRAE arrayRae(1, v.azimuth_array, v.elevation_array);
            auto station_xyz =  Array2Station( Array2Station(  RAE2xyz(arrayRae), Attitude(echoInfo->head->mArrayAttitude) * -1 ),echoInfo->head->mInertialNavAttitude * -1);
            auto station_rae = xyz2RAE( station_xyz);
            v.azimuth_station = station_rae.A;
            v.elevation_station = station_rae.E;
            v.range_station = v.range_array;//TODO
            v.cycleIndex = echoInfo->head->mCurrentCycle;
            v.beamPosIndex = echoInfo->head->mBeamCode;
            //修正跟踪搜索标志
            if( echoInfo->head->mTrkingInfo.Purpose == 0x11 )
            {
                v.pointType = echoInfo->head->mTrkingInfo.Purpose;
                v.trackId = echoInfo->head->mTrkingInfo.TrackNumber ;
            }
            else if( echoInfo->head->mTrkingInfo.Purpose == 0x42 )
            {
                v.pointType = echoInfo->head->mTrkingInfo.Purpose;
                v.trackId = echoInfo->head->mTrkingInfo.TrackNumber ;
            }
            /************************************|精度  |************************************/
            //精度
            v.azi_resolution = echoInfo->head->mWaveInfo.aziPrecisionRad();
            v.ele_resolution = echoInfo->head->mWaveInfo.elePrecisionRad();
            v.dop_resolution = echoInfo->head->mWaveInfo.mVecQuanti;
            v.rng_resolution = echoInfo->head->mWaveInfo.mRngQuanti;
            v.radarId = echoInfo->head->radarIndex;
            v.arrayId = echoInfo->head->radarIndex;
//            static int saveEchoIdx = 0;
//            SaveData(reinterpret_cast<char*>(v.mtdEcho.data()), sizeof(float)*v.mtdEcho.size(), QString("./log/row/mtd_echo_%0.dat").arg(saveEchoIdx++));
            /************************************| |************************************/
            pointPkg.payload.points.push_back(v);
        }
        bool is_send = false;
        auto dataSpPkg = pointPkg.data();
        IRadarV51::SignalProcPointPackageWithHeader test;
        int flg = test.fromData( dataSpPkg.data(), dataSpPkg.size() );
        if( flg != 0x00 )
        {
            qDebug() << "数据解析识别" << QString::number( flg, 16);
        }
        else
        {
        }
        data.append( dataSpPkg.data(), dataSpPkg.size());
//        qDebug()<<"SP POINTS";
        this->mSocketSendPoints->writeDatagram( data.data(), data.size(), "LocalPointsSend");
        is_send = true;
        if(is_send == false)
        {
            qDebug() << "发送失败";
            return 0x02;
        }
        return 0x00;
    }

    void SignalCoreRGCommunicateCore::setWriteDataPragram(const function<int64 (char* data, int size, std::string theme)>& writeDataPragram)
    {
        mWriteDataPragram = writeDataPragram;
    }

}
