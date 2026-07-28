#ifndef RCSIGNALPROCESSMAIN_H
#define RCSIGNALPROCESSMAIN_H
#ifdef IRadarBC_MOC
    #include<IRadarBC.h>
#endif
#include<mutex>
#include<QDebug>
#include<QObject>
/** @brief   . */
#include "common.h"
#include "init.h"
#include "kernel.h"
#include "blas.h"
//#include<InternalVarDefinition.h>4
//#include<RSSignalProcessingDataTypeDefinition.h>
#include<QTimer>
//#include<CDefine.h>
//#include<DEchoParDefine.h>
#include<QThread>
#include<QDateTime>
//#include<SPGlobalSignal.h>
//#include<SPGlobalVar.h>
#include<SignalProcPntPackage.h>
#include<concurrentqueue.h>
#include<InstanceBirth.h>
//#include<DTypeDefine4DW.h>
#include<CUTypeDefine.h>
#include<RGTimeSystem.h>
#if 1
#include"DefExchangeWithSP.h"
#include<SignalCore/SignalCoreDefine.h>
#include<Common/SAmpRatioInquire.h>
#include<Common/CommonDefine.h>
#include<SignalCore/SignalCoreRGCommunicateCore.h>
//用于获取当前 GPU 的数量以及每块 GPU 占用资源的多少，主要包括显存资源和计算资源
#include <cuda_runtime.h>
#include<ScndLvlDefn/DSignalProcPointProtocol.h>
#include<TargetTypeV5.h>
using namespace Common;
using namespace SignalCore;
class RCSignalProcessMain  : public QObject, public BuildStatus, public  InstanceBirth<RCSignalProcessMain>
{
    Q_OBJECT
public:
    //信号处理参数        //1. typedefs和enums；
    struct  SP_para
    {
        float cfar_th_r1 = 0;
        float cfar_th_r2 = 0;
        float cfar_th_r3 = 0;
        float cfar_th_v1 = 0;
        float cfar_th_v2 = 0;
        float cfar_th_v3 = 0;
        float snr_th = 0;
        unsigned int v_th_valid = 0;
        unsigned int r_th_valid = 0;
        float v_min = 0;
        float v_max = 0;
        float r_min = 0;
        float r_max = 0;
        QString toStr(qint32 reservedWidth = 0 )
        {
            QString str;
            str += QString("[v_th_valid:%0]").arg(v_th_valid, 6 + reservedWidth, 10, QLatin1Char(' '));
            str += QString("[r_th_valid:%0]").arg(r_th_valid, 6 + reservedWidth, 10, QLatin1Char(' '));
            str += QString("[snr_th:%0]").arg(snr_th, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
            str += QString("[v_min:%0]").arg(v_min, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
            str += QString("[v_max:%0]").arg(v_max, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
            str += QString("[r_min:%0]").arg(r_min, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
            str += QString("[r_max:%0]").arg(r_max, 8 + reservedWidth, 'f', 1, QLatin1Char(' '));
            return str;
        }


    };
    //2. 常量；
    //3. 成员函数，含静态成员函数；
public:
    void SaveData(char* buff, qint32 len, QString path)
    {
        QFileInfo fileinfo(path);
        QDir dir(fileinfo.absolutePath());
        if(dir.exists())
        {
        }
        else
        {
            dir.mkpath(fileinfo.absolutePath());//创建多级目录
        }
        QFile file;
        file.setFileName(path);
        bool isOK = file.open(QIODevice::WriteOnly);
        if (isOK == true)
        {
            file.write(buff, len);
            file.close();
        }
    }
    void ResRelease();

    void work(const SP_para SP, shared_ptr<RawData2SP> dataFromDataPar,
              QList<IRadarV51::SignalProcPoint>& DotPkg,
              shared_ptr<DefClutterMemory> clutterStatic, shared_ptr<DefClutterMemory> clutterDynamic);
    void fr_para_init(const SP_para I_SP,
                      shared_ptr<RawData2SP> dataFromDataPar,
                      wave_para_t wave_para_all[],
                      wave_para_t* wave_para_cur,
                      data_pack_para_t* data_para_cur,
                      sp_para_t* sp_para_cur);
    void signalProcess(shared_ptr<RawData2SP> I_dataFromDataPar,
                       shared_ptr<DefClutterMemory> clutterStatic,
                       shared_ptr<DefClutterMemory> clutterDynamic
                      );
    void wave_para_init_archiwave(wave_para_t* wave_para_p, const QMap<WaveBaseInfo, int>& wave_map);
    int RFSignalProcess(QList<IRadarV51::SignalProcPoint>& points,
                        shared_ptr<DefClutterMemory> clutterStatic,
                        shared_ptr<DefClutterMemory> clutterDynamic);

    struct GPUInfo
    {
        int deviceId;
        QString name;
        size_t totalMemory;
        size_t freeMemory;
        size_t usedMemory;
        int computeCapabilityMajor;
        int computeCapabilityMinor;
        int multiProcessorCount;
        int clockRate;
        bool isAvailable;
    };

    /**
     * @brief 获取当前 GPU 的数量以及每块 GPU 占用资源信息
     * @param gpuInfoList 输出参数，存储各 GPU 的详细信息
     * @return 返回可用的 GPU 数量，如果出错返回 -1
     */
    int GetGPUResourceInfo(QList<GPUInfo>& gpuInfoList)
    {
        int currentDevice = 0;
        cudaGetDevice(&currentDevice);
        int deviceCount = 0;
        cudaError_t err = cudaGetDeviceCount(&deviceCount);
        if (err != cudaSuccess)
        {
            qDebug() << "获取 GPU 数量失败:" << cudaGetErrorString(err);
            return -1;
        }
        gpuInfoList.clear();
        for (int i = 0; i < deviceCount; i++)
        {
            GPUInfo info;
            info.deviceId = i;
            cudaDeviceProp prop;
            err = cudaGetDeviceProperties(&prop, i);
            if (err != cudaSuccess)
            {
                qDebug() << "获取 GPU" << i << "属性失败:" << cudaGetErrorString(err);
                continue;
            }
            info.name = QString(prop.name);
            info.computeCapabilityMajor = prop.major;
            info.computeCapabilityMinor = prop.minor;
            info.multiProcessorCount = prop.multiProcessorCount;
#if CUDART_VERSION >= 13000
            int clockRateKhz = 0;
            err = cudaDeviceGetAttribute(&clockRateKhz, cudaDevAttrClockRate, i);
            if (err != cudaSuccess)
            {
                qDebug() << "获取 GPU" << i << "时钟频率失败:" << cudaGetErrorString(err);
                info.clockRate = 0;
                cudaGetLastError();
            }
            else
            {
                info.clockRate = clockRateKhz;
            }
#else
            info.clockRate = prop.clockRate;
#endif
            err = cudaSetDevice(i);
            if (err != cudaSuccess)
            {
                qDebug() << "设置 GPU" << i << "失败:" << cudaGetErrorString(err);
                info.isAvailable = false;
                gpuInfoList.append(info);
                continue;
            }
            size_t freeMem = 0, totalMem = 0;
            err = cudaMemGetInfo(&freeMem, &totalMem);
            if (err != cudaSuccess)
            {
                qDebug() << "获取 GPU" << i << "显存信息失败:" << cudaGetErrorString(err);
                info.isAvailable = false;
                gpuInfoList.append(info);
                continue;
            }
            info.totalMemory = totalMem;
            info.freeMemory = freeMem;
            info.usedMemory = totalMem - freeMem;
            info.isAvailable = true;
            gpuInfoList.append(info);
        }
        cudaSetDevice(currentDevice);
        cudaGetLastError();
        return gpuInfoList.size();
    }

    /**
     * @brief 打印 GPU 资源信息
     * @param gpuInfoList GPU 信息列表
     */
    void PrintGPUResourceInfo(const QList<GPUInfo>& gpuInfoList)
    {
        qDebug() << "================================ GPU 资源信息 ================================";
        qDebug() << "GPU 数量:" << gpuInfoList.size();
        for (const auto& info : gpuInfoList)
        {
            qDebug() << "----------------------------------------";
            qDebug() << "设备 ID:" << info.deviceId;
            qDebug() << "设备名称:" << info.name;
            qDebug() << "是否可用:" << (info.isAvailable ? "是" : "否");
            if (info.isAvailable)
            {
                qDebug() << "总显存:" << info.totalMemory / (1024.0 * 1024.0) << "MB";
                qDebug() << "已用显存:" << info.usedMemory / (1024.0 * 1024.0) << "MB";
                qDebug() << "空闲显存:" << info.freeMemory / (1024.0 * 1024.0) << "MB";
                qDebug() << "显存使用率:" << (double)info.usedMemory / info.totalMemory * 100 << "%";
                qDebug() << "计算能力:" << info.computeCapabilityMajor << "." << info.computeCapabilityMinor;
                qDebug() << "多处理器数量:" << info.multiProcessorCount;
                qDebug() << "时钟频率:" << info.clockRate / 1000 << "MHz";
            }
        }
        qDebug() << "============================================================================";
    }



    int SignalCoreRunCounter();
    void processAndSendMicroDoppler(shared_ptr<RawData2SP> I_dataFromDataPar, quint32& mRange, cuComplex* pcDataSum, const wave_para_t& wavePara);
private:
    void calculateRCS(IRadarV51::SignalProcPoint& dot_tmp);
    bool fillPcAndMtdExportData(IRadarV51::SignalProcPoint& dot_tmp,
                                quint32 pointIndex,
                                shared_ptr<RawData2SP> I_dataFromDataPar,
                                int DotNum);

    //4. 数据成员，含静态数据成员。
public:

private:
    //
    int deviceCount = 0;
    int signalcore = 0;
    int targetDevice = 0;
    wave_para_t* wave_para_all;//存储波形参数
    wave_para_t wave_para_current;//当前使用的波形参数
    sp_para_t SP_para_current;
    data_pack_para_t data_pack_para_current;
    cfar_para_t cfar_para_current;//CFAR Par
    //
    qint64 dataSaveSerialNumber = 1;
    QString DataFolder;
    //
    float lambda, seitaHalfAzi, seitaHalfEle, Fazi_t, Fazi_r, Fele_t, Fele_r, alpha_azi, alpha_ele, L_txz, L_dzf, L_gf, L_hcq, L_pz, Gama_a, Gama_w, L_atm, L_total, L_avefil, sig_Pr, V_AD, AD_Pr, Pr_0;
    double freq;
    //脉压系数
    cuComplex** pc_coel_wave;
    //窗函数系数
    float** kaiserCoel;
    float** hammingCoel;

    //float* kaiserCoel_512;
    //float* hammingCoel_512;
    //微多普勒回波数据
    cuComplex* echo_data_sum_doppler, * echo_data_azi_doppler, * echo_data_ele_doppler;
    bool DopplerDataSet = 0;
    //回波
    short* echo_data_sum_int, * echo_data_azi_int, * echo_data_ele_int;




    cuComplex* echo_data_sum, * echo_data_azi, * echo_data_ele;
    //平台速度补偿后回波
    cuComplex* echoCompensationSum, * echoCompensationAzi, * echoCompensationEle;
    //脉压
    cuComplex* padded_echo_data_sum;
    cuComplex* padded_echo_data_azi;
    cuComplex* padded_echo_data_ele;
    cuComplex* echo_fft_data_sum;
    cuComplex* echo_fft_data_azi;
    cuComplex* echo_fft_data_ele;
    cuComplex* pc_fft_data_sum;
    cuComplex* pc_fft_data_azi;
    cuComplex* pc_fft_data_ele;
    cuComplex* pc_data_sum;
    cuComplex* pc_data_azi;
    cuComplex* pc_data_ele;
    //
    cuComplex* DataWeighedMeanFilSum;
    cuComplex* DataWeighedMeanFilAzi;
    cuComplex* DataWeighedMeanFilEle;
    //
    cuComplex* DataMTDWinSum;
    cuComplex* DataMTDWinAzi;
    cuComplex* DataMTDWinEle;
    //capon
    cuComplex* dataCapon;
    cuComplex* dataCaponCorrxSum;
    cuComplex* dataCaponCorrxAzi;
    cuComplex* dataCaponCorrxEle;
    cuComplex* dataCaponCorrxtpSum;
    cuComplex* dataCaponCorrxtpAzi;
    cuComplex* dataCaponCorrxtpEle;
    cuComplex* dataCaponCorrxtpinvSum;
    cuComplex* dataCaponCorrxtpinvAzi;
    cuComplex* dataCaponCorrxtpinvEle;
    //MTD
    cuComplex* mtd_sum, * mtd_azi, * mtd_ele;
    cuComplex* mtd_sum_fftshift, * mtd_azi_fftshift, * mtd_ele_fftshift;
    float* mtd_abs_sum;
    float* mtd_abs_sum_on_cpu;
    float* mtd_abs_sum_clutter;
    //CFAR
    float* integral_image_row, * integral_image_col;
    unsigned char* bool_data;
    //
    DianJi_para_t* DianJi_para;
    //fot time
    double iStart, iElaps;
    shared_ptr<RawData2SP> mDataFrmPar  = nullptr;//待处理的回波数据
    //micro-Doppler signature
    cuComplex* pc_data_sum_Part;
    cuComplex* mtd_sum_Part;
    // 导出目标识别数据时，按目标距离单元左右各保留的邻域宽度。
    // 例如为 2 时，实际导出 [center-2, center-1, center, center+1, center+2] 共 5 个距离单元。
    int exportRangeHalfWidth = 2;

    int mCoreIndex = 0x00;

    //信号处理程序初始化标志以及资源锁
    int64_t initialFlg = 0x00;
    bool is_destroy_start  = false;
    QMap<WaveBaseInfo, int> mWaveInfoLst;
    RWLock mInitialFlgLock;//
    //杂波图-静态
//    DefClutterMemory StcClutterPattern;
    //**********************************************************************************//
    //                                                                                  //
    //**********************************************************************************//
public:

    bool variableApplication() override;
    bool internalSignalConnect() override ;
    bool exec() override;
    void run();



public:
    // 将其构造和析构成为私有的, 禁止外部构造和析构
    RCSignalProcessMain();
    ~RCSignalProcessMain();
    int coreIndex() const;
    void setCoreIndex(int coreIndex);

    // BuildStatus interface
public:
    virtual void destroy() override;
    virtual void active() override;
    void setDataFrmPar(const shared_ptr<RawData2SP>& dataFrmPar);
};
#endif
#endif
