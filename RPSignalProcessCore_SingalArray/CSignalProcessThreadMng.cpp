#include "CSignalProcessThreadMng.h"
#include <cmath>
#include <limits>
#if 0
CSignalProcessThreadMng::CSignalProcessThreadMng()
{
}
#endif

CSignalProcessThreadMng::CSignalProcessThreadMng()
{
    std::thread t(std::bind( &CSignalProcessThreadMng::active, this) );
    t.detach();
}

CSignalProcessThreadMng::~CSignalProcessThreadMng()
{
    this->destroy();
}

void CSignalProcessThreadMng::SlotWaveInfoReload()
{
    auto v = RWLock::make_write_guard(this->waveReloadCmdLocker);
    waveReloadCmd.push_back( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() );
}

void CSignalProcessThreadMng::RFWaveInfoReload()
{
    if( mSpProcCore != nullptr )
    {
        qDebug() << "delete sp core:";
        delete mSpProcCore;
        mSpProcCore = nullptr;
    }
    qDebug() << "create sp core :";
    this->mSpProcCore = new RCSignalProcessMain();
    this->mSpProcCore->setCoreIndex(0);
}

void CSignalProcessThreadMng::SlotDataParSucess()
{
    this->mIsRuning = true;
    while (this->isStop == false)
    {
        //查看是否需要回放
        if(mReplayTask.size_approx())
        {
            DefReplayInfo info;
            if( this->mReplayTask.try_dequeue(info) )
            {
                //旧版回波数据回放
                if(info.version == 0x00)
                {
                    this->RFReplayEchoFromDataOld(info);
                }
                //新版回波数据回放
                else if(info.version == 0x01)
                {
                    int flg =   this->RFReplayEchoFromData(info);
                }
            }
            continue;
        }
        //
        RGTimeSystem::GetInstance()->RFDelayMs(1);
//        //
        auto v = SPRuningFlg.get();
        SPRuningFlg.set(++v);
    }
    this->mIsRuning = false;
}

bool CSignalProcessThreadMng::variableApplication()
{
    mTimeStampStart = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
    this->SPRuningFlg.set(1);
    this->mSpProcCore = new RCSignalProcessMain();
    //
    mShareMemoryClient = CSafeShareMemoryClient::GetInstance(20000);
#if 1
    std::function<int (char*, int)> func = std::bind( &CSignalProcessThreadMng::RFReadShareMemory, this, placeholders::_1, placeholders::_2 );
#else
    std::function<int (char*, int)> func = std::bind( &CSignalProcessThreadMng::RFReadShareMemoryT, this, placeholders::_1, placeholders::_2 );
#endif
    mShareMemoryClient->setEchoDealFunc(func);
    return true;
}

bool CSignalProcessThreadMng::internalSignalConnect()
{
    connect( SignalCore::GlobalSignal::GetInstance(), &SignalCoreGlobalSignal::SignalWaveScheduleReload,
             this, &CSignalProcessThreadMng::SlotWaveInfoReload );
    connect( SignalCore::GlobalSignal::GetInstance(), &SignalCore::GlobalSignal::SignalReplayInfo, [this](QString path, int64_t id, int version)
    {
//        qDebug() << "放入原始数据-单包" << path << id << version;
        DefReplayInfo info;
        info.db_path = path;
        info.id = id;
        info.version = version;
        this->mReplayTask.enqueue( info );
    } );
    return true;
}

bool CSignalProcessThreadMng::exec()
{
    std::thread t = std::thread( std::bind(&CSignalProcessThreadMng::run, this ));
    t.detach();
    this->SlotDataParSucess();
    return true;
}

void CSignalProcessThreadMng::destroy()
{
    this->isStop = true;
    while (this->mIsRuning)
    {
        RGTimeSystem::GetInstance()->RFDelayMs(100);
    }
    if( mSpProcCore != nullptr )
    {
        delete mSpProcCore;
        mSpProcCore = nullptr;
    }
}

void CSignalProcessThreadMng::active()
{
    this->variableApplication();
    this->internalSignalConnect();
    this->exec();
}

