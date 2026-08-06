#include "RadarEchoDataAdms.h"
namespace SignalCore
{
    QJsonObject toJsonObj(Attitude v)
    {
        QJsonObject obj;
        obj.insert("pitch", v.pitch);
        obj.insert("roll", v.roll);
        obj.insert("yaw", v.yaw);
        return obj;
    }

    QByteArray JsonData(QJsonObject obj)
    {
        return QJsonDocument(obj).toJson();
    }


    QJsonObject toJsonObj(DroneLocInfo v)
    {
        QJsonObject obj;
        obj.insert("type", v. type);
        obj.insert("V_x",  double(v.V_x) / 1000);
        obj.insert("V_y",  double(v.V_y) / 1000);
        obj.insert("V_z",  double(v.V_z) / 1000);
        obj.insert("lon",  double(v. lon ) / 1e10);
        obj.insert("lat",  double(v. lat ) / 1e10);
        obj.insert("high", double(v. high) / 1e3);
        obj.insert("attitudePitch", (double)v.attitudePitch / 1e6);
        obj.insert("attitudeRoll", (double)v.attitudeRoll / 1e6);
        obj.insert("attitudeYaw", (double)v.attitudeYaw / 1e6);
        obj.insert("droneNbr", v.droneNbr);
        obj.insert("bak", v.bak);
        obj.insert("bak1", v.bak1);
        return obj;
    }

    QJsonObject toJsonObj(RadarMeasureCoor5D v)
    {
        QJsonObject obj;
        obj.insert("time", v.time);
        obj.insert("rng", v.rng);
        obj.insert("azi", v.azi);
        obj.insert("ele", v.ele);
        obj.insert("vec", v.vec);
        obj.insert("amp", v.amp);
        return obj;
    }




    RadarEchoDataAdms::RadarEchoDataAdms()
    {
        this->active();
    }

    RadarEchoDataAdms::~RadarEchoDataAdms()
    {
        this->destroy();
    }

    bool RadarEchoDataAdms::variableApplication()
    {
        return true;
    }

    bool RadarEchoDataAdms::internalSignalConnect()
    {
        connect( GlobalSignal::GetInstance(), &GlobalSignal::SignalReadRawEchoData, this, &RadarEchoDataAdms::Test_forSignal);
        connect( GlobalSignal::GetInstance(), &GlobalSignal::SignalResetRawSavePath, [this]()
        {
            this->needResetSavePath = true;
        } );
        return true;
    }

    bool RadarEchoDataAdms::exec()
    {
        return true;
    }

    void RadarEchoDataAdms::destroy()
    {
        this->isStop = true;
        while (this->mIsRuning)
        {
            RGTimeSystem::GetInstance()->RFDelayMs(1);
        }
    }

    void RadarEchoDataAdms::active()
    {
        this->variableApplication();
        this->internalSignalConnect();
        this->exec();
    }



