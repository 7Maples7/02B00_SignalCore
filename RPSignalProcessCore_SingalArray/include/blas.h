#ifndef BLAS_H_
#define BLAS_H_

#include <cuda.h>
#include <cublas_v2.h>
#include <cufft.h>
#include "kernel.h"
#define WAVENUM 20
void initCuHandle(wave_para_t* wave_para_p, int waveNum);
void init_pulsecompress_fft_gpu(wave_para_t* wave_para_p);
void pulsecompress_fft_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p);
void pulsecompress_ifft_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p);
void pulsecompress_fft_three_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p);
void pulsecompress_ifft_three_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p);
void init_mtd_fft_gpu(wave_para_t* wave_para_p);
void mtd_fft_gpu(cuComplex* input_data, cuComplex* output_data, const wave_para_t wave_para_p);
void destroyCuHandle(void);
void initCuHandle_doppler(void);
void mtd_fft_gpu_512(cuComplex *input_data, cuComplex *output_data);
void pulsecompress_fft_gpu_512(cuComplex *input_data, cuComplex *output_data);
void pulsecompress_ifft_gpu_512(cuComplex *input_data, cuComplex *output_data);
#endif /* INC_BLAS_H_ */