bool CSignalProcessThreadMng::validateReceivedData(const RawData2SPHead* head, int size) const
{
    if(head == nullptr)
    {
        qWarning() << "[SignalCore] received null data header";
        return false;
    }
    if(size >= 0 && size < static_cast<int>(sizeof(RawData2SPHead)))
    {
        qWarning() << "[SignalCore] received data is shorter than header:" << size;
        return false;
    }
    const auto& waveInfo = head->mWaveInfo;
    bool valid = true;
    const auto warnInvalidPositive = [&valid](const char* name, double value, bool reject)
    {
        if(!std::isfinite(value) || value <= 0)
        {
            qWarning() << "[SignalCore] invalid waveform parameter:" << name << value;
            if(reject)
            {
                valid = false;
            }
        }
    };
    if(!std::isfinite(waveInfo.mRadarFreq) ||
            waveInfo.mRadarFreq <= 1e6 || waveInfo.mRadarFreq > 1e12)
    {
        qWarning() << "[SignalCore] invalid radar frequency (Hz):" << waveInfo.mRadarFreq;
        valid = false;
    }
    warnInvalidPositive("bandwidthHz", waveInfo.mBandwidthHz, true);
    warnInvalidPositive("sampleFreqHz", waveInfo.mSampleFreqHz, true);
    warnInvalidPositive("pulseWidthUs", waveInfo.mPulseWidthUs, true);
    warnInvalidPositive("prtUs", waveInfo.mPRTUs, true);
    const int sampleNbr = waveInfo.mSampleNbr;
    if(sampleNbr <= 0)
    {
        qWarning() << "[SignalCore] invalid sample number:" << sampleNbr;
        valid = false;
    }
    const double pulseAmount = waveInfo.mPulseAmount;
    const bool pulseAmountValid = std::isfinite(pulseAmount) &&
                                  pulseAmount >= 1 &&
                                  pulseAmount <= std::numeric_limits<int>::max() &&
                                  std::floor(pulseAmount) == pulseAmount;
    if(!pulseAmountValid)
    {
        qWarning() << "[SignalCore] invalid pulse number:" << pulseAmount;
        valid = false;
    }
    if(pulseAmountValid && sampleNbr > 0 && size >= 0)
    {
        const qint64 sampleTotal = static_cast<qint64>(sampleNbr) *
                                   static_cast<qint64>(pulseAmount);
        const qint64 bytesPerSampleTotal =
            3LL * static_cast<qint64>(sizeof(SampleComplexInt16));
        const qint64 payloadSize = static_cast<qint64>(size) - sizeof(RawData2SPHead);
        if(sampleTotal > payloadSize / bytesPerSampleTotal)
        {
            qWarning() << "[SignalCore] received data length is inconsistent with waveform parameters:"
                       << "size" << size << "sampleNbr" << sampleNbr << "pulseNbr" << pulseAmount;
            valid = false;
        }
    }
    if(waveInfo.mPRTUs > 0 && waveInfo.mPulseWidthUs > waveInfo.mPRTUs)
    {
        qWarning() << "[SignalCore] pulse width is greater than PRT:"
                   << waveInfo.mPulseWidthUs << waveInfo.mPRTUs;
        valid = false;
    }
    if(!std::isfinite(waveInfo.mPulseWidthMix) || waveInfo.mPulseWidthMix < 0)
    {
        qWarning() << "[SignalCore] invalid mixed pulse width (us):" << waveInfo.mPulseWidthMix;
    }
    if(waveInfo.mPulseWidthMix > waveInfo.mPRTUs && waveInfo.mPRTUs > 0)
    {
        qWarning() << "[SignalCore] mixed pulse width is greater than PRT:"
                   << waveInfo.mPulseWidthMix << waveInfo.mPRTUs;
    }
    if(waveInfo.mBandwidthHz > waveInfo.mSampleFreqHz && waveInfo.mSampleFreqHz > 0)
    {
        qWarning() << "[SignalCore] bandwidth is greater than sample frequency:"
                   << waveInfo.mBandwidthHz << waveInfo.mSampleFreqHz;
    }
    const auto warnAngle = [](const char* name, double value, double minValue, double maxValue)
    {
        if(!std::isfinite(value) || value < minValue || value > maxValue)
        {
            qWarning() << "[SignalCore] invalid angle (rad):" << name << value;
        }
    };
    warnAngle("aziStationRad", waveInfo.mAziStationRad, -2 * M_PI, 2 * M_PI);
    warnAngle("eleStationRad", waveInfo.mEleStationRad, -M_PI / 2, M_PI / 2);
    warnAngle("aziFaceRad", waveInfo.mAziFaceRad, -2 * M_PI, 2 * M_PI);
    warnAngle("eleFaceRad", waveInfo.mEleFaceRad, -M_PI / 2, M_PI / 2);
    if(waveInfo.mTimeStampMs <= 0)
    {
        qWarning() << "[SignalCore] waveform timestamp is not assigned:" << waveInfo.mTimeStampMs;
    }
    if(!std::isfinite(waveInfo.mRngQuanti) || waveInfo.mRngQuanti <= 0)
    {
        qWarning() << "[SignalCore] invalid range quantization:" << waveInfo.mRngQuanti;
    }
    if(!std::isfinite(waveInfo.mVecQuanti) || waveInfo.mVecQuanti <= 0)
    {
        qWarning() << "[SignalCore] invalid velocity quantization:" << waveInfo.mVecQuanti;
    }
    return valid;
}