    int RadarEchoDataAdms::echoDataSaveAffterPar(
        shared_ptr< RawData2SP> raw_info,
        QList<IRadarV51::SignalProcPoint>& points)
    {
        for(auto  v : points)
        {
            if( deg2rad(std::abs(v.azimuth_array)) > M_PI * 2 || deg2rad(std::abs( v.elevation_array )) > M_PI )
            {
                qDebug() << "输出点迹方位/俯仰不对" << v.azimuth_array << v.elevation_array;
            }
        }
        IRadarV51::SignalProcPointPackageWithHeader spPointsPkg;
        for(auto v : points)
        {
            spPointsPkg.payload.points.push_back(v);
        }
//        raw_info->mSPResult = spPointsPkg;
        if(points.size() > 100)
        {
            qDebug() << "输入的信号处理点迹超过100个，将丢弃一部分;";
        }
        //判断是否需要录取
        bool is_save_flg = false;
        auto setting = GlobalVal::GetInstance()->getRadarDataSaveInfo();
        if( setting.mIsSaveRawData == 0x00 )
        {
            return 0x01;
        }
        if( setting.mDataSaveBy == 0x00 )
        {
            return 0x02;
        }
        if( setting.mDataSaveBy == 0x01 )
        {
            bool flg = setting.isAngleInScope(raw_info->head->mWaveInfo.aziFaceRad(),  raw_info->head->mWaveInfo.eleFaceRad()  );
            if( flg == false )
            {
                return 0x03;
            }
            qDebug() << "通过阵面坐标系录取数据";
        }
        if( setting.mDataSaveBy == 0x02 )
        {
            bool flg = setting.isAngleInScope(raw_info->head->mWaveInfo.aziStationRad(),  raw_info->head->mWaveInfo.eleStationRad()  );
            if( flg == false )
            {
                return 0x04;
            }
        }
        if( setting.mDataSaveBy == 0x03 )
        {
            if( raw_info->head->mTrkingInfo.Purpose != 0x11  )
            {
                return 0x05;
            }
        }
        if( setting.mDataSaveBy == 0x04 )
        {
            if( raw_info->head->mTrkingInfo.Purpose != 0x42  )
            {
                return 0x06;
            }
        }
        if( setting.mDataSaveBy == 0x05 )
        {
            if( raw_info->head->mDronInfo.Purpose != 0x13 )
            {
                return 0x07;
            }
            qDebug() << "yyyyyyyyyyyy";
        }
        if( setting.mDataSaveBy == 0x06 )
        {
            if( raw_info->head->mDronInfo.Purpose != 0x14 )
            {
                return 0x08;
            }
        }
        is_save_flg = true;
        //保存数据到内存中
        if(is_save_flg  )
        {
            RFSaveFolderInitial();
            SqlQuery query(mEchodataSavePath);
            query.transaction();
            //参数校验
            DefEchoDataSave dataSaveInfo;
            dataSaveInfo.mTimeStamp = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
            dataSaveInfo.mIdentifyNbr = raw_info->head->mIdentifyNbr;
            dataSaveInfo.mSaveID  = 0;// NOTE:=  raw_info->mIdentifyNbr;
            //界面设置的姿态角（差异）
            dataSaveInfo.mAttitudeRadarGnss  = raw_info->head->mArrayAttitude;
            //气象数据(根据时间判断使能与否)
//                    dataSaveInfo.mWeatherStationData  ;
            dataSaveInfo.mWeatherStationDataValid  = false;
            //惯导数据
            dataSaveInfo.CustomaryDerivative  = raw_info->head->mInertialNavAttitude;
            dataSaveInfo.CustomaryDerivativeValid  = true;
            //雷达位置
            dataSaveInfo.mLocalGPS = raw_info->head->mRadarCurrentGPS  ;
            //无人机数据(或ADSB信息)
            dataSaveInfo.mDronLocInfo = raw_info->head->mDronInfo ;
            dataSaveInfo.mDronLocInfoValid  = (
                                                  raw_info->head->mDronInfo.Purpose == 0x13 ||
                                                  raw_info->head->mDronInfo.Purpose == 0x14
                                              ) ;
            //跟踪信息
            dataSaveInfo.mTrkingInfo  =  raw_info->head->mTrkingInfo ;
            dataSaveInfo.mTrkingSaveValid  = (  ( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - dataSaveInfo.mTrkingInfo.Timestamp )  < 1e4);
            //类型
            dataSaveInfo.mTargetType  = 0x00;
            //
//            dataSaveInfo.mSPDotsData = raw_info->mSPResult;
            //
            EchoSaveHeader echoInfoDetail;
            echoInfoDetail.waveTypeFlg    = 0x00;
            echoInfoDetail.frequencyHz    = raw_info->head->mWaveInfo.mRadarFreq;//频率
            echoInfoDetail.ele_station    =  raw_info -> head-> mWaveInfo.mEleStationRad; //波束-站心-方位
            echoInfoDetail.azi_station    = raw_info->head->mWaveInfo.mAziStationRad;//波束-站心-方位
            echoInfoDetail.azi_array     = raw_info->head->mWaveInfo.mAziFaceRad; //波束-方位
            echoInfoDetail.ele_array      = raw_info->head->mWaveInfo.mEleFaceRad;//波束-俯仰
            echoInfoDetail.pulse_width1Us = raw_info->head->mWaveInfo.mPulseWidthUs;//脉宽
            echoInfoDetail.pulse_width2Us = raw_info->head->mWaveInfo.mPulseWidthMix;
            echoInfoDetail.prtUs       =  raw_info->head->mWaveInfo.pRTUs();
            echoInfoDetail.pulseNbr    = raw_info->head->mWaveInfo.pulseAmount();
            echoInfoDetail.sampleNbr   = raw_info->head->mWaveInfo.mSampleNbr;//采样点数量
            echoInfoDetail.dataSzie    =  raw_info->head->mWaveInfo.mSampleNbr * raw_info->head->mWaveInfo.mPulseAmount ;
            echoInfoDetail.timeStampMs = raw_info->head->mWaveInfo.mTimeStampMs;
            //
            echoInfoDetail.radarIndex = raw_info->head->radarIndex;
            echoInfoDetail.radarType = raw_info->head->radarType;
            //写入无人机数据
            //
            if( dataSaveInfo.mDronLocInfo.Purpose == 0x13 )
            {
                auto rae =  PointRAE(  dataSaveInfo.mDronLocInfo.Range,  dataSaveInfo.mDronLocInfo.Azi, dataSaveInfo.mDronLocInfo.Ele )  ;
                echoInfoDetail.mDroneLoc. rng = rae.R;
                echoInfoDetail.mDroneLoc. azi = std::round(rae.A * 180.0 / M_PI * 100) ;
                echoInfoDetail.mDroneLoc. ele = std::round(rae.E * 180.0 / M_PI * 100) ;
                double dop =  dataSaveInfo.mDronLocInfo.DopplerVelocity;
                echoInfoDetail.mDroneLoc.vec =  std::round( dop * 100);
                echoInfoDetail.mDroneLoc.valid = true;
                qDebug() <<
                         echoInfoDetail.mDroneLoc.rng <<
                         echoInfoDetail.mDroneLoc.azi <<
                         echoInfoDetail.mDroneLoc.ele <<
                         echoInfoDetail.mDroneLoc.vec;
                //
                auto xyz =   Station2Array(  RAE2xyz( rae ),   raw_info->head->mInertialNavAttitude);
                xyz =   Station2Array(  xyz,   raw_info->head->mArrayAttitude);
                rae = xyz2RAE(xyz);
                echoInfoDetail.dronAziArray = rad2deg(rae.A);
                echoInfoDetail.dronEleArray = rad2deg(rae.E);
            }
            int raw_data_size = raw_info->sizeSingalChannel() * 3;
//            echoInfoDetail.radarZTDirect = raw_info->head->mInertialNavAttitude.yaw;//转台指向
            if( raw_data_size < 20 * 1024 * 1024 )
            {
                dataSaveInfo.mRadarEchoData.append(reinterpret_cast<char*>( &echoInfoDetail ), sizeof( EchoSaveHeader ));
                /** @brief 保存回波信息   2024-02-22 22:13:38 .*/
                dataSaveInfo.mRadarEchoData.append( reinterpret_cast<char*>( raw_info->echoDataMix), raw_info->sizeSingalChannel());
                dataSaveInfo.mRadarEchoData.append( reinterpret_cast<char*>( raw_info->echoDataAzi), raw_info->sizeSingalChannel());
                dataSaveInfo.mRadarEchoData.append( reinterpret_cast<char*>( raw_info->echoDataEle), raw_info->sizeSingalChannel());
            }
            /** @brief 写入数据库   2024-05-29 10:36:39 .*/
            else
            {
                //保存回波信息到数据库
                dataSaveInfo.mRadarEchoData.append(reinterpret_cast<char*>( &echoInfoDetail ), sizeof( EchoSaveHeader ));
                //
                QString path = QString("%0/导出数据_%1/数据包_%2/%2_echo.dat").arg(  QFileInfo(mEchodataSavePath).absolutePath()   )
                               .arg(   QFileInfo(mEchodataSavePath).baseName()  )
                               .arg( dataSaveInfo.mIdentifyNbr );
                SaveDataAppend(reinterpret_cast<char*>( &echoInfoDetail ), sizeof( EchoSaveHeader ), path);
                SaveDataAppend( reinterpret_cast<char*>( raw_info->echoDataMix), raw_info->sizeSingalChannel(), path);
                SaveDataAppend( reinterpret_cast<char*>( raw_info->echoDataAzi), raw_info->sizeSingalChannel(), path);
                SaveDataAppend( reinterpret_cast<char*>( raw_info->echoDataEle), raw_info->sizeSingalChannel(), path);
            }
            sqlInsert2Table( dataSaveInfo, query, mEchodataSaveTable );
            qDebug() << "save data;";
            query.commit();
        }
        return 0x00;
    }

