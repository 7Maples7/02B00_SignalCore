#include "../include/blas.h"
#include <cuda.h>
#include <cublas_v2.h>
#include <cufft.h>
#include <stdio.h>
#include "../include/common.h"

//#include<archive/CDefine.h>

cufftHandle* pc_fft_handle = NULL;
#if defined (Signalprocess_Radar_Archiwave) || defined (Signalprocess_Radar_chaofeng)
static cufftHandle* pc_fft_handle_three = NULL;
static int pc_fft_handle_three_count = 0;
#endif
cufftHandle* mtd_fft_handle = NULL;
cufftHandle mtd_fft_handle_512 = 0;
cufftHandle pc_fft_handle_512 = 0;
cublasHandle_t blasHandle = NULL;

void initCuHandle(wave_para_t* wave_para_p, int waveNum)
{
    mtd_fft_handle = (cufftHandle*)malloc(waveNum * sizeof(cufftHandle));
    unsigned int i;
#if !defined (Signalprocess_Radar_Archiwave) && !defined (Signalprocess_Radar_chaofeng)
    pc_fft_handle = (cufftHandle*)malloc(waveNum * sizeof(cufftHandle));
    for(i = 0; i < waveNum; i++)
    {
        cufftPlan1d(&pc_fft_handle[i], wave_para_p[i].fft_length, CUFFT_C2C, wave_para_p[i].pulse_number);
    }
#else
    pc_fft_handle_three = (cufftHandle*)calloc(waveNum, sizeof(cufftHandle));
    pc_fft_handle_three_count = waveNum;
    for(i = 0; i < waveNum; i++)
    {
        if(wave_para_p[i].fft_length == 0 || wave_para_p[i].pulse_number == 0)
        {
            continue;
        }
        CHECK_CUFFT(cufftPlan1d(&pc_fft_handle_three[i], wave_para_p[i].fft_length,
                                CUFFT_C2C, 3 * wave_para_p[i].pulse_number));
        CHECK_CUFFT(cufftSetStream(pc_fft_handle_three[i], cudaStreamPerThread));
    }
#endif
    int pulse_length, fft_number, fft_length;
    const int rank = 1;
    int n[rank];
    int inembed[1];
    int istride;
    int idist = 1;
    int onembed[1];
    int ostride;
    int odist = 1;
    int batch;
    for(i = 0; i < waveNum; i++)
    {
        pulse_length = wave_para_p[i].pulse_length;
        fft_number = wave_para_p[i].fft_number;
        fft_length = wave_para_p[i].fft_length;
        n[0] = fft_number;
        inembed[0] = fft_number;
        istride = fft_length;
        onembed[0] = fft_number;
        ostride = pulse_length;
        batch = pulse_length;
        cufftPlanMany(&mtd_fft_handle[i], rank, n, inembed, istride, idist, onembed, ostride, odist, CUFFT_C2C, batch);
    }
    cublasCreate_v2(&blasHandle);
}

void init_pulsecompress_fft_gpu(wave_para_t* wave_para_p)
{
    unsigned int i;
    for(i = 0; i < WAVENUM; i++)
    {
        cufftPlan1d(&pc_fft_handle[i], wave_para_p[i].fft_length, CUFFT_C2C, wave_para_p[i].pulse_number);
    }
}

void pulsecompress_fft_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p)
{
    cufftHandle fft_handle;
    if(wave_para_p.waveCode < WAVENUM)
    {
        fft_handle = pc_fft_handle[wave_para_p.waveCode];
    }
    else
    {
        fft_handle = pc_fft_handle[wave_para_p.waveCode - WAVENUM];
    }
    cufftExecC2C(fft_handle, input_data, output_data, CUFFT_FORWARD);
}

void pulsecompress_ifft_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p)
{
    cufftHandle fft_handle;
    if(wave_para_p.waveCode < WAVENUM)
    {
        fft_handle = pc_fft_handle[wave_para_p.waveCode];
    }
    else
    {
        fft_handle = pc_fft_handle[wave_para_p.waveCode - WAVENUM];
    }
    cufftExecC2C(fft_handle, input_data, output_data, CUFFT_INVERSE);
}

#if defined (Signalprocess_Radar_Archiwave) || defined (Signalprocess_Radar_chaofeng)
void pulsecompress_fft_three_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p)
{
    const int index = wave_para_p.waveCode < WAVENUM
                          ? wave_para_p.waveCode : wave_para_p.waveCode - WAVENUM;
    CHECK_CUFFT(cufftExecC2C(pc_fft_handle_three[index], input_data, output_data, CUFFT_FORWARD));
}

