/*
 * init.cpp
 *
 *  Created on: Jul 12, 2021
 *      Author: ff
 */

#include <stdio.h>
#include <assert.h>
#include "init.h"
#include<QString>
#include<CharAndNumProcessing.h>
//#include<SPGlobalSignal.h>
//#include<SPGlobalVar.h>
#include<math.h>

//archiwave波形参数初始化
/*
void wave_para_init_archiwave(wave_para_t* wave_para_p, const QMap<WaveBaseInfo, int>& wave_map)
{
    for(auto info : wave_map.keys())
    {
        int waveCode = wave_map[info];
        wave_para_p[waveCode].waveCode = waveCode;
        wave_para_p[waveCode].pulse_length = roundf((info.prt_us - info.pw_us) * 40 - 12);
        wave_para_p[waveCode].fft_length = roundf(info.prt_us * 40);
        wave_para_p[waveCode].pulse_number = info.pulseNbr;
        wave_para_p[waveCode].fft_number = info.pulseNbr;
        wave_para_p[waveCode].prt = info.prt_us;
        wave_para_p[waveCode].pw_length = roundf(info.pw_us * 40);
    }

    //波形0
    wave_para_p[0].waveCode = 0;
    wave_para_p[0].pulse_length = 1528;
    wave_para_p[0].fft_length = 2000;
    wave_para_p[0].pulse_number = 512;
    wave_para_p[0].fft_number = 512;
    wave_para_p[0].prt = 50;
    wave_para_p[0].pw_length = 400;//10us
    //波形1
    wave_para_p[1].waveCode = 1;
    wave_para_p[1].pulse_length = 952;
    wave_para_p[1].fft_length = 1280;
    wave_para_p[1].pulse_number = 256;
    wave_para_p[1].fft_number = 256;
    wave_para_p[1].prt = 32;
    wave_para_p[1].pw_length = 256;//6.65us
    //波形2
    wave_para_p[2].waveCode = 2;
    wave_para_p[2].pulse_length = 1588;
    wave_para_p[2].fft_length = 2000;
    wave_para_p[2].pulse_number = 512;
    wave_para_p[2].fft_number = 512;
    wave_para_p[2].prt = 50;
    wave_para_p[2].pw_length = 400;//10us
    //波形3
    wave_para_p[3].waveCode = 3;
    wave_para_p[3].pulse_length = 1012;
    wave_para_p[3].fft_length = 1280;
    wave_para_p[3].pulse_number = 800;
    wave_para_p[3].fft_number = 800;
    wave_para_p[3].prt = 32;
    wave_para_p[3].pw_length = 256;//6.4us
    //波形4
    wave_para_p[4].waveCode = 4;
    wave_para_p[4].pulse_length = 500;
    wave_para_p[4].fft_length = 640;
    wave_para_p[4].pulse_number = 1600;
    wave_para_p[4].fft_number = 1600;
    wave_para_p[4].prt = 16;
    wave_para_p[4].pw_length = 128;//3.2us
    //波形5
    wave_para_p[5].waveCode = 5;
    wave_para_p[5].pulse_length = 348;
    wave_para_p[5].fft_length = 400;
    wave_para_p[5].pulse_number = 2560;
    wave_para_p[5].fft_number = 2560;
    wave_para_p[5].prt = 10;
    wave_para_p[5].pw_length = 40;//1us
    //波形6
    wave_para_p[6].waveCode = 6;
    wave_para_p[6].pulse_length = 588;
    wave_para_p[6].fft_length = 640;
    wave_para_p[6].pulse_number = 128;
    wave_para_p[6].fft_number = 128;
    wave_para_p[6].prt = 16;
    wave_para_p[6].pw_length = 40;//1us
    //波形7
    wave_para_p[7].waveCode = 7;
    wave_para_p[7].pulse_length = 4388;
    wave_para_p[7].fft_length = 5200;
    wave_para_p[7].pulse_number = 256;
    wave_para_p[7].fft_number = 256;
    wave_para_p[7].prt = 130;
    wave_para_p[7].pw_length = 800;//20us
    //波形8
    wave_para_p[8].waveCode = 8;
    wave_para_p[8].pulse_length = 748;
    wave_para_p[8].fft_length = 800;
    wave_para_p[8].pulse_number = 512;
    wave_para_p[8].fft_number = 512;
    wave_para_p[8].prt = 20;
    wave_para_p[8].pw_length = 40;//1us
    //波形9 (波形0短脉冲)
    wave_para_p[9].waveCode = 9;
    wave_para_p[9].pulse_length = 1528;
    wave_para_p[9].fft_length = 2000;
    wave_para_p[9].pulse_number = 512;
    wave_para_p[9].fft_number = 512;
    wave_para_p[9].prt = 50;
    wave_para_p[9].pw_length = 40;//1us
    //波形10 (波形1短脉冲)
    wave_para_p[10].waveCode = 10;
    wave_para_p[10].pulse_length = 952;
    wave_para_p[10].fft_length = 1280;
    wave_para_p[10].pulse_number = 256;
    wave_para_p[10].fft_number = 256;
    wave_para_p[10].prt = 32;
    wave_para_p[10].pw_length = 40;//1us
    //    FILE* pFile = fopen("archiwave/wave3_echo.bin", "rb");
    //    fread(echo, 1, sizeof(cuComplex) * wave_para_p[3].pulse_length * wave_para_p[3].pulse_number, pFile);
    //    fclose(pFile);
    /////////////////////////////////////////////////////30M//////////////////////////////////////////////////
    //    //波形2
    //    wave_para_p[2].waveCode = 2;
    //    wave_para_p[2].pulse_length = 1191;
    //    wave_para_p[2].fft_length = 2048;
    //    wave_para_p[2].pulse_number = 512;
    //    wave_para_p[2].fft_number = 512;
    //    wave_para_p[2].prt = 50;
    //    wave_para_p[2].pw_length = 300;//10us
    //    //波形3
    //    wave_para_p[3].waveCode = 3;
    //    wave_para_p[3].pulse_length = 759;
    //    wave_para_p[3].fft_length = 2048;
    //    wave_para_p[3].pulse_number = 800;
    //    wave_para_p[3].fft_number = 1024;
    //    wave_para_p[3].prt = 32;
    //    wave_para_p[3].pw_length = 196;//6.4us
    //    //波形4
    //    wave_para_p[4].waveCode = 4;
    //    wave_para_p[4].pulse_length = 375;
    //    wave_para_p[4].fft_length = 1024;
    //    wave_para_p[4].pulse_number = 1600;
    //    wave_para_p[4].fft_number = 2048;
    //    wave_para_p[4].prt = 16;
    //    wave_para_p[4].pw_length = 96;//3.2us
    //    //波形5
    //    wave_para_p[5].waveCode = 5;
    //    wave_para_p[5].pulse_length = 261;
    //    wave_para_p[5].fft_length = 512;
    //    wave_para_p[5].pulse_number = 2560;
    //    wave_para_p[5].fft_number = 4096;
    //    wave_para_p[5].prt = 10;
    //    wave_para_p[5].pw_length = 30;//1us
    //    //波形6
    //    wave_para_p[6].waveCode = 6;
    //    wave_para_p[6].pulse_length = 441;
    //    wave_para_p[6].fft_length = 1024;
    //    wave_para_p[6].pulse_number = 128;
    //    wave_para_p[6].fft_number = 128;
    //    wave_para_p[6].prt = 16;
    //    wave_para_p[6].pw_length = 30;//1us
    //    //波形7
    //    wave_para_p[7].waveCode = 7;
    //    wave_para_p[7].pulse_length = 2391;
    //    wave_para_p[7].fft_length = 4096;
    //    wave_para_p[7].pulse_number = 256;
    //    wave_para_p[7].fft_number = 256;
    //    wave_para_p[7].prt = 100;
    //    wave_para_p[7].pw_length = 600;//20us
    //    //波形8
    //    wave_para_p[8].waveCode = 8;
    //    wave_para_p[8].pulse_length = 561;
    //    wave_para_p[8].fft_length = 1024;
    //    wave_para_p[8].pulse_number = 512;
    //    wave_para_p[8].fft_number = 512;
    //    wave_para_p[8].prt = 20;
    //    wave_para_p[8].pw_length = 30;//1us

    return;
}
*/
//RDW波形参数初始化
/*
void wave_para_init_RDV(wave_para_t* wave_para_p)
{
    //波形0
    wave_para_p[0].waveCode = 0;
    wave_para_p[0].pulse_length = 1708;
    wave_para_p[0].fft_length = 2000;
    wave_para_p[0].pulse_number = 512;
    wave_para_p[0].fft_number = 512;
    wave_para_p[0].prt = 50;
    wave_para_p[0].pw_length = 200;//5us
    //波形1
    wave_para_p[1].waveCode = 1;
    wave_para_p[1].pulse_length = 988;
    wave_para_p[1].fft_length = 1200;
    wave_para_p[1].pulse_number = 256;
    wave_para_p[1].fft_number = 256;
    wave_para_p[1].prt = 30;
    wave_para_p[1].pw_length = 120;//3us
    //波形2
    wave_para_p[2].waveCode = 2;
    wave_para_p[2].pulse_length = 528;
    wave_para_p[2].fft_length = 600;
    wave_para_p[2].pulse_number = 255;          //20210203 ff
    wave_para_p[2].fft_number = 255;
    wave_para_p[2].prt = 15;
    wave_para_p[2].pw_length = 60;//1.5us
    //波形3
    wave_para_p[3].waveCode = 3;
    wave_para_p[3].pulse_length = 1788;
    wave_para_p[3].fft_length = 2000;
    wave_para_p[3].pulse_number = 512;
    wave_para_p[3].fft_number = 512;
    wave_para_p[3].prt = 50;
    wave_para_p[3].pw_length = 200;//5us
    //波形4
    wave_para_p[4].waveCode = 4;
    wave_para_p[4].pulse_length = 1068;
    wave_para_p[4].fft_length = 1200;
    wave_para_p[4].pulse_number = 853;
    wave_para_p[4].fft_number = 853;
    wave_para_p[4].prt = 30;
    wave_para_p[4].pw_length = 120;//3us
    //波形5
    wave_para_p[5].waveCode = 5;
    wave_para_p[5].pulse_length = 528;
    wave_para_p[5].fft_length = 600;
    wave_para_p[5].pulse_number = 1705;         //20210203 -1
    wave_para_p[5].fft_number = 1705;
    wave_para_p[5].prt = 15;
    wave_para_p[5].pw_length = 60;//1.5us
    //波形6
    wave_para_p[6].waveCode = 6;
    wave_para_p[6].pulse_length = 1788;
    wave_para_p[6].fft_length = 2000;
    wave_para_p[6].pulse_number = 512;
    wave_para_p[6].fft_number = 512;
    wave_para_p[6].prt = 50;
    wave_para_p[6].pw_length = 200;//5us
    //波形7
    wave_para_p[7].waveCode = 7;
    wave_para_p[7].pulse_length = 328;
    wave_para_p[7].fft_length = 400;
    wave_para_p[7].pulse_number = 2559;         //20210203 -1
    wave_para_p[7].fft_number = 2559;
    wave_para_p[7].prt = 10;
    wave_para_p[7].pw_length = 60;//1.5us
    //波形8
    wave_para_p[8].waveCode = 8;
    wave_para_p[8].pulse_length = 3228;
    wave_para_p[8].fft_length = 3600;
    wave_para_p[8].pulse_number = 256;
    wave_para_p[8].fft_number = 256;
    wave_para_p[8].prt = 90;
    wave_para_p[8].pw_length = 360;//9us
    //波形9
    wave_para_p[9].waveCode = 9;
    wave_para_p[9].pulse_length = 788;
    wave_para_p[9].fft_length = 860;
    wave_para_p[9].pulse_number = 1070;
    wave_para_p[9].fft_number = 1070;
    wave_para_p[9].prt = 21.5;
    wave_para_p[9].pw_length = 60;//1.5us
    //
    //波形0 short
    wave_para_p[10].waveCode = 10;
    wave_para_p[10].pulse_length = 1708;
    wave_para_p[10].fft_length = 2000;
    wave_para_p[10].pulse_number = 512;
    wave_para_p[10].fft_number = 512;
    wave_para_p[10].prt = 50;
    wave_para_p[10].pw_length = 60;//1.5us
    //波形1 short
    wave_para_p[11].waveCode = 11;
    wave_para_p[11].pulse_length = 988;
    wave_para_p[11].fft_length = 1200;
    wave_para_p[11].pulse_number = 256;
    wave_para_p[11].fft_number = 256;
    wave_para_p[11].prt = 30;
    wave_para_p[11].pw_length = 60;//1.5us
}
*/