    int RadarEchoDataAdms::loadData4Reloay(QString path, int64_t id, char* memory, int lenMax, shared_ptr<RawData2SP>& ptr)
    {
        //            id = 846172811799126;
        SqlQuery query( path );
        QList< DefEchoDataSave> dataList;
        /// CREATE TABLE EchoDataInfo ( _mTimeStamp long long, _mIdentifyNbr long long, _mSaveID long long, _mAttitudeRadarGnss BLOB, _mWeatherStationDataValid long long, _mWeatherStationData BLOB, _CustomaryDerivativeValid long long, _CustomaryDerivative BLOB, _mLocalGPS BLOB, _mDronLocInfoValid long long, _mDronLocInfo BLOB, _mCameraImgDataValid long long, _mCameraImgDataImage BLOB, _mTrkingSaveValid long long, _mTrkingInfo BLOB, _mTargetType long long, _mFileName string, _mRadarEchoData BLOB, _mSPDotsData BLOB)
        sqlFromValue<DefEchoDataSave>( dataList, query, mEchodataSaveTable, QString("_mIdentifyNbr = %0").arg(id) );
        //
        if(dataList.size() == 0)
        {
            return 0x01;
        }
        DefEchoDataSave& echoInfo = dataList.first();
        EchoSaveHeader echoInfoDetail;
        if(  echoInfo.mRadarEchoData.size() > 0 )
        {
            echoInfoDetail = *reinterpret_cast<EchoSaveHeader*>( echoInfo.mRadarEchoData.data() );
        }
        else
        {
            QString path = QString("%0/导出数据_%1/数据包_%2/%2_echo.dat")
                           .arg(  QFileInfo(mEchodataSavePath).absolutePath()   )
                           .arg(   QFileInfo(path).baseName()  )
                           .arg( id );
            QByteArray data;
            if(LoadData(data, path))
            {
                if( data.size() < (int)sizeof( EchoSaveHeader ) )
                {
                    return 0x02;
                }
                echoInfoDetail = *reinterpret_cast<EchoSaveHeader*>(data.data());
            }
            else
            {
                return 0x03;
            }
        }
        int memoryLen =   RawData2SP::memoryLenUsed( echoInfoDetail.sampleNbr, echoInfoDetail.pulseNbr );
        if( lenMax < int(memoryLen + sizeof(RawData2SPHead)) )
        {
            return 0x04;
        }
        ptr =   RawData2SP::fromData(memory, lenMax, echoInfoDetail.sampleNbr, echoInfoDetail.pulseNbr);
        ptr->head->mWaveInfo.mRadarFreq       = echoInfoDetail.frequencyHz;
        ptr->head->mWaveInfo.mBandwidthHz     = 50 * 1e6;
        ptr->head->mWaveInfo.mSampleNbr       = echoInfoDetail.sampleNbr;
        ptr->head->mWaveInfo.mSampleFreqHz    = 50 * 1e6;
        ptr->head->mWaveInfo.mPulseWidthUs    = echoInfoDetail.pulse_width1Us;
        ptr->head->mWaveInfo.mPulseWidthMix   = echoInfoDetail.pulse_width2Us;
        ptr->head->mWaveInfo.modulatorModel   = 0;//TODO:修正混合波形
        ptr->head->mWaveInfo.modulatorRatio = 0   ;
        ptr->head->mWaveInfo.mPRTUs           = echoInfoDetail.prtUs;
        ptr->head->mWaveInfo.mPulseAmount     = echoInfoDetail.pulseNbr;
        ptr->head->mWaveInfo.mTimeStampMs     = echoInfoDetail.timeStampMs;
        ptr->head->mWaveInfo.mAziStationRad   = echoInfoDetail.azi_station;
        ptr->head->mWaveInfo.mEleStationRad   = echoInfoDetail.ele_station;
        ptr->head->mWaveInfo.mAziFaceRad      = echoInfoDetail.azi_array;
        ptr->head->mWaveInfo.mEleFaceRad      = echoInfoDetail.ele_array;
        ptr->head->mWaveInfo.mRngQuanti       = 3;
        //
        ptr->head->mArrayAttitude       = echoInfo.mAttitudeRadarGnss;
        ptr->head->mInertialNavAttitude = echoInfo.CustomaryDerivative;
        //信号处理门限
        ptr->head->mSPThrold = SignalCore::GlobalVal::GetInstance()->getThroldDebug();
        ptr->head->radarType =  echoInfoDetail.radarType ;
        ptr->head->radarIndex = echoInfoDetail.radarIndex;//TODO:需要根据雷达进行修改
        //拷贝回波
        if( echoInfo.mRadarEchoData.size() == 0 )
        {
            QString path = QString("%0/导出数据_%1/数据包_%2/%2_echo.dat")
                           .arg(  QFileInfo(mEchodataSavePath).absolutePath()   )
                           .arg(   QFileInfo(path).baseName()  )
                           .arg( id );
            QByteArray data;
            if(LoadData(data, path))
            {
                if( data.size() < int(ptr->sizeSingalChannel() * 3 + sizeof ( RawData2SPHead )) )
                {
                    return  0x05;
                }
                memcpy( (char*)ptr->echoDataMix, echoInfo.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + 0, ptr->sizeSingalChannel() );
                memcpy( (char*)ptr->echoDataAzi, echoInfo.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
                memcpy( (char*)ptr->echoDataEle, echoInfo.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + 2 * ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
            }
        }
        else
        {
            if(echoInfo.mRadarEchoData.size()  < int(ptr->sizeSingalChannel() * 3 + sizeof ( EchoSaveHeader )))
            {
                qDebug() << echoInfo.mRadarEchoData.size();
                qDebug() << ptr->sizeSingalChannel() * 3 + sizeof(EchoSaveHeader);
                qDebug() << ptr->head->mWaveInfo.toStr();
                return 0x06;
            }
            memcpy( (char*)ptr->echoDataMix, echoInfo.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + 0, ptr->sizeSingalChannel() );
            memcpy( (char*)ptr->echoDataAzi, echoInfo.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + 1 * ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
            memcpy( (char*)ptr->echoDataEle, echoInfo.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + 2 * ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
        }
        return 0x00;
    }

    QString RadarEchoDataAdms::SPPnt2Str(IRadar_V4_1::SpDataDot& pnt, bool isRadians)
    {
        QString str;
        int reservedWidth = 0;
        str += QString("[timeStamp:%0]").arg(pnt.timeStamp, 6 + reservedWidth, 10, QLatin1Char(' '));
        str += QString("[range:%0]").arg(pnt.range, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        if(isRadians)
        {
            str += QString("[azimuth:%0]").arg(pnt.azimuth * 180.0 / M_PI, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        }
        else
        {
            str += QString("[azimuth:%0]").arg(pnt.azimuth, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        }
        if(isRadians)
        {
            str += QString("[elevation:%0]").arg(pnt.elevation * 180.0 / M_PI, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        }
        else
        {
            str += QString("[elevation:%0]").arg(pnt.elevation, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        }
        str += QString("[rng_resolution:%0]").arg(pnt.rng_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[azi_resolution:%0]").arg(pnt.azi_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[ele_resolution:%0]").arg(pnt.ele_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[dop_resolution:%0]").arg(pnt.dop_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[velocity:%0]").arg(pnt.velocity, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[amplitude:%0]").arg(pnt.amplitude, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[aziRatio:%0]").arg(pnt.aziRatio, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[eleRatio:%0]").arg(pnt.eleRatio, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[FrequencyCode:%0]").arg(pnt.FrequencyCode, 6 + reservedWidth, 10, QLatin1Char(' '));
        str += QString("[I_azi:%0]").arg(pnt.I_azi, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[Q_azi:%0]").arg(pnt.Q_azi, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[I_ele:%0]").arg(pnt.I_ele, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[Q_ele:%0]").arg(pnt.Q_ele, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[RCS:%0]").arg(pnt.RCS, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[bcAzi:%0]").arg(pnt.bcAzi, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[bcEle:%0]").arg(pnt.bcEle, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[searchOrTrack:%0]").arg(pnt.searchOrTrack, 6 + reservedWidth, 10, QLatin1Char(' '));
        str += QString("[waveCode:%0]").arg(pnt.waveCode, 6 + reservedWidth, 10, QLatin1Char(' '));
        str += QString("[pw:%0]").arg(pnt.pw, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[prt:%0]").arg(pnt.prt, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
        str += QString("[prtNum:%0]").arg(pnt.prtNum, 6 + reservedWidth, 10, QLatin1Char(' '));
        return str;
    }

    void RadarEchoDataAdms::RFExtractRawData(double& rate, bool& is_running, QString path)
    {
        is_running = true;
        SqlQuery query( path );
        QList< DefEchoSave>  echoLst;
        sqlFromValue( echoLst, query, "EchoDataInfo"  );
        QDir dir = QFileInfo( path ).dir() ;
        QString folderPath = QString("%0/导出数据_%1").arg( dir.absolutePath() ).arg( QFileInfo( path ).baseName() );
        {
            double counter = 0;
            for(auto info : echoLst)
            {
                rate = counter / echoLst.size();
                counter += 1;
                QString saveFoler = QString("%0/数据包_%1").arg( folderPath  ).arg( info.mIdentifyNbr );
                dir = QDir( saveFoler );
                //基础信息
                PointBLH localGPS;
                SaveData( JsonData( info.toJsonObj()), dir.absoluteFilePath(QString("%0_基础信息数据.json").arg( info.mIdentifyNbr )) );
                //雷达姿态角
                {
                    auto* atti  = reinterpret_cast<Attitude*>( info.mAttitudeRadarGnss.data() );
                    SaveData( JsonData( toJsonObj(* atti ) ), dir.absoluteFilePath(QString("%0_雷达姿态数据.json").arg( info.mIdentifyNbr )) );
                }
                //气象
                {
                    auto* v  = reinterpret_cast<SMeteorologicalElementsSendWithTime*>( info.mWeatherStationData.data() );
                    SaveData( JsonData( toJsonObj( *v ) ), dir.absoluteFilePath(QString("%0_气象数据.json").arg( info.mIdentifyNbr )) );
                }
                //惯导
                {
                    auto* v  = reinterpret_cast<CompInertialNavDef*>( info.CustomaryDerivative.data() );
                    SaveData( JsonData( ( v->toJsonObj() ) ), dir.absoluteFilePath(QString("%0_惯导数据.json").arg( info.mIdentifyNbr )) );
                }
                //本地经纬高
                {
                    auto* v  = reinterpret_cast<PointBLH*>( info.mLocalGPS.data() );
                    localGPS = *v;
                    SaveData( JsonData( (v->toJsonObj()) ), dir.absoluteFilePath(QString("%0_本地坐标.json").arg( info.mIdentifyNbr )) );
                }
                //无人机数据(或adsb)
                {
                    auto* v  = reinterpret_cast<IRadar_V4_1::RadarTrkSingleTrk*>( info.mDronLocInfo.data() );
                    SaveData( JsonData( toJsonObj(*v) ), dir.absoluteFilePath(QString("%0_无人机数据.json").arg( info.mIdentifyNbr )) );
                }
                //跟踪数据
                {
                    auto* v  = reinterpret_cast<IRadar_V4_1::RadarTrkSingleTrk*>( info.mTrkingInfo.data() );
                    SaveData( JsonData( toJsonObj(*v) ), dir.absoluteFilePath(QString("%0_跟踪数据.json").arg( info.mIdentifyNbr )) );
                }
                //光电图片
                if( info.mCameraImgDataImage.size() > 8 )
                {
                    SaveData(  info.mCameraImgDataImage, dir.absoluteFilePath(QString("%0_光电图像.jpg").arg( info.mIdentifyNbr )) );
                }
                //波形参数
                {
                    if( info.mRadarEchoData.size() >= (int)sizeof( EchoSaveHeader ) )
                    {
                        auto* v  = reinterpret_cast<EchoSaveHeader*>( info.mRadarEchoData.data() );
                        SaveData(  v->toStr() .toUtf8(), dir.absoluteFilePath(QString("%0_波形参数.txt").arg( info.mIdentifyNbr )) );
                    }
                }
                //信号处理点迹
                //            if( info.mSPDotsData.dots.MaxSize )
                {
                    QJsonArray array;
                    for(int i = 0; i < (int)info.mSPDotsData.dots.MaxSize; i++ )
                    {
                        array.append( SPPnt2Str( info.mSPDotsData.dots.dots[i], false) + "\n");
                    }
                    SaveData( QJsonDocument( array ).toJson(), dir.absoluteFilePath(QString("%0_点迹数据.json").arg( info.mIdentifyNbr )) );
                }
                //
                //回波数据
                if(info.mRadarEchoData.size() > (int)sizeof( EchoSaveHeader ))
                {
                    SaveData(  info.mRadarEchoData, dir.absoluteFilePath( QString("%0_echo.dat").arg( info.mIdentifyNbr )) );
                }
            }
        }
        path =  dir.absolutePath() ; //获取程序当前目录
        QDesktopServices::openUrl(QUrl(QString("file:///%0").arg(path), QUrl::TolerantMode));
        is_running = false;
    }

    void RadarEchoDataAdms::RFSaveFolderInitial()
    {
        //查看根目录是否改变了
        bool need_initial = false;
        if( this->mEchodataSaveRoot != GlobalVal::GetInstance()->getRadarDataSaveInfo().rootFolder )
        {
            mEchodataSaveRoot = GlobalVal::GetInstance()->getRadarDataSaveInfo().rootFolder;
            need_initial = true;
        }
        if( GlobalVal::GetInstance()->occupyRadarDataSaveInfo()->mIsSaveRawData != this->mRawDataSaveTime &&
                GlobalVal::GetInstance()->occupyRadarDataSaveInfo()->mIsSaveRawData > 0
          )
        {
            this->mRawDataSaveTime = GlobalVal::GetInstance()->occupyRadarDataSaveInfo()->mIsSaveRawData;
            need_initial = true;
        }
        //如果文件太大，需要修改
        if( FileSizeGB( mEchodataSavePath ) >= 10 )
        {
            need_initial = true;
        }
        if( needResetSavePath )
        {
            need_initial = true;
            needResetSavePath = false;
        }
        if(need_initial)
        {
            auto dateTime = QDateTime::currentDateTime();
            if( mEchodataSaveRoot.isEmpty() == false )
            {
                mEchodataSavePath  = QString("%0/%1/%2.db").arg( mEchodataSaveRoot ).arg(dateTime.toString("yyyy_MM_dd") ).arg( dateTime.toString("yyyy_MM_dd_hh_mm_ss") );
            }
            else
            {
                mEchodataSavePath  = QString("./%0/%1.db").arg(dateTime.toString("yyyy_MM_dd") ).arg( dateTime.toString("yyyy_MM_dd_hh_mm_ss") );
            }
        }
    }

    void RadarEchoDataAdms::Test_forSignal(QString /*dataName*/, uint64_t /*dataNumber*/, uint64_t /*dataNumber_max*/)
    {
#if 0
        //初始化
        SqlQuery query(dataName );
        QString condition = QString( "_mIdentifyNbr>=%0 and _mIdentifyNbr<=%1").arg(dataNumber).arg(dataNumber_max);
        QList< DefEchoDataSave>  echoLst;//DefEchoDataSave
        sqlFromValue( echoLst, query, "EchoDataInfo", condition  );
        //参数赋值
        for(auto& info : echoLst)
        {
            EchoSaveHeader radarWaveInfo;
            shared_ptr<RawData2SP> ptr = nullptr;
            if( info.mRadarEchoData.size() == 0)
            {
                char* data = nullptr;
                QString path = QString("%0/%1/%2.dat").arg( QFileInfo(dataName).absolutePath() )
                               .arg( QFileInfo( dataName ).baseName() ).arg( info.mIdentifyNbr );
                qint64 size;
                int flg =  LoadData(data, size, path);
                if(flg != 0x00)
                {
                    qDebug() << "not exist file" << path;
                    return;
                }
                radarWaveInfo = * reinterpret_cast<EchoSaveHeader*>(data);
                ptr =  RawData2SP::_create_share( MacLoc, radarWaveInfo.sampleNbr, radarWaveInfo.pulseNbr );
                memcpy( ptr->echoDataMix, data + sizeof( EchoSaveHeader ), ptr->sizeSingalChannel() );
                memcpy( ptr->echoDataAzi, data + sizeof( EchoSaveHeader ) + ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
                memcpy( ptr->echoDataEle, data + sizeof( EchoSaveHeader ) + ptr->sizeSingalChannel() * 2, ptr->sizeSingalChannel() );
            }
            else
            {
                radarWaveInfo = * reinterpret_cast<EchoSaveHeader*>(info.mRadarEchoData.data());
                ptr =  RawData2SP::_create_share( MacLoc, radarWaveInfo. sampleNbr, radarWaveInfo.pulseNbr );
                memcpy( ptr->echoDataMix, info.mRadarEchoData.data() + sizeof( EchoSaveHeader ), ptr->sizeSingalChannel() );
                memcpy( ptr->echoDataAzi, info.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
                memcpy( ptr->echoDataEle, info.mRadarEchoData.data() + sizeof( EchoSaveHeader )  + ptr->sizeSingalChannel() * 2, ptr->sizeSingalChannel());
            }
            //check
            ptr->mWaveInfo.setPRTUs(radarWaveInfo.prtUs );
            ptr->mWaveInfo.setRadarFreq(radarWaveInfo.frequencyHz);
            ptr->mWaveInfo.setSampleNbr(radarWaveInfo.sampleNbr);
            ptr->mWaveInfo.setAziFaceRad(radarWaveInfo.azi_array);
            ptr->mWaveInfo.setEleFaceRad(radarWaveInfo.ele_array);
            ptr->mWaveInfo.setPulseAmount(radarWaveInfo.pulseNbr);
            ptr->mWaveInfo.setTimeStampMs(radarWaveInfo.timeStampMs);
            ptr->mWaveInfo.setPulseWidthUs(radarWaveInfo.pulse_width1Us);
            ptr->mWaveInfo.setBandwidthHz(30 * 1e6);
            //单独的数据 采样点脉冲数目
            //
            ptr->sampleNbr = radarWaveInfo. sampleNbr;
            ptr->pulseNbr = radarWaveInfo. pulseNbr;
            char* ptrz_echo =   info.mRadarEchoData.data();
            ptrz_echo += sizeof(EchoSaveHeader);
            ptr->mSPThrold = PGlobalVal::GetInstance()->getThrold4SP();
            //
            float pwus = radarWaveInfo. pulse_width1Us;
            float prtus = radarWaveInfo. prtUs;
            float infofs = 40.0; //40M采样频率
            float bandwidth = 30.0; //30M带宽
            ptr->hamming = SignalProcesEchoCal::GetInstance()->getHammingWndPar( radarWaveInfo. pulseNbr  );
            ptr->kaiserWnd = SignalProcesEchoCal::GetInstance()->getKaiserWndPar(radarWaveInfo. pulseNbr);
            ptr->pc_echo = SignalProcesEchoCal::GetInstance()->getPulseCompressCoef(pwus, prtus, infofs, bandwidth );
            PGlobalVal::GetInstance()->pushEchoDataPkg(ptr);
            //  RadarEchoDataAdms::GetInstance()->echoDataSaveAffterPar( ptr );//发送参数配置完成的信号
            emit PGlobalSignal::GetInstance()->SignalDataParSuccess();
            qDebug() << "mIdentifyNbr(min~max)" << dataNumber << " " << dataNumber_max;
            QThread::msleep(20);
        }
#endif
    }

    QJsonObject toJsonObj(SMeteorologicalElementsSendWithTime v)
    {
        QJsonObject obj;
        obj.insert("IdentifyNumber", (qint64)v.sixElements.IdentifyNumber);
        obj.insert("mTheTemp", v.sixElements.mTheTemp);
        obj.insert("mHumidity", v.sixElements.mHumidity);
        obj.insert("mAtmosphericPressure", v.sixElements.mAtmosphericPressure);
        obj.insert("mTheWindSpeed", v.sixElements.mTheWindSpeed);
        obj.insert("mTheDirOfTheWind", v.sixElements.mTheDirOfTheWind);
        obj.insert("mTenMinutesOfRainfall", v.sixElements.mTenMinutesOfRainfall);
        return obj;
    }

    QJsonObject toJsonObj(IRadar_V4_1::RadarTrkSingleTrk trk)
    {
        QJsonObject obj;
        obj.insert("TrackNumber", trk.TrackNumber);
        obj.insert("TrkQuality", trk.TrkQuality);
        obj.insert("Purpose", trk.Purpose);
        obj.insert("DispAttr", trk.DispAttr);
        obj.insert("SNR", trk.SNR);
        obj.insert("SCR", trk.SCR);
        obj.insert("Timestamp", trk.Timestamp);
        obj.insert("Range", trk.Range);
        obj.insert("Azi", trk.Azi);
        obj.insert("Ele", trk.Ele);
        obj.insert("DopplerVelocity", trk.DopplerVelocity);
        return obj;
    }


}