int CSignalProcessThreadMng::RFReadShareMemory(char* data, int size)
{
    if(data == nullptr)
    {
        qDebug() << "input nullptr data:" << data << size;
        return 0x01;
    }
    RawData2SPHead* head = reinterpret_cast<RawData2SPHead*>( data );
    if(!validateReceivedData(head, size))
    {
        return 0x02;
    }
    auto ptr =  Common::RawData2SP::fromData(data, size, head->mWaveInfo.sampleNbr(), head->mWaveInfo.pulseAmount());
    if(head->mTrkingInfo.Purpose == 0x42)
    {
//        qDebug() << "识别跟踪任务信息:" << head->mTrkingInfo.toStr();
    }
    //波形检测
    if(ptr != nullptr)
    {
        data_size_recv++;
        mSpProcCore->setDataFrmPar( ptr );
        QList< IRadarV51::SignalProcPoint > points;
        int flg = 0;
        if( RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - this->mTimeStampStart >= 10  * 1e3 )
        {
            if( ptr->head->mTrkingInfo.Purpose == 0x00  )
            {
#if 0
                flg =  mSpProcCore->RFSignalProcess(points, nullptr, nullptr);
#else
                if(ptr->head->radarType == (int)Common::ParseCoreType::RadarCF_ZT)
                {
                    //ZT_CF Use New Clutter
//                    qDebug() << ptr->head->mBeamCode;
                    int azi_idx = ptr->head->mBeamCode;// int(rad2deg( ptr->head->mWaveInfo.aziStationRad()) );
                    int ele_idx =  ptr->head->mBeamCode;//int(rad2deg(ptr->head->mWaveInfo.eleStationRad()));
//                    qDebug() << azi_idx << ele_idx << ptr->head->mBeamCode;
                    flg =  mSpProcCore->RFSignalProcess(points,
                                                        SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                                                            head->arraynbr, float(azi_idx),
                                                            float(ele_idx), SignalCore::DefClutterMemoryMng::ClutterType::TStatic,
                                                            ptr->head->mWaveInfo.pulseAmount(),  ptr->head->mWaveInfo.sampleNbr()),
                                                        SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                                                            head->arraynbr, float(azi_idx),
                                                            float(ele_idx), SignalCore::DefClutterMemoryMng::ClutterType::TDynamic,
                                                            ptr->head->mWaveInfo.pulseAmount(), ptr->head->mWaveInfo.sampleNbr())
                                                       );
                }
                if(ptr->head->radarType == (int)Common::ParseCoreType::RadarCF)
                {
                    //CF Use Old Clutter
                    flg =  mSpProcCore->RFSignalProcess(points,
                                                        SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                                                            head->arraynbr, head->mWaveInfo.aziFaceRad(),
                                                            head->mWaveInfo.eleFaceRad(), SignalCore::DefClutterMemoryMng::ClutterType::TStatic,
                                                            head->mWaveInfo.pulseAmount(),  head->mWaveInfo.sampleNbr()),
                                                        SignalCore::DefClutterMemoryMng::GetInstance()->getClutter( head->arraynbr,
                                                                head->mWaveInfo.aziFaceRad(), head->mWaveInfo.eleFaceRad(),
                                                                SignalCore::DefClutterMemoryMng::ClutterType::TDynamic,
                                                                head->mWaveInfo.pulseAmount(), head->mWaveInfo.sampleNbr())
                                                       );
                }
#endif
            }
            else
            {
//                qDebug() << "跟踪波束:" << ptr->head->mTrkingInfo.Purpose;
                flg =  mSpProcCore->RFSignalProcess(points, nullptr, nullptr);
            }
            if(flg == 0x00)
            {
                data_size_send += 1;
            }
            else
            {
                qDebug() << QString::number(flg, 16);
            }
        }
        //
        for(int i = 0; i < points.size(); i++)
        {
            points[i].azimuth_array = deg2rad(points[i].azimuth_array);
            points[i].elevation_array = deg2rad(points[i].elevation_array);
            points[i].azimuth_station = deg2rad(points[i].azimuth_station);
            points[i].elevation_station = deg2rad(points[i].elevation_station);
        }
        //保存回波数据
        SignalCore::RadarEchoDataAdms::GetInstance()->echoDataSaveAffterPar( ptr, points ); //TODO:remove mark laster
        //波形检测
        SignalCore::RGCommunicateCore::GetInstance()-> RFWaveInfoCheck( ptr->head->mWaveInfo );
        RFWaveReloadCheck();
    }
    else
    {
        qDebug() << "nullptr :" << "size" << size << head->mWaveInfo.toStr();
    }