//读取DDC系数
bool read_ddc_coel_data_archiwave(cuComplex* coel_data0_long, cuComplex* coel_data0_short, cuComplex* coel_data1_long,
                                  cuComplex* coel_data1_short, float* lpf, int lpf_length, wave_para_t* wave_para_p)
{
    int pulse_length, pulse_number;
    FILE* pFile;
    pulse_length = wave_para_p[0].pulse_length;
    pulse_number = wave_para_p[0].pulse_number;
    //波形0长脉冲
    pFile = fopen("archiwave/wave0_long_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave0_long_ddc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data0_long, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    //波形0短脉冲
    pFile = fopen("archiwave/wave0_short_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave0_short_ddc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data0_short, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    pulse_length = wave_para_p[1].pulse_length;
    pulse_number = wave_para_p[1].pulse_number;
    //波形1长脉冲
    pFile = fopen("archiwave/wave1_long_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave1_long_ddc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data1_long, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    //波形1短脉冲
    pFile = fopen("archiwave/wave1_short_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave1_short_ddc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data1_short, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    //读取lpf系数
    pFile = fopen("archiwave/ddc_lpf.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open ddc_lpf.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)lpf, 1, sizeof(float)*lpf_length, pFile);
    fclose(pFile);
    return true;
}

//读取脉压系数
bool read_pc_coel_data_archiwave(cuComplex* coel_data[], wave_para_t* wave_para_p)
{
    FILE* pFile;
    //波形0
    //长脉冲
    pFile = fopen("archiwave/wave0_long_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave0_long_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[0], 1, sizeof(cuComplex)*wave_para_p[0].fft_length, pFile);
    fclose(pFile);
    //短脉冲
    pFile = fopen("archiwave/wave0_short_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave0_short_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[9], 1, sizeof(cuComplex)*wave_para_p[9].fft_length, pFile);
    fclose(pFile);
    //波形1
    //长脉冲
    pFile = fopen("archiwave/wave1_long_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave1_long_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[1], 1, sizeof(cuComplex)*wave_para_p[1].fft_length, pFile);
    fclose(pFile);
    //短脉冲
    pFile = fopen("archiwave/wave1_short_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave1_short_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[10], 1, sizeof(cuComplex)*wave_para_p[10].fft_length, pFile);
    fclose(pFile);
    //波形2
    pFile = fopen("archiwave/wave2_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave2_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[2], 1, sizeof(cuComplex)*wave_para_p[2].fft_length, pFile);
    fclose(pFile);
    //波形3
    pFile = fopen("archiwave/wave3_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave3_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[3], 1, sizeof(cuComplex)*wave_para_p[3].fft_length, pFile);
    fclose(pFile);
    //波形4
    pFile = fopen("archiwave/wave4_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave4_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[4], 1, sizeof(cuComplex)*wave_para_p[4].fft_length, pFile);
    fclose(pFile);
    //波形5
    pFile = fopen("archiwave/wave5_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave5_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[5], 1, sizeof(cuComplex)*wave_para_p[5].fft_length, pFile);
    fclose(pFile);
    //波形6
    pFile = fopen("archiwave/wave6_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave6_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[6], 1, sizeof(cuComplex)*wave_para_p[6].fft_length, pFile);
    fclose(pFile);
    //波形7
    pFile = fopen("archiwave/wave7_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave7_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[7], 1, sizeof(cuComplex)*wave_para_p[7].fft_length, pFile);
    fclose(pFile);
    //波形8
    pFile = fopen("archiwave/wave8_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        SaveDataAppend(QString("can't open wave8_pc.bin\n").toUtf8(), "./splog.txt");
        assert(false);
        return false;
    }
    fread((float*)coel_data[8], 1, sizeof(cuComplex)*wave_para_p[8].fft_length, pFile);
    fclose(pFile);
    return true;
}

bool read_ddc_coel_data_RDV(cuComplex* coel_data0_long, cuComplex* coel_data0_short, cuComplex* coel_data1_long,
                            cuComplex* coel_data1_short, float* lpf, int lpf_length, wave_para_t* wave_para_p)
{
    int pulse_length, pulse_number;
    FILE* pFile;
    pulse_length = wave_para_p[0].pulse_length;
    pulse_number = wave_para_p[0].pulse_number;
    //波形0长脉冲
    pFile = fopen("RDV/wave0_long_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data0_long, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    //波形0短脉冲
    pFile = fopen("RDV/wave0_short_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data0_short, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    pulse_length = wave_para_p[1].pulse_length;
    pulse_number = wave_para_p[1].pulse_number;
    //波形1长脉冲
    pFile = fopen("RDV/wave1_long_ddc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data1_long, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    //波形1短脉冲
    pFile = fopen("RDV/wave1_short_ddc.bin", "rb");
    fread((float*)coel_data1_short, 1, sizeof(cuComplex)*pulse_length * pulse_number, pFile);
    fclose(pFile);
    //读取lpf系数
    pFile = fopen("RDV/ddc_lpf.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)lpf, 1, sizeof(float)*lpf_length, pFile);
    fclose(pFile);
    return true;
}

bool read_pc_coel_data_RDV(cuComplex* coel_data[], wave_para_t* wave_para_p)
{
    //int fft_length, pulse_number;
    FILE* pFile;
    //波形0
    //长脉冲
    pFile = fopen("RDV/wave0_long_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[0], 1, sizeof(cuComplex)*wave_para_p[0].fft_length, pFile);
    fclose(pFile);
    //短脉冲
    pFile = fopen("RDV/wave0_short_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[10], 1, sizeof(cuComplex)*wave_para_p[10].fft_length, pFile);
    fclose(pFile);
    //波形1
    //长脉冲
    pFile = fopen("RDV/wave1_long_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[1], 1, sizeof(cuComplex)*wave_para_p[1].fft_length, pFile);
    fclose(pFile);
    //短脉冲
    pFile = fopen("RDV/wave1_short_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[11], 1, sizeof(cuComplex)*wave_para_p[11].fft_length, pFile);
    fclose(pFile);
    //波形2
    pFile = fopen("RDV/wave2_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[2], 1, sizeof(cuComplex)*wave_para_p[2].fft_length, pFile);
    fclose(pFile);
    //波形3
    pFile = fopen("RDV/wave3_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[3], 1, sizeof(cuComplex)*wave_para_p[3].fft_length, pFile);
    fclose(pFile);
    //波形4
    pFile = fopen("RDV/wave4_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[4], 1, sizeof(cuComplex)*wave_para_p[4].fft_length, pFile);
    fclose(pFile);
    //波形5
    pFile = fopen("RDV/wave5_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[5], 1, sizeof(cuComplex)*wave_para_p[5].fft_length, pFile);
    fclose(pFile);
    //波形6
    pFile = fopen("RDV/wave6_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[6], 1, sizeof(cuComplex)*wave_para_p[6].fft_length, pFile);
    fclose(pFile);
    //波形7
    pFile = fopen("RDV/wave7_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[7], 1, sizeof(cuComplex)*wave_para_p[7].fft_length, pFile);
    fclose(pFile);
    //波形8
    pFile = fopen("RDV/wave8_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[8], 1, sizeof(cuComplex)*wave_para_p[8].fft_length, pFile);
    fclose(pFile);
    //波形9
    pFile = fopen("RDV/wave9_pc.bin", "rb");
    if(pFile != NULL)
    {
    }
    else
    {
        fclose(pFile);
        return  false;
    }
    fread((float*)coel_data[9], 1, sizeof(cuComplex)*wave_para_p[9].fft_length, pFile);
    fclose(pFile);
    return true;
}

//读取窗函数系数
bool ReadWindowsCoel(float* kaiser[], float* hamming[], wave_para_t* wave_para)
{
    FILE* pFile;
    uint i;
    for(i = 0; i < WAVENUM; i++)
    {
        switch(wave_para[i].pulse_number)
        {
            case 128:
                pFile = fopen("WinCoel/kaiser128.bin", "rb");
                break;
            case 255:
                pFile = fopen("WinCoel/kaiser255.bin", "rb");
                break;
            case 256:
                pFile = fopen("WinCoel/kaiser256.bin", "rb");
                break;
            case 512:
                pFile = fopen("WinCoel/kaiser512.bin", "rb");
                break;
            case 800:
                pFile = fopen("WinCoel/kaiser800.bin", "rb");
                break;
            case 853:
                pFile = fopen("WinCoel/kaiser853.bin", "rb");
                break;
            case 1070:
                pFile = fopen("WinCoel/kaiser1070.bin", "rb");
                break;
            case 1600:
                pFile = fopen("WinCoel/kaiser1600.bin", "rb");
                break;
            case 1705:
                pFile = fopen("WinCoel/kaiser1705.bin", "rb");
                break;
            case 2559:
                pFile = fopen("WinCoel/kaiser2559.bin", "rb");
                break;
            case 2560:
                pFile = fopen("WinCoel/kaiser2560.bin", "rb");
                break;
        }
        if(pFile != NULL)
        {
            fread((float*)kaiser[i], 1, sizeof(float) * wave_para[i].pulse_number, pFile);
            fclose(pFile);
        }
        else
        {
            qDebug() << "SP ERROR: no such file /WinCoel/kaiser" << wave_para[i].pulse_number << ".bin";
            fclose(pFile);
            return  false;
        }
        switch(wave_para[i].pulse_number)
        {
            case 128:
                pFile = fopen("WinCoel/hamming128.bin", "rb");
                break;
            case 255:
                pFile = fopen("WinCoel/hamming255.bin", "rb");
                break;
            case 256:
                pFile = fopen("WinCoel/hamming256.bin", "rb");
                break;
            case 512:
                pFile = fopen("WinCoel/hamming512.bin", "rb");
                break;
            case 800:
                pFile = fopen("WinCoel/hamming800.bin", "rb");
                break;
            case 853:
                pFile = fopen("WinCoel/hamming853.bin", "rb");
                break;
            case 1070:
                pFile = fopen("WinCoel/hamming1070.bin", "rb");
                break;
            case 1600:
                pFile = fopen("WinCoel/hamming1600.bin", "rb");
                break;
            case 1705:
                pFile = fopen("WinCoel/hamming1705.bin", "rb");
                break;
            case 2559:
                pFile = fopen("WinCoel/hamming2559.bin", "rb");
                break;
            case 2560:
                pFile = fopen("WinCoel/hamming2560.bin", "rb");
                break;
        }
        if(pFile != NULL)
        {
            fread((float*)hamming[i], 1, sizeof(float) * wave_para[i].pulse_number, pFile);
            fclose(pFile);
        }
        else
        {
            qDebug() << "SP ERROR: no such file /WinCoel/hamming" << wave_para[i].pulse_number << ".bin";
            fclose(pFile);
            return  false;
        }
    }
    return true;
}

//CFAR参数初始化
void cfar_para_init(cfar_para_t* cfar_para_p)
{
    cfar_para_p->refer_left_width = 16;
    cfar_para_p->refer_right_width = 16;
    cfar_para_p->refer_top_height = 16;
    cfar_para_p->refer_bottom_height = 16;
    cfar_para_p->guard_left_width = 3;
    cfar_para_p->guard_right_width = 3;
    cfar_para_p->guard_top_height = 3;
    cfar_para_p->guard_bottom_height = 3;
    cfar_para_p->pfa = 1e-5f;
//    cfar_para_p->refer_left_width = 10;
//    cfar_para_p->refer_right_width = 10;
//    cfar_para_p->refer_top_height = 10;
//    cfar_para_p->refer_bottom_height = 10;
//    cfar_para_p->guard_left_width = 3;
//    cfar_para_p->guard_right_width = 3;
//    cfar_para_p->guard_top_height = 3;
//    cfar_para_p->guard_bottom_height = 3;
//    cfar_para_p->pfa = 1e-5f;
    return;
}

//读取模拟回波
bool read_input_data_archiwave(int wave_num, wave_para_t* wave_para_p, cuComplex* echo_data)
{
    FILE* pFile = NULL;
    int pulse_length = wave_para_p->pulse_length;
    int pulse_number = wave_para_p->pulse_number;
    switch(wave_num)
    {
        case 0:
            pFile = fopen("archiwave/wave0_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave0_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 1:
            pFile = fopen("wave1_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave1_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 2:
            pFile = fopen("wave2_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave2_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 3:
            pFile = fopen("wave3_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave3_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 4:
            pFile = fopen("wave4_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave4_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 5:
            pFile = fopen("wave5_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave5_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 6:
            pFile = fopen("wave6_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave6_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 7:
            pFile = fopen("wave7_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave7_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 8:
            pFile = fopen("wave8_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave8_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 9:
            pFile = fopen("wave9_echo.bin", "rb");
            if(pFile != NULL)
            {
            }
            else
            {
                SaveDataAppend(QString("can't open wave9_echo.bin\n").toUtf8(), "./splog.txt");
                assert(false);
                return false;
            }
            fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
    }
    return true;
}

bool read_input_data_RDW(int wave_num, wave_para_t* wave_para_p, cuComplex* echo_data)
{
    FILE* pFile = NULL;
    int pulse_length = wave_para_p->pulse_length;
    int pulse_number = wave_para_p->pulse_number;
    size_t sizeOfRead = 0;
    switch(wave_num)
    {
        case 0:
            pFile = fopen("wave0_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 1:
            pFile = fopen("wave1_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 2:
            pFile = fopen("wave2_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 3:
            pFile = fopen("wave3_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 4:
            pFile = fopen("wave4_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 5:
            pFile = fopen("wave5_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 6:
            pFile = fopen("wave6_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 7:
            pFile = fopen("wave7_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 8:
            pFile = fopen("wave8_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
        case 9:
            pFile = fopen("wave9_echo.bin", "rb");
            sizeOfRead = fread((float*)echo_data, 1, sizeof(cuComplex) * pulse_length * pulse_number, pFile);
            fclose(pFile);
            break;
    }
    if(pFile == NULL || sizeOfRead == 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}
