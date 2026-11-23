#pragma once

#include "init.h"
#include <cuda_runtime.h>
#include <cuComplex.h>

#define BLOCK_DIM 1024
#define BLOCK_2DIM 32

#define LPF_LENGTH 67
#define DDS_R_TDELAY_NUM 5      //100ns
#define PW_GAP_NUM 20         //0.5us
#define PLOT_CENTROID_TH_R 4    //距离维最大凝聚点数
#define PLOT_CENTROID_TH_V 2    //速度维最大凝聚点数

#define MAX_DOTS_NUM 100

typedef struct DianJi_para
{
    float range ;
    float velocity ;
    float amplitude ;
    float aziRatio ;//chabiao
    float eleRatio ;//chabiao
    float FreqHz;
    float I_azi = 0;
    float Q_azi = 0;
    float I_ele = 0;
    float Q_ele = 0;
    float RCS   = 0;
    float mean_row = 0;
    float mean_col = 0;
    float cfar_th_r = 0;
    float cfar_th_v = 0;
    float ampStatic_Clutter = 0;
    int   rangeCeilIdx = 0;//距离维索引
    int   dopCeilIdx = 0;
} DianJi_para_t;

struct CompactDot
{
    unsigned int index;
    DianJi_para_t para;
};

struct CompactDotResult
{
    unsigned int count;
    CompactDot dots[MAX_DOTS_NUM];
};

void initCaponInvertVar();
void destroyCaponInvertVar();

int iDivUp(int a, int b);
void CalcParabolaVertex(int x1, float y1, int x2, float y2, int x3, float y3, float* xv, float* yv);
void cuda_abs(cuComplex* src_data, float* dst_data, const wave_para_t wave_para_p, unsigned int height, unsigned int width);

//平台速度补偿
void platformSpeedCompensation(cuComplex* inData, cuComplex* outData,
                               float platformSpeed, float radarFrequency, const wave_para_t ParaWave);

//脉压
void pad_pc_data(float* src_echo_data, float* dst_data, unsigned int width, unsigned int height, unsigned int border_width);
void pad_pc_data_three(const cuComplex* sum, const cuComplex* azi, const cuComplex* ele,
                       cuComplex* dst, unsigned int width, unsigned int height, unsigned int border_width);
void pulse_compress(cuComplex* src_data, cuComplex* dst_data, cuComplex* coel_data, unsigned int height, unsigned int width);

//加权均值滤波
void SPWeighedMeanFil(cuComplex* InData, cuComplex* OutData, float* win1[], float* win2[], const wave_para_t ParaWave, unsigned int width, unsigned int height);
void SPWeighedMeanFil_512(cuComplex* InData, cuComplex* OutData, float* win1, float* win2, unsigned int width, unsigned int height);
//MTD加窗
void SPMTDWin(cuComplex* InData, cuComplex* OutData, float* CoelWin, const wave_para_t ParaWave);

void pad_mtd_data(float* src_data, float* dst_data, const wave_para_t wave_para_p);
void pad_matrix_mirror(float* src_data, float* dst_data, wave_para_t* wave_para_p, cfar_para_t* cfar_para_p);
void ca_cfar_2d_rec(float* src_data, int width, int height, float* padded_data, float* integral_image, float pfa,
                    int refer_left_width, int refer_right_width, int refer_top_height, int refer_bottom_height,
                    int guard_left_width, int guard_right_width, int guard_top_height, int guard_bottom_height,
                    float* dst_data);
void calculate_integral_cross(int width, int height, float* padded_data, float* integral_image_row, float* integral_image_column, cfar_para_t* cfar_para_p);


void ca_cfar_2d_cross(float* integral_image_row, float* integral_image_column,
                      float* mtd_abs_data, unsigned char* dst_data,
                      unsigned int width, unsigned int height, const wave_para_t wave_para_p,
                      const cfar_para_t cfar_para_p,
                      const sp_para_t SP_para_current);

void calculate_cross_serve(float* mtd_abs_data, const cfar_para_t cfar_para_p,
                           float* integral_image_row, float* integral_image_column,
                           unsigned int width, unsigned int height);

void plot_centroid(unsigned char* bool_data, float* mtd_abs_sum, const wave_para_t wave_para_p);
void plot_centroid_neighbour(unsigned char* bool_data, float* mtd_abs_sum, unsigned int width, unsigned int height);

void sec_ddc(cuComplex* echo_sum, cuComplex* echo_azi, cuComplex* echo_ele,
             cuComplex* lpf_out_sum, cuComplex* lpf_out_azi, cuComplex* lpf_out_ele,
             cuComplex* ddc_coel_0l, cuComplex* ddc_coel_0s,
             cuComplex* ddc_coel_1l, cuComplex* ddc_coel_1s,
             float* lpf, int l_lpf,
             wave_para_t* wave_para_p, int lORs_flag);

void SPCalculateCeilPara(float* mtd_abs_sum, float* integral_image_row, float* integral_image_column,
                         cufftComplex* mtd_sum, cufftComplex* mtd_fw, cufftComplex* mtd_fy,
                         DianJi_para_t* OutParaDots,
                         unsigned int width, unsigned int height, const wave_para_t ParaWave, const sp_para_t SP_para_current,
                         const data_pack_para_t ParaArray, unsigned char* bool_data, float* clutter);

void compactSignalPoints(const unsigned char* bool_data, const DianJi_para_t* dots,
                         unsigned int count, unsigned int* blockCounts,
                         unsigned int* blockOffsets, CompactDotResult* result);

void caponInvert(cuComplex* src, cuComplex* dst, int n, int batchSize);
void caponPowerSpectraEstimation(cuComplex* x, cuComplex* S,
                                 cuComplex* corrx, cuComplex* corrxtp, cuComplex* corrxtpinv,
                                 unsigned int mOrder, const wave_para_t wave_para_p);
void Get_pc_data_sum(cuComplex* pc_data_sum, cuComplex* pc_data_sum_part, unsigned int idxRange, const wave_para_t wave_para_p);
void Get_mtd_data_sum(cuComplex* mtd_sum, cuComplex* mtd_sum_part, unsigned int idxRange, const wave_para_t wave_para_p);
void fftshift(cufftDoubleComplex** matrix);
void stft(double* f, int f_length, double wd, cufftDoubleComplex** TF);
void microDopplerSignature(unsigned int M_range, double* f, const wave_para_t ParaWave, float** TF_DB);
void convertIntToCuComplex(short* shortArray, cuComplex* cuComplexArray, const wave_para_t wave_para_p);

void fftshift_2d_rows_cpu(cuComplex* InData, cuComplex* OutData, int height, int width);
void DefClutterMemoryUpdate_cpu(float* InData, float* OutData, int height, int width, float RateWeight);
void Get_doppler_data_sum(cuComplex* doppler_data_sum, cuComplex* doppler_data_sum_part, unsigned int idxRange, const wave_para_t wave_para_p);