//数据发送信号处理进行处理
    return 0x00;
}

int CSignalProcessThreadMng::RFReplayEchoFromData(DefReplayInfo dataInfo)
{
    //加载数据
    int len =   512 * (1 << 20) ;
    char* data = (char*)malloc(len);
    if(data == nullptr)
    {
        return 0x01;
    }
    int flg;
    shared_ptr<Common::RawData2SP> ptr;
    flg =  SignalCore::RadarEchoDataAdms::GetInstance()->loadData4Reloay(dataInfo.db_path, dataInfo.id, data, len, ptr);
    if(flg != 0)
    {
        return (flg << 8) + 0x02;
    }
    //
    flg =   SignalCore::RGCommunicateCore::GetInstance()-> RFWaveInfoCheck( ptr->head->mWaveInfo );
    if(flg == false)
    {
        this->RFWaveInfoReload();
    }
    //
    mSpProcCore->setDataFrmPar( ptr );
    QList< IRadarV51::SignalProcPoint> points;
    if(ptr->head->radarType == (int)Common::ParseCoreType::RadarCF_ZT)
    {
        //ZT_CF Use New Clutter
        mSpProcCore->RFSignalProcess(points,
                                     SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                                         ptr->head->mBeamCode, float(ptr->head->mBeamCode),
                                         float(ptr->head->mBeamCode), SignalCore::DefClutterMemoryMng::ClutterType::TStatic,
                                         ptr->head->mWaveInfo.pulseAmount(),  ptr->head->mWaveInfo.sampleNbr()),
                                     SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                                         ptr->head->mBeamCode, float(ptr->head->mBeamCode),
                                         float(ptr->head->mBeamCode), SignalCore::DefClutterMemoryMng::ClutterType::TDynamic,
                                         ptr->head->mWaveInfo.pulseAmount(), ptr->head->mWaveInfo.sampleNbr())
                                    );
    }
    if(ptr->head->radarType == (int)Common::ParseCoreType::RadarCF)
    {
        //CF Use Old Clutter
        mSpProcCore->RFSignalProcess(points,
                                     SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                                         ptr->head->arraynbr, ptr->head->mWaveInfo.aziFaceRad(),
                                         ptr->head->mWaveInfo.eleFaceRad(), SignalCore::DefClutterMemoryMng::ClutterType::TStatic,
                                         ptr->head->mWaveInfo.pulseAmount(),  ptr->head->mWaveInfo.sampleNbr()),
                                     SignalCore::DefClutterMemoryMng::GetInstance()->getClutter( ptr->head->arraynbr,
                                             ptr->head->mWaveInfo.aziFaceRad(), ptr->head->mWaveInfo.eleFaceRad(),
                                             SignalCore::DefClutterMemoryMng::ClutterType::TDynamic,
                                             ptr->head->mWaveInfo.pulseAmount(), ptr->head->mWaveInfo.sampleNbr())
                                    );
    }
    free(data);
    return 0x00;
}

