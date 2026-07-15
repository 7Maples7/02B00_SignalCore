/*
 * init.h
 *
 *  Created on: Sep 28, 2020
 *      Author: ff
 */

#ifndef INIT_H_
#define INIT_H_

#include "cufft.h"
//#include<SPGlobalVar.h>
//#define SPSelfTest

//#define Signalprocess_Radar_Archiwave
//#define WAVENUM 9+2      //安其威阵面配置波形个数,+2表示2个长短脉冲的短脉冲
//#define Signalprocess_Radar_RDV
//#define WAVENUM 10+2   //瑞迪微阵面配置波形个数,+2表示2个长短脉冲的短脉冲
#define Signalprocess_Radar_chaofeng
//#include<CDefine.h>
//#include "QMap"
//#include<QMap>

#define MCAPONORDER 10
#define WAVENUM 10
typedef struct sp_para
{
    float cfar_th_r1 ;
    float cfar_th_r2 ;
    float cfar_th_r3 ;
    float cfar_th_v1 ;
    float cfar_th_v2 ;
    float cfar_th_v3 ;
    float snr_th ;
    unsigned int v_th_valid ;
    unsigned int r_th_valid ;
    unsigned int v_th_No_min;
    unsigned int v_th_No_max;
    unsigned int r_th_No_min;
    unsigned int r_th_No_max;
} sp_para_t;

typedef struct data_pack_para
{
    int array_num: 4;
    int wave_num: 8;
    float bc_azi;//deg
    float bc_ele;//deg
    float freq;
} data_pack_para_t;

typedef struct wave_para
{
    unsigned int waveCode;
    unsigned int pulse_length;
    unsigned int fft_length;
    unsigned int pulse_number;
    unsigned int fft_number;
    float prt_us;//*e-6,us
    float pw_us;
    int          sampleFreqMHz = 0    ;
    int          waveBandMHz   = 0    ;
} wave_para_t;

typedef struct cfar_para
{
    int refer_left_width;
    int refer_right_width;
    int refer_top_height;
    int refer_bottom_height;
    int guard_left_width;
    int guard_right_width;
    int guard_top_height;
    int guard_bottom_height;
    float pfa;
} cfar_para_t;

//void wave_para_init_archiwave(wave_para_t* wave_para_p, const QMap<WaveBaseInfo, int>& wave_map);  //波形参数初始化

void wave_para_init_RDV(wave_para_t* wave_para_p);

bool read_ddc_coel_data_archiwave(cuComplex* coel_data0_long, cuComplex* coel_data0_short, cuComplex* coel_data1_long,
                                  cuComplex* coel_data1_short, float* lpf, int lpf_length, wave_para_t* wave_para_p);

bool read_pc_coel_data_archiwave(cuComplex* coel_data[], wave_para_t* wave_para_p);

bool read_ddc_coel_data_RDV(cuComplex* coel_data0_long, cuComplex* coel_data0_short, cuComplex* coel_data1_long,
                            cuComplex* coel_data1_short, float* lpf, int lpf_length, wave_para_t* wave_para_p);

bool read_pc_coel_data_RDV(cuComplex* coel_data[], wave_para_t* wave_para_p);

bool ReadWindowsCoel(float* kaiser[], float* hamming[], wave_para_t* wave_para);

void cfar_para_init(cfar_para_t* cfar_para_p);  //CFAR参数初始化

bool read_input_data_archiwave(int wave_num, wave_para_t* wave_para_p, cuComplex* echo_data);

bool read_input_data_RDW(int wave_num, wave_para_t* wave_para_p, cuComplex* echo_data);

#endif /* INIT_H_ */