void pulsecompress_ifft_three_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p)
{
    const int index = wave_para_p.waveCode < WAVENUM
                          ? wave_para_p.waveCode : wave_para_p.waveCode - WAVENUM;
    CHECK_CUFFT(cufftExecC2C(pc_fft_handle_three[index], input_data, output_data, CUFFT_INVERSE));
}
#endif

void init_mtd_fft_gpu(wave_para_t* wave_para_p)
{
    int pulse_length, fft_number, fft_length;
    const int rank = 1;
    int n[rank];
    int inembed[1];
    int istride;
    int idist = 1;
    int onembed[1];
    int ostride;
    int odist = 1;
    int batch;
    unsigned int i;
    for(i = 0; i < WAVENUM; i++)
    {
        pulse_length = wave_para_p[i].pulse_length;
        fft_number = wave_para_p[i].fft_number;
        fft_length = wave_para_p[i].fft_length;
        n[0] = fft_number;
        inembed[0] = fft_number;
        istride = fft_length;
        onembed[0] = fft_number;
        ostride = pulse_length;
        batch = pulse_length;
        cufftPlanMany(&mtd_fft_handle[i], rank, n, inembed, istride, idist, onembed, ostride, odist, CUFFT_C2C, batch);
    }
}

void mtd_fft_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p)
{
//	int width = wave_para_p->pulse_length;
//  int height = wave_para_p->fft_number;
    cufftHandle fft_handle;
    if(wave_para_p.waveCode < WAVENUM)
    {
        fft_handle = mtd_fft_handle[wave_para_p.waveCode];
    }
    else
    {
        fft_handle = mtd_fft_handle[wave_para_p.waveCode - WAVENUM];
    }
    cufftExecC2C(fft_handle, input_data, output_data, CUFFT_FORWARD);
    //cuda_abs(output_data, width, height, output_data_abs);
}

void mtd_fft_gpu_512(cuComplex* input_data, cuComplex* output_data)
{
//	int width = wave_para_p->pulse_length;
//  int height = wave_para_p->fft_number;
    cufftExecC2C(mtd_fft_handle_512, input_data, output_data, CUFFT_FORWARD);
}

void pulsecompress_fft_gpu_512(cuComplex* input_data, cuComplex* output_data)
{
    cufftExecC2C(pc_fft_handle_512, input_data, output_data, CUFFT_FORWARD);
}

void pulsecompress_ifft_gpu_512(cuComplex* input_data, cuComplex* output_data)
{
    cufftExecC2C(pc_fft_handle_512, input_data, output_data, CUFFT_INVERSE);
}

void destroyCuHandle(void)
{
//    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//    unsigned int i;
//    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx%d",pc_fft_handle == NULL);
#if defined (Signalprocess_Radar_Archiwave) || defined (Signalprocess_Radar_chaofeng)
    if(pc_fft_handle_three != NULL)
    {
        for(int i = 0; i < pc_fft_handle_three_count; ++i)
        {
            if(pc_fft_handle_three[i] != 0)
            {
                CHECK_CUFFT(cufftDestroy(pc_fft_handle_three[i]));
            }
        }
        free(pc_fft_handle_three);
        pc_fft_handle_three = NULL;
        pc_fft_handle_three_count = 0;
    }
#endif
    if(pc_fft_handle != NULL)
    {
        cufftDestroy(*pc_fft_handle);
        free(pc_fft_handle);
    }
    if(mtd_fft_handle != NULL)
    {
        cufftDestroy(*mtd_fft_handle);
        free(mtd_fft_handle);
    }
    //
    if( blasHandle != NULL)
    {
        cublasDestroy_v2(blasHandle);
    }
    cufftDestroy(mtd_fft_handle_512);
}


void initCuHandle_doppler(void)
{
    int pulse_length, fft_number, fft_length;
    const int rank = 1;
    int n[rank];
    int inembed[1];
    int istride;
    int idist = 1;
    int onembed[1];
    int ostride;
    int odist = 1;
    int batch;
    pulse_length = 3589;
    fft_number = 512;
    fft_length = 4000;
    n[0] = fft_number;
    inembed[0] = fft_number;
    istride = fft_length;
    onembed[0] = fft_number;
    ostride = pulse_length;
    batch = pulse_length;
    cufftPlan1d(&pc_fft_handle_512, fft_length, CUFFT_C2C, fft_number);
    cufftPlanMany(&mtd_fft_handle_512, rank, n, inembed, istride, idist, onembed, ostride, odist, CUFFT_C2C, 3589);
}