int CSignalProcessThreadMng::RFReplayEchoFromDataOld(DefReplayInfo datainfo)
{
    qDebug() << "处理数据包:" << datainfo.db_path << datainfo.id << datainfo.version;
    SqlQuery query(datainfo.db_path );
    QString condition = QString( "_mIdentifyNbr==%0").arg(datainfo.id);
    QList< DefEchoDataSave>  echoLst;
    sqlFromValue( echoLst, query, "EchoDataInfo", condition  );
    Common::RawData2SP* ptr = Common::RawData2SP::_create(MacLoc);
    EchoSaveHeader head;
    RawData2SPHead head_1;//需要配置所有相关信息
    //参数赋值
    if( echoLst.size() == 0)
    {
        char* echoDataRead = nullptr;
        qint64 echoDataSize = 0;
        LoadData(echoDataRead, echoDataSize, datainfo.db_path);
        //旧版回波数据-报头
        head = * reinterpret_cast<EchoSaveHeader*>(echoDataRead);
        //信号处理门限
        head_1.mSPThrold = SignalCore::SignalCoreGlobalVal::GetInstance()->getThroldDebug();
        //波形参数配置
        head_1.mIdentifyNbr = datainfo.id;
        head_1.mWaveInfo.setPRTUs(head.prtUs);
        head_1.mWaveInfo.setRadarFreq(head.frequencyHz);
        head_1.mWaveInfo.setSampleNbr(head.sampleNbr);
        head_1.mWaveInfo.setAziFaceRad(head.azi_array);
        head_1.mWaveInfo.setEleFaceRad(head.ele_array);
        head_1.mWaveInfo.setPulseAmount(head.pulseNbr);
        head_1.mWaveInfo.setTimeStampMs(head.timeStampMs);
        head_1.mWaveInfo.setPulseWidthUs(head.pulse_width1Us);
        head_1.mWaveInfo.setBandwidthHz(30 * 1e6);
        head_1.mWaveInfo.mSampleNbr = head. sampleNbr;
        head_1.mWaveInfo.mPulseWidthUs = head.pulse_width1Us;
        head_1.mWaveInfo.mPulseAmount = head. pulseNbr;
        ptr->head = &head_1;
        qDebug() << "提取波形信息：" << head.prtUs << head.pulse_width1Us << head.pulseNbr << head. pulseNbr;
        QByteArray echoData;
        //加载数据(从数据库或本地读取-同时考虑新版和旧版解析软件保存数据的情况)
        int lenSingalChannel = ptr->sizeSingalChannel();
        ptr->echoDataMix =  ( SampleComplexInt16* )malloc( lenSingalChannel ) ;
        ptr->echoDataAzi = ( SampleComplexInt16* )malloc( lenSingalChannel ) ;
        ptr->echoDataEle = ( SampleComplexInt16* )malloc( lenSingalChannel ) ;
        //赋值
        int sampleTotal = ptr->head->mWaveInfo.sampleNbr() * ptr->head->mWaveInfo.pulseAmount();
        float* EchoIQ = reinterpret_cast<float*>( echoDataRead + 256 );
        for(int i = 0; i < sampleTotal; i++ )
        {
            ptr->echoDataMix[i].real  = EchoIQ[2 * i   + sampleTotal * 0];
            ptr->echoDataMix[i].image = EchoIQ[2 * i + 1 + sampleTotal * 0];
            //
            ptr->echoDataAzi[i].real  = EchoIQ[2 * i   +   sampleTotal * 1];
            ptr->echoDataAzi[i].image = EchoIQ[2 * i + 1 + sampleTotal * 1];
            //
            ptr->echoDataEle[i].real  = EchoIQ[2 * i   + sampleTotal * 2];
            ptr->echoDataEle[i].image = EchoIQ[2 * i + 1 + sampleTotal * 2];
        }
    }
    else
    {
        for(auto& info : echoLst)
        {
            head = * reinterpret_cast<EchoSaveHeader*>(info.mRadarEchoData.data());
            //信号处理门限
            head_1.mSPThrold = SignalCore::SignalCoreGlobalVal::GetInstance()->getThroldDebug();
            //波形参数配置
            head_1.mIdentifyNbr = datainfo.id;
            head_1.mWaveInfo.setPRTUs(head.prtUs);
            head_1.mWaveInfo.setRadarFreq(head.frequencyHz);
            head_1.mWaveInfo.setSampleNbr(head.sampleNbr);
            head_1.mWaveInfo.setAziFaceRad(head.azi_array);
            head_1.mWaveInfo.setEleFaceRad(head.ele_array);
            head_1.mWaveInfo.setPulseAmount(head.pulseNbr);
            head_1.mWaveInfo.setTimeStampMs(head.timeStampMs);
            head_1.mWaveInfo.setPulseWidthUs(head.pulse_width1Us);
            head_1.mWaveInfo.mSampleNbr = head. sampleNbr;
            head_1.mWaveInfo.mPulseAmount = head. pulseNbr;
            head_1.mWaveInfo.mSampleFreqHz = 40 * 1e6;
            head_1.mWaveInfo.mBandwidthHz = 30 * 1e6;
            ptr->head = &head_1;
            //auto radarWaveInfo = * reinterpret_cast<EchoSaveHeader*>(info.mRadarEchoData.data());
            //auto ptr1 = Common::RawData2SP::_create_share( MacLoc, radarWaveInfo.sampleNbr, radarWaveInfo.pulseNbr );
            memcpy( ptr->echoDataMix, info.mRadarEchoData.data() + sizeof( EchoSaveHeader ), ptr->sizeSingalChannel() );
            memcpy( ptr->echoDataAzi, info.mRadarEchoData.data() + sizeof( EchoSaveHeader ) + ptr->sizeSingalChannel(), ptr->sizeSingalChannel() );
            memcpy( ptr->echoDataEle, info.mRadarEchoData.data() + sizeof( EchoSaveHeader )  + ptr->sizeSingalChannel() * 2, ptr->sizeSingalChannel());
        }
    }
    //发送信号处理执行
    std::shared_ptr<Common::RawData2SP> sharedDPtr(ptr);
    mSpProcCore->setDataFrmPar( sharedDPtr );
    QList< IRadarV51::SignalProcPoint> points;
    //波形检测 ->放入波形？
    SignalCore::RGCommunicateCore::GetInstance()-> RFWaveInfoCheck( sharedDPtr->head->mWaveInfo );
    auto flg =  mSpProcCore->RFSignalProcess(points,
                SignalCore::DefClutterMemoryMng::GetInstance()->getClutter(
                    ptr->head->arraynbr, ptr->head->mWaveInfo.aziFaceRad(),
                    ptr->head->mWaveInfo.eleFaceRad(), SignalCore::DefClutterMemoryMng::ClutterType::TStatic,
                    ptr->head->mWaveInfo.pulseAmount(),  ptr->head->mWaveInfo.sampleNbr()),
                SignalCore::DefClutterMemoryMng::GetInstance()->getClutter( ptr->head->arraynbr,
                        ptr->head->mWaveInfo.aziFaceRad(), ptr->head->mWaveInfo.eleFaceRad(),
                        SignalCore::DefClutterMemoryMng::ClutterType::TDynamic,
                        ptr->head->mWaveInfo.pulseAmount(), ptr->head->mWaveInfo.sampleNbr())
                                            );
    Q_UNUSED(flg)
    //
    return 0x00;
}

void CSignalProcessThreadMng::run()
{
    this->isStop = false;
    this->mIsRuning = true;
    int64_t tTimeCheck = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
    while (this->isStop == false)
    {
        if(RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms() - tTimeCheck >= 30000)
        {
//            if(this->SPRuningFlg.get() < 1)
//            {
//                qDebug() << "过多时间未处理数据，即将退出;";
//                emit SignalCore::GlobalSignal::GetInstance()->SignalSoftwareErrorQuit();
//                this->isStop = true;
//            }
//            else
//            {
//                this->SPRuningFlg.set(0);
//            }
            tTimeCheck = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ms();
        }
        RFReadSendSpeedCal();
        RGTimeSystem::GetInstance()->RFDelayMs(1000);
    }
    this->mIsRuning = false;
}
