#include<math.h>
#if 1
#include "../include/RCSignalProcessMain.h"
#include"cmath"
#include<algorithm>
#include<array>
#include<cstring>
#include<QProcess>
#include <sys/resource.h>
#include <unistd.h>
#include <TargetType.h>
#include <SignalCore/SignalCoreGlobalVal.h>

namespace
{
constexpr int temporaryStcCurve = 3; // TODO: Use the STC curve reported for each echo.
constexpr int stcStartM = 54;
constexpr int stcEndM = 3000;
constexpr int stcGateSpacingM = 3;
constexpr int stcGateCount = (stcEndM - stcStartM) / stcGateSpacingM + 1;
constexpr std::array<double, 4> stcAlpha = {{1.0, 0.8, 0.6, 0.4}};
using StcRcsGainTable = std::array<double, stcGateCount>;

const StcRcsGainTable* stcRcsGainTable(int curve)
{
    if (curve < 0 || curve >= static_cast<int>(stcAlpha.size()))
        return nullptr;

    // Match STC_Model.m: quantize attenuation to 0.5 dB before computing gain.
    static const auto tables = [] {
        std::array<StcRcsGainTable, stcAlpha.size()> result{};
        const double logSpan = std::log10(double(stcEndM) / stcStartM);
        for (std::size_t curveIndex = 0; curveIndex < stcAlpha.size(); ++curveIndex)
        {
            for (int gate = 0; gate < stcGateCount; ++gate)
            {
                const double rangeM = stcStartM + gate * stcGateSpacingM;
                const double normalized = std::log10(stcEndM / rangeM) / logSpan;
                const double attenuationDb = 0.5 * std::round(60.0 * std::pow(normalized, stcAlpha[curveIndex]));
                result[curveIndex][gate] = std::pow(10.0, attenuationDb / 10.0);
            }
        }
        return result;
    }();
    return &tables[curve];
}

double stcRcsGain(const StcRcsGainTable* table, double rawRangeM)
{
    if (!table || !std::isfinite(rawRangeM) || rawRangeM <= 0.0 || rawRangeM >= stcEndM)
        return 1.0;
    const double rangeM = std::max(rawRangeM, double(stcStartM));
    const auto gate = static_cast<std::size_t>((rangeM - stcStartM) / stcGateSpacingM + 0.5);
    return (*table)[gate];
}
}

double getProcessMemMB()
{
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss / 1024.0; // ru_maxrss 单位是KB
}

#pragma pack(push, 1)
struct ExportDataHeader
{
    char magic[4];
    quint8 version;
    quint8 dataType;
    quint16 rows;
    quint16 cols;
    quint8 channels;
    quint8 reserved;
    quint32 dataCount;
};
#pragma pack(pop)

enum ExportDataType : quint8
{
    ExportDataTypePC = 1,
    ExportDataTypeMTD = 2
};


void RCSignalProcessMain::ResRelease()
{
    qDebug() << "RCSignalProcessMain_archiwave::ResRelease";
    this->thread()->quit();
    this->thread()->wait();
}
quint16 cycle_t = 0;
void debug_dotpara(IRadarV51::SignalProcPoint dot_tmp, qint32 reservedWidth = 0)
{
    QString str;
    //str += QString("[timeStamp:%0]").arg(timeStamp, 6 + reservedWidth, 10, QLatin1Char(' '));
    str += QString("[R:%0]").arg(dot_tmp.range_array, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    str += QString("[azi:%0]").arg(dot_tmp.azimuth_array, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[ele:%0]").arg(dot_tmp.elevation_array, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    //str += QString("[rng_resolution:%0]").arg(rng_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    //str += QString("[azi_resolution:%0]").arg(azi_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    //str += QString("[ele_resolution:%0]").arg(ele_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    //str += QString("[dop_resolution:%0]").arg(dop_resolution, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    str += QString("[V:%0]").arg(dot_tmp.velocity, 6 + reservedWidth, 'f', 1, QLatin1Char(' '));
    str += QString("[A:%0]").arg(dot_tmp.amplitude, 10 + reservedWidth, 'f', 0, QLatin1Char(' '));
    str += QString("[Ra:%0]").arg(dot_tmp.aziRatio, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[Re:%0]").arg(dot_tmp.eleRatio, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    //str += QString("[f:%0]").arg(FrequencyCode, 6 + reservedWidth, 10, QLatin1Char(' '));
    str += QString("[Ia:%0]").arg(dot_tmp.I_azi, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[Qa:%0]").arg(dot_tmp.Q_azi, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[Ie:%0]").arg(dot_tmp.I_ele, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[Qe:%0]").arg(dot_tmp.Q_ele, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[RCS:%0]").arg(dot_tmp.RCS, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    str += QString("[bca:%0]").arg(dot_tmp.beamAzimuth, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    str += QString("[bce:%0]").arg(dot_tmp.beamElevation, 6 + reservedWidth, 'f', 2, QLatin1Char(' '));
    //str += QString("[searchOrTrack:%0]").arg(searchOrTrack, 6 + reservedWidth, 10, QLatin1Char(' '));
    //str += QString("[waveCode:%0]").arg(waveCode, 6 + reservedWidth, 10, QLatin1Char(' '));
    //str += QString("[pw:%0]").arg(pw, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    //str += QString("[prt:%0]").arg(prt, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
    //str += QString("[prtNum:%0]").arg(prtNum, 6 + reservedWidth, 10, QLatin1Char(' '));
    qDebug() << str;
    return;
}

void RCSignalProcessMain::work(const SP_para I_SP,
                               shared_ptr<RawData2SP> I_dataFromDataPar,
                               QList<IRadarV51::SignalProcPoint>&   pointLst,
                               shared_ptr<DefClutterMemory> clutterStatic,
                               shared_ptr<DefClutterMemory> clutterDynamic)
{
    float tmpAmplitude = 0.0;
    quint32 M_Range = 0;
    int DotNum = 0;
    this->fr_para_init(I_SP, I_dataFromDataPar, wave_para_all,
                       &wave_para_current, &data_pack_para_current, &SP_para_current);
    lambda = 3e8 / data_pack_para_current.freq;
    unsigned int PulseLenMTDNum = wave_para_current.pulse_length * wave_para_current.fft_number;
    CompactDotResult compactDotsHost;
    cudaSetDevice(targetDevice);
//    qDebug() << I_dataFromDataPar->head->mWaveInfo.mAziStationRad;
    this->signalProcess(I_dataFromDataPar, clutterStatic, clutterDynamic);
    compactSignalPoints(bool_data, DianJi_para, PulseLenMTDNum,
                        compactBlockCounts, compactBlockOffsets, compactDots);
    CHECK_CUDA(cudaEventRecord(pointsReadyEvent, cudaStreamPerThread));
    CHECK_CUDA(cudaStreamWaitEvent(cudaStreamLegacy, pointsReadyEvent, 0));
    CHECK_CUDA(cudaMemcpy(&compactDotsHost, compactDots, sizeof(compactDotsHost), cudaMemcpyDeviceToHost));
    const int radarType = I_dataFromDataPar->head->radarType;
    const bool isCfRadar = radarType == (int)Common::ParseCoreType::RadarCF ||
                           radarType == (int)Common::ParseCoreType::RadarCF_ZT;
    SAmpRatioInquire* angleInquire = nullptr;
    const StcRcsGainTable* stcGainTable = nullptr;
    if(compactDotsHost.count != 0 && isCfRadar)
    {
        angleInquire = SAmpRatioInquire::GetInstance(I_dataFromDataPar->head->radarIndex);
        stcGainTable = stcRcsGainTable(temporaryStcCurve);
    }
    if(compactDotsHost.count != 0)
    {
        alpha_azi = 1.15;
        alpha_ele = 1.15;
        seitaHalfAzi = 0.88 * lambda / (32 * 17.2e-3 * cos(deg2rad(data_pack_para_current.bc_azi))) * 180 / M_PI;
        seitaHalfEle = 0.88 * lambda / (24 * 17.2e-3 * cos(deg2rad(data_pack_para_current.bc_ele))) * 180 / M_PI;
    }
    const bool savePcEcho = SignalCore::GlobalVal::GetInstance()->getSavePcEcho();
    const bool saveMtdEcho = SignalCore::GlobalVal::GetInstance()->getSaveMtdEcho();
    for (quint32 dotIndex = 0; dotIndex < compactDotsHost.count; ++dotIndex)
    {
        const quint32 i = compactDotsHost.dots[dotIndex].index;
        const DianJi_para_t& DianJi_para_h = compactDotsHost.dots[dotIndex].para;
        IRadarV51::SignalProcPoint dot_tmp;
        dot_tmp.timeStamp        = I_dataFromDataPar->head->mWaveInfo.timeStampMs();
        dot_tmp.frequency = I_dataFromDataPar->head->mWaveInfo.mRadarFreq;
        dot_tmp.amplitude        = DianJi_para_h.amplitude;
        dot_tmp.ampStaticClutter = DianJi_para_h.ampStatic_Clutter;
        dot_tmp.aziRatio         = DianJi_para_h.aziRatio;
        dot_tmp.eleRatio         = DianJi_para_h.eleRatio;
        dot_tmp.I_azi            = DianJi_para_h.I_azi;
        dot_tmp.I_ele            = DianJi_para_h.I_ele;
        dot_tmp.Q_azi            = DianJi_para_h.Q_azi;
        dot_tmp.Q_ele            = DianJi_para_h.Q_ele;
        dot_tmp.beamAzimuth      =  data_pack_para_current.bc_azi;//deg
        dot_tmp.beamElevation    = data_pack_para_current.bc_ele;//deg
        dot_tmp.waveCode         = wave_para_current.waveCode;
        dot_tmp.pw               = wave_para_current.pw_us;
        dot_tmp.prt              = wave_para_current.prt_us;
        dot_tmp.prtNum           = wave_para_current.pulse_number;
//                dot_tmp.RCS              = DianJi_para_h.RCS;
        dot_tmp.mean_col         = DianJi_para_h.mean_col;
        dot_tmp.mean_row         = DianJi_para_h.mean_row;
        dot_tmp.pointType   = I_dataFromDataPar->head->mTrkingInfo.Purpose;
        switch (I_dataFromDataPar->head->radarType)
        {
            case (int)Common::ParseCoreType::RadarXP:
                dot_tmp.velocity     = DianJi_para_h.velocity;
                dot_tmp.range_array        = DianJi_para_h.range ;
                dot_tmp.azimuth_array      = data_pack_para_current.bc_azi + dot_tmp.aziRatio * 3.61;
                dot_tmp.elevation_array    = data_pack_para_current.bc_ele - dot_tmp.eleRatio * 2.68;
                break;
            case (int)Common::ParseCoreType::RadarCF:
                dot_tmp.velocity     = -DianJi_para_h.velocity;
                dot_tmp.range_array        = DianJi_para_h.range - 27.0;
                dot_tmp.azimuth_array      = data_pack_para_current.bc_azi +
                                             angleInquire->inquireAziDeg(I_dataFromDataPar->head->mWaveInfo.mRadarFreq,
                                                     data_pack_para_current.bc_azi,
                                                     dot_tmp.aziRatio);
                dot_tmp.elevation_array    = data_pack_para_current.bc_ele +
                                             angleInquire->inquireEleDeg(I_dataFromDataPar->head->mWaveInfo.mRadarFreq,
                                                     data_pack_para_current.bc_ele,
                                                     dot_tmp.eleRatio);
                break;
            case (int)Common::ParseCoreType::RadarSG:
                qDebug() << "ku四面阵雷达测角函数";
                break;
            case (int)Common::ParseCoreType::RadarCF_ZT:
                dot_tmp.velocity     = -DianJi_para_h.velocity;
                dot_tmp.range_array        = DianJi_para_h.range - 30.0;
                dot_tmp.azimuth_array      = data_pack_para_current.bc_azi +
                                             angleInquire->inquireAziDeg(I_dataFromDataPar->head->mWaveInfo.mRadarFreq,
                                                     data_pack_para_current.bc_azi,
                                                     dot_tmp.aziRatio);
                dot_tmp.elevation_array    = data_pack_para_current.bc_ele +
                                             angleInquire->inquireEleDeg(I_dataFromDataPar->head->mWaveInfo.mRadarFreq,
                                                     data_pack_para_current.bc_ele,
                                                     dot_tmp.eleRatio);
                break;
        }
        calculateRCS(dot_tmp, stcRcsGain(stcGainTable, DianJi_para_h.range));
//                if (fillPcAndMtdExportData(dot_tmp, i, I_dataFromDataPar, DotNum))
//                {
//                    M_Range = i;
//                }
        // 将原始点迹添加到列表
        dot_tmp.MtdClsResult = IRadarV51::TargetSubTypeV5::MultiRotorUav;//TODO:后期修改为真实的识别结果
        pointLst.push_back(dot_tmp);
//                debug_dotpara(dot_tmp);
        // 将包含MTD数据的点迹添加到MTD点迹列表
        if (DianJi_para_h.amplitude > tmpAmplitude)
        {
            tmpAmplitude = DianJi_para_h.amplitude;
            M_Range = i;
        }
        DotNum++;
    }
    //目标识别处理
    if (I_dataFromDataPar->head->mTrkingInfo.Purpose == 0x42 )
    {
        qDebug() << "0x42";
        if(DotNum > 0)
        {
            //配置打出微多普勒识别的标志
            qDebug() << "Microing";
            this->processAndSendMicroDoppler(I_dataFromDataPar, M_Range, pc_data_sum, wave_para_current);
            I_dataFromDataPar->head->mTrkingInfo.bak.ClsInfo.typeMicroCls = 0x01;
        }
        else
        {
            I_dataFromDataPar->head->mTrkingInfo.bak.ClsInfo.typeMicroCls = 0x00;
        }
    }
}

void RCSignalProcessMain::calculateRCS(IRadarV51::SignalProcPoint& dot_tmp, double stcGain)
{
    Fazi_t = exp(-2.7726 * pow((dot_tmp.azimuth_array - data_pack_para_current.bc_azi) / seitaHalfAzi, 2));
    Fele_t = exp(-2.7726 * pow((dot_tmp.elevation_array - data_pack_para_current.bc_ele) / seitaHalfEle, 2));
    Fazi_r = exp(-2.7726 * pow((dot_tmp.azimuth_array - data_pack_para_current.bc_azi) / (alpha_azi * seitaHalfAzi), 2));
    Fele_r = exp(-2.7726 * pow((dot_tmp.elevation_array - data_pack_para_current.bc_ele) / (alpha_ele * seitaHalfEle), 2));
    dot_tmp.RCS = pow(dot_tmp.range_array, 4) * pow(4 * M_PI, 3) * pow(dot_tmp.amplitude / (50 * wave_para_current.pw_us) / wave_para_current.pulse_number / pow(10, 74.0 / 10.0 / 2.0) / pow(2, 15) * 1.3 * 1.4, 2) * 0.01 * 6.310 /
                  (484.57 * pow(10, (32.7221 / 10.0)) * pow(10, (31.9221 / 10.0)) * Fazi_t* Fele_t* Fazi_r * Fele_r * pow(cos(dot_tmp.azimuth_array * M_PI / 180) * cos(dot_tmp.elevation_array * M_PI / 180), 2) * lambda * lambda) * 4 * stcGain;
}

bool RCSignalProcessMain::fillPcAndMtdExportData(IRadarV51::SignalProcPoint& dot_tmp,
        quint32 pointIndex,
        shared_ptr<RawData2SP> I_dataFromDataPar,
        int DotNum,
        bool savePcEcho,
        bool saveMtdEcho)
{
    const quint32 M_Range = pointIndex;
    if(savePcEcho)
    {
        int mRange = M_Range % wave_para_current.pulse_length;
        Get_pc_data_sum(pc_data_sum, pc_data_sum_Part, mRange, wave_para_current);
        cuComplex* pc_part_cpu = (cuComplex*)malloc(wave_para_current.pulse_number * sizeof(cuComplex));
        CHECK_CUDA(cudaMemcpy(pc_part_cpu, pc_data_sum_Part, sizeof(cuComplex) * wave_para_current.pulse_number, cudaMemcpyDeviceToHost));
        dot_tmp.pcFreqData.resize(2 * 1 * wave_para_current.pulse_number);
        dot_tmp.pcFreqRows = wave_para_current.pulse_number;
        dot_tmp.pcFreqCols = 1;
        dot_tmp.pcFreqChannels = 2;
        int dataLength = wave_para_current.pulse_number;
        for (int dataIndex = 0; dataIndex < dataLength; dataIndex++)
        {
            dot_tmp.pcFreqData[dataIndex * 2]     = pc_part_cpu[dataIndex].x;
            dot_tmp.pcFreqData[dataIndex * 2 + 1] = pc_part_cpu[dataIndex].y;
        }
        free(pc_part_cpu);//2026-0626
        pc_part_cpu = nullptr;
        std::vector<char> buffer;
        buffer.insert(buffer.end(), (char*)&dot_tmp.frequency, (char*)&dot_tmp.frequency + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.amplitude, (char*)&dot_tmp.amplitude + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.pw, (char*)&dot_tmp.pw + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.prt, (char*)&dot_tmp.prt + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.prtNum, (char*)&dot_tmp.prtNum + sizeof(uint16_t));
        buffer.insert(buffer.end(), (char*)&dot_tmp.RCS, (char*)&dot_tmp.RCS + sizeof(float));
        buffer.insert(buffer.end(), (char*)dot_tmp.pcFreqData.data(), (char*)dot_tmp.pcFreqData.data() + dot_tmp.prtNum * 2 * sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.targetType, (char*)&dot_tmp.targetType + sizeof(dot_tmp.targetType));
        QDateTime currentTime = QDateTime::currentDateTime();
        QString filename = QString("./debug/datasave/dopplerTimeData%0_%1_%2.dat")
                           .arg(I_dataFromDataPar->head->mTrkingInfo.TrackNumber)
                           .arg(currentTime.toString("MM月dd日hh时mm分ss秒-zzz"))
                           .arg(DotNum);
//        SaveData( buffer.data(), buffer.size(), filename);
    }
    if(saveMtdEcho)
    {
        unsigned int mDoppler;
        mDoppler = M_Range / wave_para_current.pulse_length;
        Get_doppler_data_sum(mtd_sum_fftshift, mtd_sum_Part, mDoppler, wave_para_current);
        cuComplex* extracted_mtd_data = (cuComplex*)calloc(129, sizeof(cuComplex));
        cuComplex* mtd_part_cpu = (cuComplex*)malloc(wave_para_current.pulse_length * sizeof(cuComplex));
        CHECK_CUDA(cudaMemcpy(mtd_part_cpu, mtd_sum_Part, sizeof(cuComplex) * wave_para_current.pulse_length, cudaMemcpyDeviceToHost));
        int center = M_Range % wave_para_current.pulse_length;
        int pulse_len = wave_para_current.pulse_length;
        int start = qMax(0, center - 64);
        int end = qMin(pulse_len - 1, center + 64);
        int dst_start = start - (center - 64);
        memcpy(extracted_mtd_data + dst_start, mtd_part_cpu + start, (end - start + 1) * sizeof(cuComplex));
        int extract_length = 129;
        dot_tmp.mtdData.resize(2 * extract_length * 1);
        dot_tmp.mtdRows = 1;
        dot_tmp.mtdCols = extract_length;
        dot_tmp.mtdChannels = 2;
        int dataLength = extract_length;
        for (int dataIndex = 0; dataIndex < dataLength; dataIndex++)
        {
            dot_tmp.mtdData[dataIndex * 2]     = extracted_mtd_data[dataIndex].x;
            dot_tmp.mtdData[dataIndex * 2 + 1] = extracted_mtd_data[dataIndex].y;
        }
        free(extracted_mtd_data);//2026-0626
        free(mtd_part_cpu);
        extracted_mtd_data = nullptr;
        mtd_part_cpu = nullptr;
        std::vector<char> buffer;
        buffer.insert(buffer.end(), (char*)&dot_tmp.frequency, (char*)&dot_tmp.frequency + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.amplitude, (char*)&dot_tmp.amplitude + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.pw, (char*)&dot_tmp.pw + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.prt, (char*)&dot_tmp.prt + sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.prtNum, (char*)&dot_tmp.prtNum + sizeof(uint16_t));
        buffer.insert(buffer.end(), (char*)&dot_tmp.RCS, (char*)&dot_tmp.RCS + sizeof(float));
        buffer.insert(buffer.end(), (char*)dot_tmp.mtdData.data(), (char*)dot_tmp.mtdData.data() + dataLength * 2 * sizeof(float));
        buffer.insert(buffer.end(), (char*)&dot_tmp.targetType, (char*)&dot_tmp.targetType + sizeof(dot_tmp.targetType));
        QDateTime currentTime = QDateTime::currentDateTime();
        QString filename = QString("./debug/datasave/PcPartData%0_%1_%2.dat")
                           .arg(I_dataFromDataPar->head->mTrkingInfo.TrackNumber)
                           .arg(currentTime.toString("MM月dd日hh时mm分ss秒-zzz"))
                           .arg(DotNum);
//        SaveData( buffer.data(), buffer.size(), filename);
    }
    return savePcEcho || saveMtdEcho;
}

void RCSignalProcessMain::fr_para_init(const SP_para I_SP,
                                       shared_ptr<RawData2SP> dataFromDataPar,
                                       wave_para_t wave_para_all[],
                                       wave_para_t* wave_para_cur,
                                       data_pack_para_t* data_para_cur,
                                       sp_para_t* sp_para_cur)
{
    // 计算距离和速度分辨率
    float dr = 3e8 / dataFromDataPar->head->mWaveInfo.mSampleFreqHz / 2;
    float dv = 3e8 / dataFromDataPar->head->mWaveInfo.mRadarFreq /
               (dataFromDataPar->head->mWaveInfo.mPRTUs * 1e-6) /
               dataFromDataPar->head->mWaveInfo.mPulseAmount / 2;
    data_para_cur->freq = dataFromDataPar->head->mWaveInfo.mRadarFreq;
    WaveBaseInfo info(
        dataFromDataPar->head->mWaveInfo.pulseWidthUs(),
        dataFromDataPar->head->mWaveInfo.pRTUs(),
        dataFromDataPar->head->mWaveInfo.mPulseAmount
    );
    auto map_waveInfo = this->mWaveInfoLst;
    if(map_waveInfo.contains(info) == false)
    {
        qDebug() << "not exists wave info;";
        return;
    }
    int waveCode = map_waveInfo.value(info);
    *wave_para_cur = wave_para_all[waveCode];
    data_para_cur->bc_azi   = dataFromDataPar->head->mWaveInfo.mAziFaceRad * 180 / M_PI;
    data_para_cur->bc_ele   = dataFromDataPar->head->mWaveInfo.mEleFaceRad * 180 / M_PI;
    sp_para_cur->cfar_th_r1 = I_SP.cfar_th_r1;
    sp_para_cur->cfar_th_r2 = I_SP.cfar_th_r2;
    sp_para_cur->cfar_th_r3 = I_SP.cfar_th_r3;
    sp_para_cur->cfar_th_v1 = I_SP.cfar_th_v1;
    sp_para_cur->cfar_th_v2 = I_SP.cfar_th_v2;
    sp_para_cur->cfar_th_v3 = I_SP.cfar_th_v3;
    sp_para_cur->snr_th     = 1e3 * I_SP.snr_th;
    sp_para_cur->v_th_valid = I_SP.v_th_valid;
    sp_para_cur->r_th_valid = I_SP.r_th_valid;
    if(sp_para_cur->v_th_valid)
    {
        sp_para_cur->v_th_No_min = round(I_SP.v_min / dv);
        sp_para_cur->v_th_No_max = round(I_SP.v_max / dv);
        if(dataFromDataPar->head->mTrkingInfo.Purpose == 0x11)
        {
            //修正搜索跟踪时，对于低速悬停目标的速度门限
//            if(abs(dataFromDataPar->head->mTrkingInfo.DopplerVelocity) < 3)
//            {
//                sp_para_cur -> v_th_No_min = 0;
//            }
//            else
//            {
//                sp_para_cur -> v_th_No_min = floor(abs((abs(dataFromDataPar->head->mTrkingInfo.DopplerVelocity) - 2)) / dv);
//            }
            sp_para_cur -> v_th_No_min = 0;  //搜索跟踪时最小速度设置为0；
            sp_para_cur->v_th_No_max = ceil(abs((abs(dataFromDataPar->head->mTrkingInfo.DopplerVelocity) + 2)) / dv);
        }
        if(sp_para_cur->v_th_No_max > dataFromDataPar->head->mWaveInfo.mPulseAmount / 2)
        {
            sp_para_cur->v_th_No_max = dataFromDataPar->head->mWaveInfo.mPulseAmount / 2;
        }
    }
    if(sp_para_cur->r_th_valid)
    {
        sp_para_cur->r_th_No_min = floor(I_SP.r_min / dr);
        sp_para_cur->r_th_No_max = ceil(I_SP.r_max / dr);
        if(dataFromDataPar->head->mTrkingInfo.Purpose == 0x11 || dataFromDataPar->head->mTrkingInfo.Purpose == 0x42)
        {
            sp_para_cur->r_th_No_min = floor((dataFromDataPar->head->mTrkingInfo.Range - 100) / dr);
            sp_para_cur->r_th_No_max = ceil((dataFromDataPar->head->mTrkingInfo.Range + 100) / dr);
            //qDebug() <<dataFromDataPar->mTrkingInfo.Range<<dataFromDataPar->mTrkingInfo.DopplerVelocity<< sp_para_cur->r_th_No_min << sp_para_cur->r_th_No_max << sp_para_cur->v_th_No_min << sp_para_cur->v_th_No_max;
        }
    }
    //qDebug() << sp_para_cur->v_th_No_min << sp_para_cur->v_th_No_max<<sp_para_cur->r_th_No_min<<sp_para_cur->r_th_No_max << I_SP.v_min << I_SP.v_max << I_SP.r_min << I_SP.r_max;
}

float* RCSignalProcessMain::deviceClutter(const shared_ptr<DefClutterMemory>& clutter)
{
    constexpr size_t cacheLimit = 128u * 1024u * 1024u;
    const size_t bytes = static_cast<size_t>(clutter->size()) * sizeof(float);
    for(auto& entry : clutterCache)
    {
        if(entry.clutter == clutter)
        {
            entry.lastUsed = ++clutterCacheClock;
            return entry.deviceData;
        }
    }
    auto evictOldest = [this]()
    {
        auto oldest = std::min_element(clutterCache.begin(), clutterCache.end(),
                                       [](const ClutterCacheEntry & left, const ClutterCacheEntry & right)
        {
            return left.lastUsed < right.lastUsed;
        });
        CHECK_CUDA(cudaMemcpy(oldest->clutter->getArray(), oldest->deviceData,
                              oldest->bytes, cudaMemcpyDeviceToHost));
        CHECK_CUDA(cudaFree(oldest->deviceData));
        clutterCacheBytes -= oldest->bytes;
        clutterCache.erase(oldest);
    };
    if(bytes <= cacheLimit)
    {
        while(!clutterCache.empty() && clutterCacheBytes > cacheLimit - bytes)
        {
            evictOldest();
        }
        float* deviceData = nullptr;
        cudaError_t status = cudaMalloc(&deviceData, bytes);
        while(status == cudaErrorMemoryAllocation && !clutterCache.empty())
        {
            cudaGetLastError();
            evictOldest();
            status = cudaMalloc(&deviceData, bytes);
        }
        if(status == cudaSuccess)
        {
            CHECK_CUDA(cudaMemcpy(deviceData, clutter->getArray(), bytes, cudaMemcpyHostToDevice));
            clutterCache.push_back({clutter, deviceData, bytes, ++clutterCacheClock});
            clutterCacheBytes += bytes;
            return deviceData;
        }
        if(status != cudaErrorMemoryAllocation)
        {
            CHECK_CUDA(status);
        }
        cudaGetLastError();
    }
    CHECK_CUDA(cudaMemcpy(mtd_abs_sum_clutter, clutter->getArray(), bytes, cudaMemcpyHostToDevice));
    return mtd_abs_sum_clutter;
}

void RCSignalProcessMain::flushClutterCache()
{
    for(const auto& entry : clutterCache)
    {
        CHECK_CUDA(cudaMemcpy(entry.clutter->getArray(), entry.deviceData,
                              entry.bytes, cudaMemcpyDeviceToHost));
        CHECK_CUDA(cudaFree(entry.deviceData));
    }
    clutterCache.clear();
    clutterCacheBytes = 0;
}

void RCSignalProcessMain::signalProcess(shared_ptr<RawData2SP> I_dataFromDataPar,
                                        shared_ptr<DefClutterMemory> clutterStatic,
                                        shared_ptr<DefClutterMemory> clutterDynamic)
{
    quint32 pulse_length, fft_length, pulse_number, mtd_number;
    pulse_length = wave_para_current.pulse_length;
    fft_length = wave_para_current.fft_length;
    pulse_number = wave_para_current.pulse_number;
    mtd_number = wave_para_current.fft_number;
    double mDopcompensation = 0;
    if( I_dataFromDataPar->head->mWaveInfo.pulseAmount() != (int)pulse_number )
    {
        qDebug() << "pulse number error;" << pulse_number << I_dataFromDataPar->head->mWaveInfo.pulseAmount();
        return;
    }
    if( I_dataFromDataPar->head->mWaveInfo.sampleNbr() != (int)pulse_length )
    {
        qDebug() << "sample number error;" << I_dataFromDataPar->head->mWaveInfo.sampleNbr()  << (int)pulse_length ;
        return;
    }
    //**********************************************************************************//
    //                                      读取回波数据                                  //
    //**********************************************************************************//
    const size_t uploadBytes = sizeof(SampleComplexInt16) * static_cast<size_t>(pulse_length) * pulse_number;
#if defined (Signalprocess_Radar_Archiwave) || defined (Signalprocess_Radar_chaofeng)
    echo_data_azi_int = reinterpret_cast<short*>(reinterpret_cast<char*>(echo_data_sum_int) + uploadBytes);
    echo_data_ele_int = reinterpret_cast<short*>(reinterpret_cast<char*>(echo_data_azi_int) + uploadBytes);
    const char* hostMix = reinterpret_cast<const char*>(I_dataFromDataPar->echoDataMix);
    const char* hostAzi = reinterpret_cast<const char*>(I_dataFromDataPar->echoDataAzi);
    const bool contiguousHost = hostAzi == hostMix + uploadBytes &&
                                reinterpret_cast<const char*>(I_dataFromDataPar->echoDataEle) == hostAzi + uploadBytes;
    if(contiguousHost)
    {
        // Shared-memory slots are reused, so register each source region only once.
        auto registration = inputRegistrations.find(hostMix);
        if(pinInput && registration == inputRegistrations.end())
        {
            const long systemPageSize = sysconf(_SC_PAGESIZE);
            if(systemPageSize > 0 && maxInputBytes >= 3 * uploadBytes)
            {
                const size_t pageSize = static_cast<size_t>(systemPageSize);
                const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(hostMix);
                const std::uintptr_t aligned = address - address % pageSize;
                const size_t span = address - aligned + maxInputBytes;
                const size_t registeredBytes = (span + pageSize - 1) / pageSize * pageSize;
                void* base = reinterpret_cast<void*>(aligned);
                const cudaError_t status = cudaHostRegister(base, registeredBytes, cudaHostRegisterDefault);
                registration = inputRegistrations.emplace(hostMix,
                               HostRegistration{status == cudaSuccess ? base : nullptr,
                                                status == cudaSuccess}).first;
                if(status != cudaSuccess)
                {
                    qWarning() << "CUDA input pin failed:" << cudaGetErrorString(status);
                }
            }
        }
        CHECK_CUDA(cudaMemcpy(echo_data_sum_int, hostMix, 3 * uploadBytes, cudaMemcpyHostToDevice));
    }
    else
#endif
    {
        CHECK_CUDA(cudaMemcpy(echo_data_sum_int, I_dataFromDataPar->echoDataMix, uploadBytes, cudaMemcpyHostToDevice));
        CHECK_CUDA(cudaMemcpy(echo_data_azi_int, I_dataFromDataPar->echoDataAzi, uploadBytes, cudaMemcpyHostToDevice));
        CHECK_CUDA(cudaMemcpy(echo_data_ele_int, I_dataFromDataPar->echoDataEle, uploadBytes, cudaMemcpyHostToDevice));
    }
    CHECK_CUDA(cudaEventRecord(legacyReadyEvent, cudaStreamLegacy));
    CHECK_CUDA(cudaStreamWaitEvent(cudaStreamPerThread, legacyReadyEvent, 0));
    convertIntToCuComplex(echo_data_sum_int, echo_data_sum, wave_para_current);
    convertIntToCuComplex(echo_data_azi_int, echo_data_azi, wave_para_current);
    convertIntToCuComplex(echo_data_ele_int, echo_data_ele, wave_para_current);
    //**********************************************************************************/ /
    //                                    平台速度补偿                                     //
    //**********************************************************************************//
//    platformSpeedCompensation(echo_data_sum, echoCompensationSum, mDopcompensation, data_pack_para_current.freq, wave_para_current);
//    platformSpeedCompensation(echo_data_azi, echoCompensationAzi, mDopcompensation, data_pack_para_current.freq, wave_para_current);
//    platformSpeedCompensation(echo_data_ele, echoCompensationEle, mDopcompensation, data_pack_para_current.freq, wave_para_current);
    //**********************************************************************************/ /
    //                                       脉压                                        //
    //**********************************************************************************//
    //
#if defined (Signalprocess_Radar_Archiwave) || defined (Signalprocess_Radar_chaofeng)
    const size_t channelLength = static_cast<size_t>(fft_length) * pulse_number;
    // Each sum pointer owns a three-channel allocation; the other two are views for this wave.
    padded_echo_data_azi = padded_echo_data_sum + channelLength;
    padded_echo_data_ele = padded_echo_data_azi + channelLength;
    echo_fft_data_azi = echo_fft_data_sum + channelLength;
    echo_fft_data_ele = echo_fft_data_azi + channelLength;
    pc_fft_data_azi = pc_fft_data_sum + channelLength;
    pc_fft_data_ele = pc_fft_data_azi + channelLength;
    pc_data_azi = pc_data_sum + channelLength;
    pc_data_ele = pc_data_azi + channelLength;
    pad_pc_data_three(echo_data_sum, echo_data_azi, echo_data_ele,
                      padded_echo_data_sum, pulse_length, pulse_number, fft_length);
    pulsecompress_fft_three_gpu(padded_echo_data_sum, echo_fft_data_sum, wave_para_current);
    pulse_compress(echo_fft_data_sum, pc_fft_data_sum,
                   pc_coel_wave[wave_para_current.waveCode], 3 * pulse_number, fft_length);
    pulsecompress_ifft_three_gpu(pc_fft_data_sum, pc_data_sum, wave_para_current);
#else
    pad_pc_data((float*)echo_data_sum, (float*)padded_echo_data_sum, pulse_length, pulse_number, fft_length);
    pulsecompress_fft_gpu(padded_echo_data_sum, echo_fft_data_sum, wave_para_current);
    pulse_compress(echo_fft_data_sum, pc_fft_data_sum, pc_coel_wave[wave_para_current.waveCode], pulse_number, fft_length);
    pulsecompress_ifft_gpu(pc_fft_data_sum, pc_data_sum, wave_para_current);
    //方位差 ----------------------------------------------------------------------------------------
    pad_pc_data((float*)echo_data_azi, (float*)padded_echo_data_azi, pulse_length, pulse_number, fft_length);
    pulsecompress_fft_gpu(padded_echo_data_azi, echo_fft_data_azi, wave_para_current);
    pulse_compress(echo_fft_data_azi, pc_fft_data_azi, pc_coel_wave[wave_para_current.waveCode], pulse_number, fft_length);
    pulsecompress_ifft_gpu(pc_fft_data_azi, pc_data_azi, wave_para_current);
    //俯仰差 ----------------------------------------------------------------------------------------
    pad_pc_data((float*)echo_data_ele, (float*)padded_echo_data_ele, pulse_length, pulse_number, fft_length);
    pulsecompress_fft_gpu(padded_echo_data_ele, echo_fft_data_ele, wave_para_current);
    pulse_compress(echo_fft_data_ele, pc_fft_data_ele, pc_coel_wave[wave_para_current.waveCode], pulse_number, fft_length);
    pulsecompress_ifft_gpu(pc_fft_data_ele, pc_data_ele, wave_para_current);
#endif
    //抠除零通道的情况
    float dv = 3e8 / I_dataFromDataPar->head->mWaveInfo.mRadarFreq / (I_dataFromDataPar->head->mWaveInfo.mPRTUs * 1e-6)
               / I_dataFromDataPar->head->mWaveInfo.mPulseAmount / 2;
    if(dv <= 2 && I_dataFromDataPar->head->mTrkingInfo.Purpose != 0x11) //搜索跟踪时不扣除零通道
    {
        //**********************************************************************************//
        //                                   加权均值滤波()                                     //
        //**********************************************************************************//
        //DataWeighedMeanFil: matric pulse_number*fft_length
        SPWeighedMeanFil(pc_data_sum, DataWeighedMeanFilSum, kaiserCoel, hammingCoel, wave_para_current, fft_length, pulse_number);
        SPWeighedMeanFil(pc_data_azi, DataWeighedMeanFilAzi, kaiserCoel, hammingCoel, wave_para_current, fft_length, pulse_number);
        SPWeighedMeanFil(pc_data_ele, DataWeighedMeanFilEle, kaiserCoel, hammingCoel, wave_para_current, fft_length, pulse_number);
        //**********************************************************************************//
        //                                      MTD                                         //
        //**********************************************************************************//
        //和通道
        mtd_fft_gpu(DataWeighedMeanFilSum, mtd_sum, wave_para_current);
        //方位差
        mtd_fft_gpu(DataWeighedMeanFilAzi, mtd_azi, wave_para_current);
        //俯仰差
        mtd_fft_gpu(DataWeighedMeanFilEle, mtd_ele, wave_para_current);
        fftshift_2d_rows_cpu(mtd_sum, mtd_sum_fftshift, mtd_number, pulse_length);
        fftshift_2d_rows_cpu(mtd_azi, mtd_azi_fftshift, mtd_number, pulse_length);
        fftshift_2d_rows_cpu(mtd_ele, mtd_ele_fftshift, mtd_number, pulse_length);
        cuda_abs(mtd_sum_fftshift, mtd_abs_sum, wave_para_current, mtd_number, pulse_length);
    }
    else//不抠除零通道的情况
    {
        //**********************************************************************************//
        //                                      MTD                                         //
        //**********************************************************************************//
        //和通道
        mtd_fft_gpu(pc_data_sum, mtd_sum, wave_para_current);
        //方位差
        mtd_fft_gpu(pc_data_azi, mtd_azi, wave_para_current);
        //俯仰差
        mtd_fft_gpu(pc_data_ele, mtd_ele, wave_para_current);
        fftshift_2d_rows_cpu(mtd_sum, mtd_sum_fftshift, mtd_number, pulse_length);
        fftshift_2d_rows_cpu(mtd_azi, mtd_azi_fftshift, mtd_number, pulse_length);
        fftshift_2d_rows_cpu(mtd_ele, mtd_ele_fftshift, mtd_number, pulse_length);
        cuda_abs(mtd_sum_fftshift, mtd_abs_sum, wave_para_current, mtd_number, pulse_length);
    }
    //**********************************************************************************//
    //                                       CFAR                                       //
    //**********************************************************************************//
    CHECK_CUDA(cudaMemset(bool_data, 0, pulse_length * mtd_number * sizeof(unsigned char)));
    //分别计算距离维和速度维的参考窗的和
    calculate_cross_serve(mtd_abs_sum, cfar_para_current,
                          integral_image_row, integral_image_col,
                          pulse_length, mtd_number);
    ca_cfar_2d_cross(integral_image_row, integral_image_col,
                     mtd_abs_sum, bool_data,
                     pulse_length, mtd_number, wave_para_current,
                     cfar_para_current,
                     SP_para_current);
    //点迹凝聚
    plot_centroid_neighbour(bool_data, mtd_abs_sum, pulse_length, mtd_number);
    //**********************************************************************************//
    //               判断目标是否过杂波图 + 测角\输出点迹 + 杂波图更新                         //
    //**********************************************************************************//
//    qDebug() <<I_dataFromDataPar->head->mTrkingInfo.Purpose;
    if(I_dataFromDataPar->head->mTrkingInfo.Purpose == 0x00)
    {
//        qDebug() << I_dataFromDataPar->head->mTrkingInfo.Purpose;
        float* clutterDeviceData = deviceClutter(clutterStatic);
        SPCalculateCeilPara(mtd_abs_sum, integral_image_row, integral_image_col,
                            mtd_sum_fftshift, mtd_azi_fftshift, mtd_ele_fftshift,
                            DianJi_para, pulse_length, mtd_number, wave_para_current, SP_para_current,
                            data_pack_para_current, bool_data, clutterDeviceData);
        DefClutterMemoryUpdate_cpu(mtd_abs_sum, clutterDeviceData, pulse_number, pulse_length, clutterStatic->rateWeight);
        if(clutterDeviceData == mtd_abs_sum_clutter)
        {
            CHECK_CUDA(cudaMemcpy(clutterStatic->getArray(), clutterDeviceData,
                                  clutterStatic->size() * sizeof(float), cudaMemcpyDeviceToHost));
        }
    }
    else
    {
        CHECK_CUDA(cudaMemset(mtd_abs_sum_clutter, 0, sizeof(float) * mtd_number * pulse_length));
        SPCalculateCeilPara(mtd_abs_sum, integral_image_row, integral_image_col,
                            mtd_sum_fftshift, mtd_azi_fftshift, mtd_ele_fftshift,
                            DianJi_para, pulse_length, mtd_number, wave_para_current, SP_para_current,
                            data_pack_para_current, bool_data, mtd_abs_sum_clutter);
    }
    CHECK_CUDA(cudaEventRecord(legacyReadyEvent, cudaStreamLegacy));
    CHECK_CUDA(cudaStreamWaitEvent(cudaStreamPerThread, legacyReadyEvent, 0));
}

void RCSignalProcessMain::wave_para_init_archiwave(wave_para_t* wave_para_p, const QMap<WaveBaseInfo, int>& wave_map)
{
    for(auto info : wave_map.keys())
    {
        int waveCode = wave_map[info];
        wave_para_p[waveCode].waveCode = waveCode;
        wave_para_p[waveCode].pulse_length = info.sampleNbr;//TODO:这个参数如何设置，表示什么
        wave_para_p[waveCode].fft_length = roundf(info.prt_us * info.sampleFreqMHz);
        wave_para_p[waveCode].pulse_number = info.pulseNbr;
        wave_para_p[waveCode].fft_number = info.pulseNbr;
        wave_para_p[waveCode].prt_us = info.prt_us;
        wave_para_p[waveCode].pw_us = info.pw_us;
        wave_para_p[waveCode].waveBandMHz = info.waveBandMHz;
        wave_para_p[waveCode].sampleFreqMHz = info.sampleFreqMHz;
    }
}
int RCSignalProcessMain::RFSignalProcess(
    QList<IRadarV51::SignalProcPoint>& points,
    shared_ptr<DefClutterMemory> clutterStatic,
    shared_ptr<DefClutterMemory> clutterDynamic)
{
    if( this->mDataFrmPar == nullptr )
    {
        return 0x01;
    }
//    SignalCore::RGCommunicateCore::GetInstance()->RFSendPoints( this->mDataFrmPar, points, true );
//    return 0x00;
    cudaSetDevice(targetDevice);
    WaveBaseInfo info(this->mDataFrmPar->head->mWaveInfo.pulseWidthUs(),
                      this->mDataFrmPar->head->mWaveInfo.pRTUs(),
                      this->mDataFrmPar->head->mWaveInfo.pulseAmount());
    if(!this->mWaveInfoLst.contains(info))
    {
        qDebug() << this->mDataFrmPar->head->mWaveInfo.pulseWidthUs() << this->mDataFrmPar->head->mWaveInfo.pRTUs() << this->mDataFrmPar->head->mWaveInfo.pulseAmount();
        return 0x02;
    }
    QMap<qint32, SP_para_Extern> spPara;
    spPara[0] =  this->mDataFrmPar->head->mSPThrold;// SPGlobalVar::GetInstance()->occupySPParSearchThrold()->throld[0];//RGGlobalVar::GetInstance()->getSPParSearchSigfar();
    SP_para par;
    par.cfar_th_r1 = spPara[0].cfar_th_range_1;
    par.cfar_th_r2 = spPara[0].cfar_th_range_2;
    par.cfar_th_r3 = spPara[0].cfar_th_range_3;
    par.cfar_th_v1 = spPara[0].cfar_th_Doppler_1;
    par.cfar_th_v2 = spPara[0].cfar_th_Doppler_2;
    par.cfar_th_v3 = spPara[0].cfar_th_Doppler_3;
    par.snr_th     = spPara[0].snr_th;
    par.r_th_valid = spPara[0].r_th_valid;
    par.v_th_valid = spPara[0].v_th_valid;
    par.r_min      = spPara[0].r_min;
    par.r_max      = spPara[0].r_max;
    par.v_min      = spPara[0].v_min;
    par.v_max      = spPara[0].v_max;
    // 用于存储信号处理后的点迹数据
    this->work(par, this->mDataFrmPar, points, clutterStatic, clutterDynamic);
    // 发送信号处理点迹数据到通信核心
    auto flg =  SignalCore::RGCommunicateCore::GetInstance()->RFSendPoints( this->mDataFrmPar, points, true );
    // 将信号处理点迹数据赋值给输出参数
    // 将包含MTD数据的点迹数据赋值给输出参数
    if(flg != 0x00)
    {
        return 0x03;
    }
    return 0x00;
}

void RCSignalProcessMain::setDataFrmPar(const shared_ptr<RawData2SP>& dataFrmPar)
{
    mDataFrmPar = dataFrmPar;
}

int RCSignalProcessMain::coreIndex() const
{
    return mCoreIndex;
}

int RCSignalProcessMain::SignalCoreRunCounter()
{
    int counter = 0;
    QString  abslutPath = "SignalCore";
#ifdef linux
    QProcess process;
    QStringList arguments;
    arguments << QString("-c");  // 需要加个-c参数命令才能执行
    QString cmd = QString(" ps -eo lstart,cmd | grep %0 | grep -v grep").arg( abslutPath );
    arguments << cmd;
    process.start("bash", arguments);
    process.waitForStarted();
    process.waitForFinished();
    QString result = QString::fromLocal8Bit(process.readAllStandardOutput());
    QStringList resultLst = result.split("\n");
    for(auto v : resultLst)
    {
        if(v.size() < 2)
        {
            continue;
        }
        counter++;
    }
#endif
    return counter;
}
void RCSignalProcessMain::setCoreIndex(int coreIndex)
{
    mCoreIndex = coreIndex;
}
void RCSignalProcessMain::destroy()
{
    qDebug() << "RCSignalProcessMain::destroy() start";
    auto v = RWLock::make_write_guard(this->mInitialFlgLock);
    if( (this->initialFlg & 0b111) != 0b111)
    {
        qDebug() << "is not initial over" << QString::number( this->initialFlg, 2 );
        return;
    }
    this->isStop = true;
    while (this->mIsRuning)
    {
        qDebug() << "is running ...";
        RGTimeSystem::GetInstance()->RFDelayMs(10);
    }
    CHECK_CUDA(cudaSetDevice(targetDevice));
    flushClutterCache();
    for(const auto& entry : inputRegistrations)
    {
        if(entry.second.pinned)
        {
            CHECK_CUDA(cudaHostUnregister(entry.second.base));
        }
    }
    inputRegistrations.clear();
    CHECK_CUDA(cudaEventDestroy(pointsReadyEvent));
    CHECK_CUDA(cudaEventDestroy(legacyReadyEvent));
    //清除handle缓存
    destroyCuHandle();
    //清除capon invert var
    //destroyCaponInvertVar();
    free(kaiserCoel );
    free(hammingCoel);
    free(wave_para_all);
    int size = this->mWaveInfoLst.size();
    if( this->mWaveInfoLst.size() )
    {
        size = 1 + *std::max_element( this->mWaveInfoLst.begin(), this->mWaveInfoLst.end() );
    }
    else
    {
        size = 1;
    }
    if(pc_coel_wave != NULL)
    {
        for( auto info : this->mWaveInfoLst.keys())
        {
            int i = this->mWaveInfoLst[info];
            //脉压系数
            if(pc_coel_wave[i] != NULL)
            {
                CHECK_CUDA(cudaFree(pc_coel_wave[i]));
            }
        }
        free(pc_coel_wave);
    }
    //回波
    CHECK_CUDA(cudaFree(echo_data_sum));
    CHECK_CUDA(cudaFree(echo_data_azi));
    CHECK_CUDA(cudaFree(echo_data_ele));
    CHECK_CUDA(cudaFree(echo_data_sum_int));
#if !defined (Signalprocess_Radar_Archiwave) && !defined (Signalprocess_Radar_chaofeng)
    CHECK_CUDA(cudaFree(echo_data_azi_int));
    CHECK_CUDA(cudaFree(echo_data_ele_int));
#endif
    //平台速度补偿后回波
    CHECK_CUDA(cudaFree(echoCompensationSum));
    CHECK_CUDA(cudaFree(echoCompensationAzi));
    CHECK_CUDA(cudaFree(echoCompensationEle));
    //脉压
    CHECK_CUDA(cudaFree(padded_echo_data_sum));
#if !defined (Signalprocess_Radar_Archiwave) && !defined (Signalprocess_Radar_chaofeng)
    CHECK_CUDA(cudaFree(padded_echo_data_azi));
    CHECK_CUDA(cudaFree(padded_echo_data_ele));
#endif
    CHECK_CUDA(cudaFree(echo_fft_data_sum));
#if !defined (Signalprocess_Radar_Archiwave) && !defined (Signalprocess_Radar_chaofeng)
    CHECK_CUDA(cudaFree(echo_fft_data_azi));
    CHECK_CUDA(cudaFree(echo_fft_data_ele));
#endif
    CHECK_CUDA(cudaFree(pc_fft_data_sum));
#if !defined (Signalprocess_Radar_Archiwave) && !defined (Signalprocess_Radar_chaofeng)
    CHECK_CUDA(cudaFree(pc_fft_data_azi));
    CHECK_CUDA(cudaFree(pc_fft_data_ele));
#endif
    CHECK_CUDA(cudaFree(pc_data_sum));
#if !defined (Signalprocess_Radar_Archiwave) && !defined (Signalprocess_Radar_chaofeng)
    CHECK_CUDA(cudaFree(pc_data_azi));
    CHECK_CUDA(cudaFree(pc_data_ele));
#endif
    //
    CHECK_CUDA(cudaFree(DataWeighedMeanFilSum));
    CHECK_CUDA(cudaFree(DataWeighedMeanFilAzi));
    CHECK_CUDA(cudaFree(DataWeighedMeanFilEle));
    //MTD
    CHECK_CUDA(cudaFree(mtd_sum));
    CHECK_CUDA(cudaFree(mtd_azi));
    CHECK_CUDA(cudaFree(mtd_ele));
    CHECK_CUDA(cudaFree(mtd_sum_fftshift));
    CHECK_CUDA(cudaFree(mtd_azi_fftshift));
    CHECK_CUDA(cudaFree(mtd_ele_fftshift));
    CHECK_CUDA(cudaFree(mtd_abs_sum));
    CHECK_CUDA(cudaFree(mtd_abs_sum_clutter));
    //CFAR
    CHECK_CUDA(cudaFree(integral_image_row));
    CHECK_CUDA(cudaFree(integral_image_col));
    CHECK_CUDA(cudaFree(bool_data));
    CHECK_CUDA(cudaFree(DianJi_para));
    CHECK_CUDA(cudaFree(compactBlockCounts));
    CHECK_CUDA(cudaFree(compactBlockOffsets));
    CHECK_CUDA(cudaFree(compactDots));
    //CHECK_CUDA(cudaFree(kaiserCoel_512));
    //CHECK_CUDA(cudaFree(hammingCoel_512));
    CHECK_CUDA(cudaFree(pc_data_sum_Part));
    CHECK_CUDA(cudaFree(mtd_sum_Part));
    qDebug() << "SP STATUS: finished !";
    qDebug() << "RCSignalProcessMain::destroy() over";
}
void RCSignalProcessMain::active()
{
    qDebug() << "RCSignalProcessMain::active - var";
    this->isStop = false;
    //根据信号处理核数量选择GPU
    signalcore = SignalCoreRunCounter();
    QList<GPUInfo>gpuInfoList;
    int flg =  GetGPUResourceInfo(gpuInfoList);
    for( auto& gpu : gpuInfoList)
    {
        qDebug() << gpu.toStr() << "compute utilization:" << gpu.computeUtilization;
    }
    std::sort(gpuInfoList.begin(), gpuInfoList.end(), [](const GPUInfo & v1, const GPUInfo & v2)
    {
        return v1.deviceId < v2.deviceId;
    });
    if(gpuInfoList.size())
    {
        // 按实例创建顺序轮询：0,1,0,1,...
        int idx = ((signalcore % gpuInfoList.size()) + gpuInfoList.size()) % gpuInfoList.size();
        targetDevice = gpuInfoList.first().deviceId;
        idx = RGTimeSystem::GetInstance()->RFElapsedNanoNow_ns() % gpuInfoList.size();
        targetDevice = gpuInfoList.at(idx).deviceId;
        qDebug() << "use gpu:" << targetDevice << "/" << gpuInfoList.size();
    }
//    targetDevice = 0;
    cudaGetDeviceCount(&deviceCount);
    this->variableApplication();
    qDebug() << "RCSignalProcessMain::active - connect";
    this->internalSignalConnect();
    this->exec();
    qDebug() << "RCSignalProcessMain::active - exec";
}
bool RCSignalProcessMain::variableApplication()
{
    auto v = RWLock::make_write_guard(this->mInitialFlgLock);
    RGTimeSystem::GetInstance()->RFDelayMs(500);
    this->mWaveInfoLst.clear();
    auto map = SignalCore::GlobalVal::GetInstance()->waveBaseInfoLst();
    this->mWaveInfoLst =  map;
//    for(auto v : this->mWaveInfoLst.keys())
//    {
//        qDebug() << v.toStr() << this->mWaveInfoLst[v];
//    }
    for(auto info : this->mWaveInfoLst.keys())
    {
//        int waveCode = this->mWaveInfoLst[info];
        if(info.pw_us * info.prt_us * info.pulseNbr  == 0)
        {
            this->mWaveInfoLst.remove(info);
            qDebug() << "invalid wave:" << info.toStr();
        }
    }
    int size = this->mWaveInfoLst.size();
    for(auto v : this->mWaveInfoLst.keys())
    {
        if( this->mWaveInfoLst[v] < 0 )
        {
            assert(false);
        }
    }
    auto waveCodeLst = this->mWaveInfoLst.values();
    if( this->mWaveInfoLst.size() )
    {
        size = 1 + *std::max_element( waveCodeLst.begin(), waveCodeLst.end() );
    }
    else
    {
        size = 1;
    }
    wave_para_all = (wave_para_t*)calloc(size, sizeof(wave_para_t));
//    bool openCoelSuccess;
    this->DataFolder = QDateTime::currentDateTime().toString("yy_MM_dd_HH_mm_ss");
    /** @brief   variable initial. */
    CHECK_CUDA(cudaSetDevice(targetDevice));
    pinInput = qgetenv("SP_PIN_INPUT") != "0";
    CHECK_CUDA(cudaEventCreateWithFlags(&legacyReadyEvent, cudaEventDisableTiming));
    CHECK_CUDA(cudaEventCreateWithFlags(&pointsReadyEvent, cudaEventDisableTiming));
#if defined (Signalprocess_Radar_Archiwave) || defined (Signalprocess_Radar_chaofeng)
    wave_para_init_archiwave(wave_para_all, this->mWaveInfoLst); //initialization waveform
    //脉压系数
    pc_coel_wave = (cuComplex**)malloc(size * sizeof(cuComplex*));
    for(auto  info : this->mWaveInfoLst.keys())
    {
        int i = this->mWaveInfoLst[info];
        cudaMalloc(&pc_coel_wave[i], sizeof(cuComplex)*wave_para_all[i].fft_length);
    }
    for(auto info : this->mWaveInfoLst.keys())
    {
        int waveCode = this->mWaveInfoLst[info];
        shared_ptr<SignalProcesEchoCal::FilterWndInfo> pc_coel_h = SignalProcesEchoCal::GetInstance()->getPulseCompressCoef(info.pw_us, info.prt_us, info.sampleFreqMHz, info.waveBandMHz);
        cudaMemcpy(pc_coel_wave[waveCode], pc_coel_h->data, sizeof(cuComplex)*wave_para_all[waveCode].fft_length, cudaMemcpyHostToDevice);
    }
    //根据实际波形信息动态计算最大值
    quint32 minNbr = 256 * 2096;
    // 导出单个距离单元时，需要一个一维临时缓存保存脉压/MTD 的复数序列。
    // 这里按工程内波形的最大 pulse_number 动态申请，避免写死 512。
    quint32 PartLenMax = 512;
    quint32 PulseLenPulseNumMax = minNbr;
    quint32 FFTLenPulseNumMax = minNbr;
    quint32 PulseLenMTDNumMax = minNbr;
    for(auto info : this->mWaveInfoLst.keys())
    {
        int sample_nbr = info.sampleNbr;
        int pulse_nbr = info.pulseNbr;
        maxInputBytes = std::max(maxInputBytes,
                                 3 * sizeof(SampleComplexInt16) * static_cast<size_t>(sample_nbr) * pulse_nbr);
        int fft_length_tmp = roundf(info.prt_us * info.sampleFreqMHz);
        quint32 currentTotalSamplesMax = fft_length_tmp * pulse_nbr;
        quint32 currentTotalSamples = sample_nbr * pulse_nbr;
        if(PulseLenPulseNumMax < currentTotalSamples)
        {
            PulseLenPulseNumMax = currentTotalSamples;
        }
        if(FFTLenPulseNumMax < currentTotalSamplesMax)
        {
            FFTLenPulseNumMax = currentTotalSamplesMax;
        }
        if(PulseLenMTDNumMax < currentTotalSamples)
        {
            PulseLenMTDNumMax = currentTotalSamples;
        }
        if(PartLenMax < (quint32)pulse_nbr)
        {
            PartLenMax = pulse_nbr;
        }
    }
    /************************************|获取GPU数量,分配内存|************************************/
    CHECK_CUDA(cudaSetDevice(targetDevice));
    //
    CHECK_CUDA(cudaMalloc(&echo_data_sum_int,       sizeof(SampleComplexInt16) * static_cast<size_t>(PulseLenPulseNumMax) * 3));
    CHECK_CUDA(cudaMalloc(&echo_data_sum,           sizeof(cuComplex)*PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_data_azi,           sizeof(cuComplex)*PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_data_ele,           sizeof(cuComplex)*PulseLenPulseNumMax));
    //平台速度补偿后回波
    CHECK_CUDA(cudaMalloc(&echoCompensationSum,     sizeof(cuComplex)*PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echoCompensationAzi,     sizeof(cuComplex)*PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echoCompensationEle,     sizeof(cuComplex)*PulseLenPulseNumMax));
    //
    // The four sum pointers own contiguous sum/azimuth/elevation buffers.
    CHECK_CUDA(cudaMalloc(&padded_echo_data_sum,    sizeof(cuComplex)*FFTLenPulseNumMax * 3));
    CHECK_CUDA(cudaMalloc(&echo_fft_data_sum,       sizeof(cuComplex)*FFTLenPulseNumMax * 3));
    CHECK_CUDA(cudaMalloc(&pc_fft_data_sum,         sizeof(cuComplex)*FFTLenPulseNumMax * 3));
    CHECK_CUDA(cudaMalloc(&pc_data_sum,             sizeof(cuComplex)*FFTLenPulseNumMax * 3));
    //
    CHECK_CUDA(cudaMalloc(&DataWeighedMeanFilSum,   sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&DataWeighedMeanFilAzi,   sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&DataWeighedMeanFilEle,   sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_sum,                 sizeof(cuComplex) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_azi,                 sizeof(cuComplex) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_ele,                 sizeof(cuComplex) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_sum_fftshift,        sizeof(cuComplex) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_azi_fftshift,        sizeof(cuComplex) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_ele_fftshift,        sizeof(cuComplex) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_abs_sum,             sizeof(float) * PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_abs_sum_clutter,     sizeof(float) * PulseLenPulseNumMax));
    //
    CHECK_CUDA(cudaMalloc(&integral_image_row,      sizeof(float) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&integral_image_col,      sizeof(float) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&bool_data,               sizeof(unsigned char) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&DianJi_para,             sizeof(DianJi_para_t) * PulseLenMTDNumMax));
    const size_t compactBlocks = (PulseLenMTDNumMax + 255) / 256;
    CHECK_CUDA(cudaMalloc(&compactBlockCounts,      sizeof(unsigned int) * compactBlocks));
    CHECK_CUDA(cudaMalloc(&compactBlockOffsets,     sizeof(unsigned int) * compactBlocks));
    CHECK_CUDA(cudaMalloc(&compactDots,             sizeof(CompactDotResult)));
    // 单距离单元导出缓存:
    // pc_data_sum_Part 保存脉压后对应距离单元的一列复数数据，
    // mtd_sum_Part 保存 MTD 后对应距离单元的一列复数数据。
    CHECK_CUDA(cudaMalloc(&pc_data_sum_Part,        sizeof(cuComplex) * PartLenMax));
    CHECK_CUDA(cudaMalloc(&mtd_sum_Part,            sizeof(cuComplex) * FFTLenPulseNumMax));
#elif defined Signalprocess_Radar_RDV
    wave_para_init_RDV(wave_para_all);//initialization waveform
    //脉压系数
    for(i = 0; i < WAVENUM; i++)
    {
        cudaMallocManaged(&pc_coel_wave[i], sizeof(cuComplex)*wave_para_all[i].fft_length);
    }
    //读取脉压系数
    openCoelSuccess =  read_pc_coel_data_RDV(pc_coel_wave, wave_para_all);
    if(openCoelSuccess == false)
    {
        qDebug() << "get coe failure";
        exit(-1);
    }
    else
    {
    }
    quint32 PulseLenPulseNumMax = 1788 * 512;   //wave3/6
    quint32 FFTLenPulseNumMax = 2048 * 853;   //wave4/5
    quint32 FFTLenMTDNumMax = 2048 * 1024;   //wave4/5/7/9
    quint32 PulseLenMTDNumMax = 788 * 2048;   //wave9
    //
    CHECK_CUDA(cudaMalloc(&echo_data_sum,           sizeof(cuComplex)*PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_data_azi,           sizeof(cuComplex)*PulseLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_data_ele,           sizeof(cuComplex)*PulseLenPulseNumMax));
    //
    CHECK_CUDA(cudaMalloc(&padded_echo_data_sum,    sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&padded_echo_data_azi,    sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&padded_echo_data_ele,    sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_fft_data_sum,       sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_fft_data_azi,       sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&echo_fft_data_ele,       sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&pc_fft_data_sum,         sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&pc_fft_data_azi,         sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&pc_fft_data_ele,         sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&pc_data_sum,             sizeof(cuComplex)*FFTLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&pc_data_azi,             sizeof(cuComplex)*FFTLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&pc_data_ele,             sizeof(cuComplex)*FFTLenMTDNumMax));
    //
    CHECK_CUDA(cudaMalloc(&DataWeighedMeanFilSum,   sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&DataWeighedMeanFilAzi,   sizeof(cuComplex)*FFTLenPulseNumMax));
    CHECK_CUDA(cudaMalloc(&DataWeighedMeanFilEle,   sizeof(cuComplex)*FFTLenPulseNumMax));
    //
    //CHECK_CUDA(cudaMalloc(&DataMTDWinSum,           sizeof(cuComplex)*FFTLenPulseNumMax));
    //CHECK_CUDA(cudaMalloc(&DataMTDWinAzi,           sizeof(cuComplex)*FFTLenPulseNumMax));
    //CHECK_CUDA(cudaMalloc(&DataMTDWinEle,           sizeof(cuComplex)*FFTLenPulseNumMax));
    //capon
    cudaMalloc(&dataCapon,                          sizeof(cuComplex)*PulseLenPulseNumMax);
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxSum,       sizeof(cuComplex)*MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxAzi,       sizeof(cuComplex)*MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxEle,       sizeof(cuComplex)*MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxtpSum,     sizeof(cuComplex)*MCAPONORDER * MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxtpAzi,     sizeof(cuComplex)*MCAPONORDER * MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxtpEle,     sizeof(cuComplex)*MCAPONORDER * MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxtpinvSum,  sizeof(cuComplex)*MCAPONORDER * MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxtpinvAzi,  sizeof(cuComplex)*MCAPONORDER * MCAPONORDER * 3228));
    CHECK_CUDA(cudaMalloc(&dataCaponCorrxtpinvEle,  sizeof(cuComplex)*MCAPONORDER * MCAPONORDER * 3228));
    //
    CHECK_CUDA(cudaMalloc(&mtd_sum,                 sizeof(cuComplex) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_azi,                 sizeof(cuComplex) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_ele,                 sizeof(cuComplex) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&mtd_abs_sum,             sizeof(float) * PulseLenMTDNumMax));
    //
    CHECK_CUDA(cudaMalloc(&integral_image_row,      sizeof(float) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&integral_image_col,      sizeof(float) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&bool_data,               sizeof(unsigned char) * PulseLenMTDNumMax));
    CHECK_CUDA(cudaMalloc(&DianJi_para,             sizeof(DianJi_para_t) * PulseLenMTDNumMax));
    const size_t compactBlocks = (PulseLenMTDNumMax + 255) / 256;
    CHECK_CUDA(cudaMalloc(&compactBlockCounts,      sizeof(unsigned int) * compactBlocks));
    CHECK_CUDA(cudaMalloc(&compactBlockOffsets,     sizeof(unsigned int) * compactBlocks));
    CHECK_CUDA(cudaMalloc(&compactDots,             sizeof(CompactDotResult)));
#endif
    //读取窗函数系数
    kaiserCoel = (float**)malloc(size * sizeof(float*));
    hammingCoel = (float**)malloc(size * sizeof(float*));
    for(auto info : this->mWaveInfoLst.keys())
    {
        int waveCode = this->mWaveInfoLst[info];
        cudaMalloc(&kaiserCoel[waveCode], sizeof(cuComplex)*wave_para_all[waveCode].pulse_number);
        shared_ptr<SignalProcesEchoCal::FilterWndInfo> kaiser_win_h = SignalProcesEchoCal::GetInstance()->getKaiserWndPar(info.pulseNbr, 10);
        cudaMemcpy(kaiserCoel[waveCode], kaiser_win_h->data, sizeof(cuComplex)*wave_para_all[waveCode].pulse_number, cudaMemcpyHostToDevice);
        cudaMalloc(&hammingCoel[waveCode], sizeof(cuComplex)*wave_para_all[waveCode].pulse_number);
        shared_ptr<SignalProcesEchoCal::FilterWndInfo> hamming_win_h = SignalProcesEchoCal::GetInstance()->getHammingWndPar(info.pulseNbr);
        cudaMemcpy(hammingCoel[waveCode], hamming_win_h->data, sizeof(cuComplex)*wave_para_all[waveCode].pulse_number, cudaMemcpyHostToDevice);
    }
    //初始化cuda handle
    initCuHandle(wave_para_all, size);
    //初始化CFAR参数
    cfar_para_init(&cfar_para_current);
    this->initialFlg |= 0b001;
    return true;
}

/**
 * @brief 处理并发送目标微多普勒特征数据
 * @param trackInfo 目标跟踪信息
 * @param identifyNbr 识别编号
 * @param mRange 目标距离单元
 * @param pcDataSum 脉压数据指针
 * @param wavePara 当前波形参数
 */
void RCSignalProcessMain::processAndSendMicroDoppler(shared_ptr<RawData2SP> I_dataFromDataPar, quint32& mRange, cuComplex* pcDataSum, const wave_para_t& wavePara)
{
    DefMicroDopplerMatrix dopplerData;
    mRange = mRange % wavePara.pulse_length;
    dopplerData.trackNumber = I_dataFromDataPar->head->mTrkingInfo.TrackNumber;
    dopplerData.identifyNbr = I_dataFromDataPar->head->mTrkingInfo.bak.ClsInfo.identifyNbr;
    Get_pc_data_sum(pcDataSum, pc_data_sum_Part, mRange, wavePara);
    SampleComplexFloat* dopplerDataArray = (SampleComplexFloat*)malloc(sizeof(SampleComplexFloat) * wavePara.pulse_number);
    CHECK_CUDA(cudaMemcpy(dopplerDataArray, pc_data_sum_Part, sizeof(cuComplex) * wavePara.pulse_number, cudaMemcpyDeviceToHost));
    QDateTime currentTime = QDateTime::currentDateTime();
    QString filename = QString("./debug/%0/Track_%1/%1_%2.dat")
                       .arg(currentTime.toString("yyyy年MM月dd日"))
                       .arg(I_dataFromDataPar->head->mTrkingInfo.TrackNumber)
                       .arg(currentTime.toString("yyyy年MM月dd日hh时mm分ss秒-zzz"));
    QDir().mkpath(QString("./debug/%0/Track_%1/").arg(currentTime.toString("yyyy年MM月dd日")).arg(I_dataFromDataPar->head->mTrkingInfo.TrackNumber));
    QFile file(filename);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(reinterpret_cast<char*>(dopplerDataArray), 512 * sizeof(SampleComplexFloat));
        file.close();
    }
    dopplerData.matrix.append(reinterpret_cast<char*>(dopplerDataArray), sizeof(SampleComplexFloat) * wavePara.pulse_number);
    QUdpSocket socket;
    socket.bind();
    socket.writeDatagram(dopplerData.data(), QHostAddress("127.0.0.1"), 14321);
    SignalCore::RGCommunicateCore::GetInstance()->RFSendMicroDopplerData(dopplerData);
    free(dopplerDataArray);
}

bool RCSignalProcessMain::internalSignalConnect()
{
    auto v = RWLock::make_write_guard(this->mInitialFlgLock);
    this->initialFlg |= 0b00010;
    return true;
}
bool RCSignalProcessMain::exec()
{
    {
        auto v = RWLock::make_write_guard(this->mInitialFlgLock);
        this->initialFlg |= 0b0100;
    }
    this->run();
    return true;
}
void RCSignalProcessMain::run()
{
}
RCSignalProcessMain::RCSignalProcessMain()
{
    this->active();
}
RCSignalProcessMain::~RCSignalProcessMain()
{
    qDebug() << "task stop;";
    this->isStop = true;
    this->destroy();
    //停止任务
}
#endif

