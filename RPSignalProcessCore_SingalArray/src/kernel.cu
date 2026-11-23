#include <nppi.h>
#include <stdio.h>
#include "../include/kernel.h"
#include <math.h>
#include"cmath"
#include "../include/cufft.h"
#include <cublas_v2.h>

#if CUDART_VERSION >= 13000
static NppStreamContext BuildDefaultNppStreamContext()
{
    static thread_local int cachedDeviceId = -1;
    static thread_local NppStreamContext cachedContext{};
    int deviceId = 0;
    cudaGetDevice(&deviceId);
    if (cachedDeviceId == deviceId)
    {
        return cachedContext;
    }

    NppStreamContext ctx{};
    ctx.hStream = 0;
    ctx.nCudaDeviceId = deviceId;

    cudaDeviceProp prop{};
    cudaGetDeviceProperties(&prop, deviceId);
    ctx.nMultiProcessorCount = prop.multiProcessorCount;
    ctx.nMaxThreadsPerMultiProcessor = prop.maxThreadsPerMultiProcessor;
    ctx.nMaxThreadsPerBlock = prop.maxThreadsPerBlock;
    ctx.nSharedMemPerBlock = prop.sharedMemPerBlock;
    ctx.nCudaDevAttrComputeCapabilityMajor = prop.major;
    ctx.nCudaDevAttrComputeCapabilityMinor = prop.minor;

    unsigned int streamFlags = 0;
    cudaStreamGetFlags(0, &streamFlags);
    ctx.nStreamFlags = streamFlags;
    ctx.nReserved0 = 0;
    cachedContext = ctx;
    cachedDeviceId = deviceId;
    return cachedContext;
}
#endif

//matric invert init
extern cublasHandle_t blasHandle;
int *P, *INFO;
cuComplex **A;
cuComplex **A_d, *A_dflat;
cuComplex **C;
cuComplex **C_d, *C_dflat;

//max batchSize = 3228 (pulse_length)
void initCaponInvertVar()
{
    uint n = MCAPONORDER;
    uint batchSize = 4388;

    cudaMalloc(&P, n * batchSize * sizeof(int));
    cudaMalloc(&INFO,  batchSize * sizeof(int));

    A = (cuComplex **)malloc(batchSize*sizeof(cuComplex *));
    cudaMalloc(&A_d,batchSize*sizeof(cuComplex *));
    cudaMalloc(&A_dflat, n*n*batchSize*sizeof(cuComplex));

    A[0] = A_dflat;
    for (int i = 1; i < batchSize; i++)
        A[i] = A[i-1]+(n*n);
    cudaMemcpy(A_d,A,batchSize*sizeof(cuComplex *),cudaMemcpyHostToDevice);

    C= (cuComplex **)malloc(batchSize*sizeof(cuComplex *));
    cudaMalloc(&C_d,batchSize*sizeof(cuComplex *));
    cudaMalloc(&C_dflat, n*n*batchSize*sizeof(cuComplex));

    C[0] = C_dflat;
    for (int i = 1; i < batchSize; i++)
        C[i] = C[i-1] + (n*n);
    cudaMemcpy(C_d,C,batchSize*sizeof(cuComplex *),cudaMemcpyHostToDevice);
}

void destroyCaponInvertVar()
{
    cudaFree(P);
    cudaFree(INFO);
    cudaFree(A_d); cudaFree(A_dflat); free(A);
    cudaFree(C_d); cudaFree(C_dflat); free(C);
}

int iDivUp(int a, int b)
{
    return (a % b != 0) ? (a / b + 1) : (a / b);
}

__device__ void CalcParabolaVertex(int x1, float y1, int x2, float y2, int x3, float y3, float* xv, float* yv)
{
    float denom = (x1 - x2) * (x1 - x3) * (x2 - x3);
    float A     = (x3 * (y2 - y1) + x2 * (y1 - y3) + x1 * (y3 - y2)) / denom;
    float B     = (x3*x3 * (y1 - y2) + x2*x2 * (y3 - y1) + x1*x1 * (y2 - y3)) / denom;
    float C     = (x2 * x3 * (x2 - x3) * y1 + x3 * x1 * (x3 - x1) * y2 + x1 * x2 * (x1 - x2) * y3) / denom;

    *xv = -B / (2*A);
    *yv = C - B*B / (4*A);
}

__global__ void sumArraysOnGPU(float* A, float* B, float* C, const int N)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < N)
    {
        C[i] = A[i] + B[i];
    }
}

//下变频
__global__ void DDConGPU_kernal(cufftComplex* din, cufftComplex* fir, cufftComplex* dout, int nx, int ny)
{
    int ix = threadIdx.x + blockIdx.x * blockDim.x;
    int iy = threadIdx.y + blockIdx.y * blockDim.y;
    int idx = iy * nx + ix;
    if (ix < nx && iy < ny)
    {
        dout[idx].x = din[idx].x * fir[idx].x - din[idx].y * fir[idx].y;
        dout[idx].y = din[idx].x * fir[idx].y + din[idx].y * fir[idx].x;
    }
    return;
}

//一维卷积实现滤波
__global__ void LPFonGPU_kernal(cufftComplex* Idata0, cufftComplex* Idata1, cufftComplex* Idata2, float* lpf, cufftComplex* Odata0, cufftComplex* Odata1, cufftComplex* Odata2, int lpf_length, int nx, int ny)
{
    int ix = threadIdx.x + blockIdx.x * blockDim.x;
    int iy = threadIdx.y + blockIdx.y * blockDim.y;
    int idx = iy * nx + ix;
    int data_length = nx * ny;
    if(ix >= nx || iy >= ny)
    {
        return;
    }
    cufftComplex value0 = {0, 0};
    cufftComplex value1 = {0, 0};
    cufftComplex value2 = {0, 0};
    int N_start_point = idx - (lpf_length / 2);
    int j;
    for(j = 0; j < lpf_length; j++)
    {
        if(N_start_point + j >= 0 && N_start_point + j < data_length)
        {
            value0.x += Idata0[N_start_point + j].x * lpf[j];
            value0.y += Idata0[N_start_point + j].y * lpf[j];
            value1.x += Idata1[N_start_point + j].x * lpf[j];
            value1.y += Idata1[N_start_point + j].y * lpf[j];
            value2.x += Idata2[N_start_point + j].x * lpf[j];
            value2.y += Idata2[N_start_point + j].y * lpf[j];
        }
    }
    Odata0[idx] = value0;
    Odata1[idx] = value1;
    Odata2[idx] = value2;
    return;
}

__global__ void cuda_abs_kernel(cuComplex* src_data, int width, int height, float* dst_data)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;
    if (c < width && r < height)
    {
        dst_data[r * width + c] = cuCabsf(src_data[r * width + c]);
        //dst_data[r*width + c] = src_data[r*width + c].x*src_data[r*width + c].x+src_data[r*width + c].y*src_data[r*width + c].y;
//        if(c == 658 && r == 49)
//        {
//            printf("104   %f\n",dst_data[r * width + c]);
//        }
    }
}

__global__ void integral_row_kernel(float* src_data, int width, int height, float* dst_data,
                                    int refer_left_width, int refer_right_width, int guard_left_width, int guard_right_width)
{
    int r = blockDim.x * blockIdx.x + threadIdx.x;
    int c;
    if (r >= height)
    {
        return;
    }
    float rs = 0.f;
    for (c = 0; c < refer_left_width - guard_left_width; c++)
    {
        rs += src_data[r * width + c];
    }
    for (c = refer_left_width + 1 + guard_right_width; c < refer_left_width + 1 + refer_right_width; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + refer_left_width] = rs;
    for (c = refer_left_width + 1; c < width - refer_right_width; c++)
    {
        dst_data[r * width + c] = dst_data[r * width + c - 1] - src_data[r * width + c - refer_left_width - 1] + src_data[r * width + c + refer_right_width] +
                                  src_data[r * width + c - guard_left_width - 1] - src_data[r * width + c + guard_right_width];
    }
}

__global__ void integral_row_kernel3(float* src_data, int width, int height, float* dst_data,
                                     int refer_left_width, int refer_right_width, int guard_left_width, int guard_right_width)
{
    int r = blockDim.x * blockIdx.x + threadIdx.x;
    int c;
    if (r >= height)
    {
        return;
    }
    float rs = 0.f;
    for (c = refer_left_width + 1 + guard_right_width; c < refer_left_width + 1 + refer_right_width; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + refer_left_width] = rs * 2;
    for (c = refer_left_width + 1; c < refer_left_width * 2; c++)
    {
        dst_data[r * width + c] = dst_data[(r) * width + c - 1] + src_data[(r) * width + c + refer_right_width] * 2 - src_data[(r) * width + c + guard_right_width] * 2;
    }
    rs = 0.f;
    for (c = refer_left_width; c < refer_left_width * 2 - guard_left_width; c++)
    {
        rs += src_data[r * width + c];
    }
    for (c = refer_left_width * 2 + 1 + guard_right_width; c < refer_left_width * 2 + 1 + refer_right_width; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + refer_left_width * 2] = rs;
    for (c = refer_left_width * 2 + 1; c < width - refer_right_width * 2; c++)
    {
        dst_data[r * width + c] = dst_data[(r) * width + c - 1] - src_data[(r) * width + c - refer_left_width - 1] + src_data[(r) * width + c + refer_right_width] +
                                  src_data[(r) * width + c - guard_left_width - 1] - src_data[(r) * width + c + guard_right_width];
    }
    rs = 0.f;
    for (c = width - refer_right_width * 2 - refer_left_width; c < width - refer_right_width * 2 - guard_left_width; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + (width - refer_right_width * 2)] = rs * 2;
    for (c = width - refer_right_width * 2 + 1; c < width - refer_right_width; c++)
    {
        dst_data[r * width + c] = dst_data[(r) * width + c - 1] - src_data[(r) * width + c - refer_left_width - 1] * 2 + src_data[(r) * width + c - guard_left_width - 1] * 2;
    }
}

__global__ void integral_row_kernel4(float* src_data, int width, int height, float* dst_data,
                                     int refer_left_width, int refer_right_width, int guard_left_width, int guard_right_width)
{
    int r = blockDim.x * blockIdx.x + threadIdx.x;
    int c;
    int i;
    if (r >= height)
    {
        return;
    }
    float rs = 0.f;
    for (c = guard_right_width + 1; c <= refer_right_width; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + 0] = rs * 2;
    for (c = 1; c < refer_left_width; c++)
    {
        dst_data[r * width + c] = dst_data[r * width + c - 1] + src_data[r * width + c + refer_right_width] * 2 - src_data[r * width + c + guard_right_width] * 2;
    }
    rs = 0.f;
    for (c = 0; c < refer_left_width - guard_left_width; c++)
    {
        rs += src_data[r * width + c];
    }
    for (c = refer_left_width + guard_right_width + 1; c < refer_left_width + refer_right_width + 1; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + refer_left_width] = rs;
    for (c = refer_left_width + 1; c < width - refer_right_width; c++)
    {
        dst_data[r * width + c] = dst_data[r * width + c - 1] - src_data[r * width + c - refer_left_width - 1] + src_data[r * width + c + refer_right_width] +
                                  src_data[r * width + c - guard_left_width - 1] - src_data[r * width + c + guard_right_width];
    }
    rs = 0.f;
    for (c = width - refer_right_width - refer_left_width; c < width - refer_right_width - guard_left_width; c++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[r * width + (width - refer_right_width)] = rs * 2;
    for (c = width - refer_right_width + 1; c < width; c++)
    {
        dst_data[r * width + c] = dst_data[(r) * width + c - 1] - src_data[(r) * width + c - refer_left_width - 1] * 2 + src_data[(r) * width + c - guard_left_width - 1] * 2;
    }
    /*for (c = 1;c <= refer_left_width + guard_left_width;c++)
    {
        for (i = 1;i <= refer_left_width;i++)
        {
            rs = rs + src_data[r*width + c + guard_right_width + i];
        }
        dst_data[r * width + c] = rs * 2;
    }
    rs = 0.f;
    for(c = refer_left_width + guard_left_width + 1;c <= width-refer_left_width-guard_left_width;c++)
    {
        for(i = 1;i <= refer_left_width;i++)
        {
            rs = rs + src_data[r*width + c - guard_left_width - i] + src_data[r*width + c +guard_left_width + i];
        }
        dst_data[r * width + c] = rs;
    }
    rs = 0.f;
    for (c = width - guard_left_width -refer_left_width + 1;c <= width;c++)
    {
        for (i = 1;i <= refer_left_width;i++)
        {
            rs = rs + src_data[r*width + c - guard_right_width - i];
        }
        dst_data[r * width + c] = rs * 2;
    }*/
}

__global__ void integral_column_kernel(float* src_data, int width, int height, float* dst_data,
                                       int refer_top_height, int refer_bottom_height, int guard_top_height, int guard_bottom_height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r;
    if (c >= width)
    {
        return;
    }
    float rs = 0.f;
    for (r = 0; r < refer_top_height - guard_top_height; r++)
    {
        rs += src_data[r * width + c];
    }
    for (r = refer_top_height + 1 + guard_bottom_height; r < refer_top_height + 1 + refer_bottom_height; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[refer_top_height * width + c] += rs;
    for (r = refer_top_height + 1; r < height - refer_bottom_height; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] - src_data[(r - refer_top_height - 1) * width + c] + src_data[(r + refer_bottom_height) * width + c] +
                                  src_data[(r - guard_top_height - 1) * width + c] - src_data[(r + guard_bottom_height) * width + c];
    }
}

__global__ void integral_column_kernel2(float* src_data, int width, int height, float* dst_data,
                                        int refer_top_height, int refer_bottom_height, int guard_top_height, int guard_bottom_height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r;
    if (c >= width)
    {
        return;
    }
    float rs = 0.f;
    for (r = 0; r < refer_top_height - guard_top_height; r++)
    {
        rs += src_data[r * width + c];
    }
    for (r = refer_top_height + 1 + guard_bottom_height; r < refer_top_height + 1 + refer_bottom_height; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[refer_top_height * width + c] = rs;
    for (r = refer_top_height + 1; r < height - refer_bottom_height; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] - src_data[(r - refer_top_height - 1) * width + c] + src_data[(r + refer_bottom_height) * width + c] +
                                  src_data[(r - guard_top_height - 1) * width + c] - src_data[(r + guard_bottom_height) * width + c];
    }
}

__global__ void integral_column_kernel3(float* src_data, int width, int height, float* dst_data,
                                        int refer_top_height, int refer_bottom_height, int guard_top_height, int guard_bottom_height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r;
    if (c >= width)
    {
        return;
    }
    float rs = 0.f;
    for (r = refer_top_height + 1 + guard_bottom_height; r < refer_top_height + 1 + refer_bottom_height; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[refer_top_height * width + c] = rs * 2;
    for (r = refer_top_height + 1; r < refer_top_height * 2; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] + src_data[(r + refer_bottom_height) * width + c] * 2 - src_data[(r + guard_bottom_height) * width + c] * 2;
    }
    rs = 0.f;
    for (r = refer_top_height; r < refer_top_height * 2 - guard_top_height; r++)
    {
        rs += src_data[r * width + c];
    }
    for (r = refer_top_height * 2 + 1 + guard_bottom_height; r < refer_top_height * 2 + 1 + refer_bottom_height; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[refer_top_height * 2 * width + c] = rs;
    for (r = refer_top_height * 2 + 1; r < height - refer_bottom_height * 2; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] - src_data[(r - refer_top_height - 1) * width + c] + src_data[(r + refer_bottom_height) * width + c] +
                                  src_data[(r - guard_top_height - 1) * width + c] - src_data[(r + guard_bottom_height) * width + c];
    }
    rs = 0.f;
    for (r = height - refer_bottom_height * 2 - refer_top_height; r < height - refer_bottom_height * 2 - guard_top_height; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[(height - refer_bottom_height * 2) * width + c] = rs * 2;
    for (r = height - refer_bottom_height * 2 + 1; r < height - refer_bottom_height; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] - src_data[(r - refer_top_height - 1) * width + c] * 2 + src_data[(r - guard_top_height - 1) * width + c] * 2;
    }
}

__global__ void integral_column_kernel4(float* src_data, int width, int height, float* dst_data,
                                        int refer_top_height, int refer_bottom_height, int guard_top_height, int guard_bottom_height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r;
    int i;
    if (c >= width)
    {
        return;
    }
    float rs = 0.f;
    for (r = guard_bottom_height + 1; r < refer_bottom_height + 1; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[0 * width + c] = rs * 2;
    for (r = 1; r < refer_top_height; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] + src_data[(r + refer_bottom_height) * width + c] * 2 - src_data[(r + guard_bottom_height) * width + c] * 2;
    }
    rs = 0.f;
    for (r = 0; r < refer_top_height - guard_top_height; r++)
    {
        rs += src_data[r * width + c];
    }
    for (r = refer_top_height + guard_bottom_height + 1; r < refer_top_height + refer_bottom_height + 1; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[refer_top_height * width + c] = rs;
    for (r = refer_top_height + 1; r < height - refer_bottom_height; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] - src_data[(r - refer_top_height - 1) * width + c] + src_data[(r + refer_bottom_height) * width + c] +
                                  src_data[(r - guard_top_height - 1) * width + c] - src_data[(r + guard_bottom_height) * width + c];
    }
    rs = 0.f;
    for (r = height - refer_bottom_height - refer_top_height; r < height - refer_bottom_height - guard_top_height; r++)
    {
        rs += src_data[r * width + c];
    }
    dst_data[(height - refer_bottom_height)*width + c] = rs * 2;
    for (r = height - refer_bottom_height + 1; r < height; r++)
    {
        dst_data[r * width + c] = dst_data[(r - 1) * width + c] - src_data[(r - refer_top_height - 1) * width + c] * 2 + src_data[(r - guard_top_height - 1) * width + c] * 2;
    }
    /*for (r = 1;r <= refer_top_height + guard_top_height;r++)
    {
        for(i = 1; i<=refer_top_height;i++)
        {
            rs = rs + src_data[(r + guard_top_height + i )* width + c ];
        }
        dst_data[r * width + c] = rs*2;
    }
    rs = 0.f;
    for(r = refer_top_height + guard_top_height + 1;r <= height - refer_top_height - guard_top_height;r++)
    {
        for(i = 1; i<= refer_top_height;i++)
        {
            rs = rs + src_data[(r - guard_top_height - i )* width + c ] + src_data[(r + guard_top_height + i )* width + c ];
        }
        dst_data[r * width + c] = rs;
    }
    rs = 0.f;
    for (r = height - refer_top_height - guard_top_height + 1;r <= height;r++)
    {
        for(i = 1; i<= refer_top_height;i++)
        {
            rs = rs + src_data[(r - guard_top_height - i )* width + c ];
        }
        dst_data[r * width + c] = rs*2;
    }*/
}

__global__ void mirror_row_kernel(float* src_dst_data, int width, int height, int border_top_height, int border_bottom_height)
{
    int r = blockDim.x * blockIdx.x + threadIdx.x;
    int c;
    if (r >= height)
    {
        return;
    }
    if (r >= 0 && (r <= border_top_height - 1))
    {
        for (c = 0; c < width; c++)
        {
            src_dst_data[r * width + c] = src_dst_data[(2 * border_top_height - 1 - r) * width + c];
        }
    }
    if (r >= height - border_bottom_height && (r <= height - 1))
    {
        for (c = 0; c < width; c++)
        {
            src_dst_data[r * width + c] = src_dst_data[(2 * (height - border_bottom_height) - 1 - r) * width + c];
        }
    }
}

__global__ void mirror_column_kernel(float* src_dst_data, int width, int height, int border_left_width, int border_right_width)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r;
    if (c >= width)
    {
        return;
    }
    if (c >= 0 && (c <= border_left_width - 1))
    {
        for (r = 0; r < height; r++)
        {
            src_dst_data[r * width + c] = src_dst_data[r * width + (2 * border_left_width - 1 - c)];
        }
    }
    if (c >= width - border_right_width && (c <= width - 1 ))
    {
        for (r = 0; r < height; r++)
        {
            src_dst_data[r * width + c] = src_dst_data[ r * width + (2 * (width - border_right_width) - 1 - c)];
        }
    }
}

void mirror_matrix(float* src_dst_data, int width, int height,
                   int border_top_height, int border_bottom_height, int border_left_width, int border_right_width)
{
    dim3 blocks_num1(iDivUp(height, BLOCK_DIM), 1, 1);
    dim3 blocks_num2(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    mirror_row_kernel <<< blocks_num1, threads_per_block >>> (src_dst_data, width, height, border_top_height, border_bottom_height);
    mirror_column_kernel <<< blocks_num2, threads_per_block >>> (src_dst_data, width, height, border_left_width, border_right_width);
}

void pad_matrix_mirror(float* src_data, float* dst_data, wave_para_t* wave_para_p, cfar_para_t* cfar_para_p)
{
    int width = wave_para_p->pulse_length;
    int height = wave_para_p->fft_number;
    int ref_left_width = cfar_para_p->refer_left_width;
    int ref_right_width = cfar_para_p->refer_right_width;
    int ref_top_height = cfar_para_p->refer_top_height;
    int ref_bottom_height = cfar_para_p->refer_bottom_height;
    int border_width = width + ref_left_width + ref_right_width;
    int border_height = height + ref_top_height + ref_bottom_height;
    // add border
    NppiSize size1;
    size1.width = width;
    size1.height = height;
    NppiSize size2;
    size2.width = border_width;
    size2.height = border_height;
#if CUDART_VERSION >= 13000
    NppStreamContext nppCtx = BuildDefaultNppStreamContext();
    nppiCopyConstBorder_32f_C1R_Ctx(src_data, width * sizeof(float), size1,
                                    dst_data, border_width * sizeof(float), size2,
                                    ref_top_height, ref_left_width, 0, nppCtx);
#else
    nppiCopyConstBorder_32f_C1R(src_data, width * sizeof(float), size1,
                                dst_data, border_width * sizeof(float), size2,
                                ref_top_height, ref_left_width, 0);
#endif
    // mirror border
    mirror_matrix(dst_data, border_width, border_height,
                  ref_top_height, ref_bottom_height, ref_left_width, ref_right_width);
}

/*
__global__ __inline float box_integral(float *data, int width, int height, int r1, int r2, int c1, int c2)
{
	float a, b, c, d;

	a = data[r1 * width + c1];
	b = data[r1 * width + c2];
	c = data[r2 * width + c1];
	d = data[r2 * width + c2];

	return a - b - c + d;
}
 */
#define box_integral(data, width, height, r1, r2, c1, c2) \
    ( (data)[(r1) * (width) + (c1)] -\
      (data)[(r1) * (width) + (c2)] - \
      (data)[(r2) * (width) + (c1)] + \
      (data)[(r2) * (width) + (c2)] )

__global__ void ca_cfar_2d_rec_kernel(float* src_data, int width, int height, float* integral_image, float alpha,
                                      int refer_left_width, int refer_right_width, int refer_top_height, int refer_bottom_height,
                                      int guard_left_width, int guard_right_width, int guard_top_height, int guard_bottom_height,
                                      float* dst_data)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;
    float mean, t;
    const float EPS = 1e-8f;
    int index;
    if(c >= width || r >= height)
    {
        return;
    }
    int n = (refer_left_width - guard_left_width) + (refer_right_width - guard_right_width) +
            (refer_top_height - guard_top_height) + (refer_bottom_height - guard_bottom_height);
    if (c >= refer_left_width && c <= width - refer_right_width - 1)
    {
        if (r >= refer_top_height && r <= height - refer_bottom_height - 1)
        {
            mean = integral_image[r * width + c] / n;
            t = alpha * mean;
            index = (r - refer_top_height) * width + (c - refer_left_width);
            dst_data[index] = (src_data[index] - t > EPS) ? 1 : 0;
            //dst_data[(r - refer_top_height)*w + (c - refer_left_width)] = mean;
        }
    }
}



__global__ void ca_cfar_2d_cross_kernel(float* src_data, int width, int height, float* integral_image_row, float* integral_image_column,
                                        const sp_para_t SP_para_current,const wave_para_t wave_para_p,
                                        int refer_left_width, int refer_right_width, int refer_top_height, int refer_bottom_height,
                                        int guard_left_width, int guard_right_width, int guard_top_height, int guard_bottom_height,
                                        unsigned char* dst_data)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;
    float mean_row, mean_column;
    const float EPS = 1e-8f;
    int n_row = (refer_left_width - guard_left_width) + (refer_right_width - guard_right_width);
    int n_column = (refer_top_height - guard_top_height) + (refer_bottom_height - guard_bottom_height);
    //int n_row = refer_left_width * 2;
    //int n_column = refer_top_height * 2;

    if(r>=height || c>=width)
    {
        return;
    }
    const float range_resolution = 300/(2 * wave_para_p.sampleFreqMHz);
    float current_range, cfar_th_r, cfar_th_v;
    current_range = c * range_resolution;
    if ((current_range >= 0.0f) && (current_range <= 1000.0f))
    {
        cfar_th_r = SP_para_current.cfar_th_r1;
        cfar_th_v = SP_para_current.cfar_th_v1;
    }
    else if ((current_range > 1000.0f) && (current_range <= 2000.0f))
    {
        cfar_th_r = SP_para_current.cfar_th_r2;
        cfar_th_v = SP_para_current.cfar_th_v2;
    }
    else
    {
        cfar_th_r = SP_para_current.cfar_th_r3;
        cfar_th_v = SP_para_current.cfar_th_v3;
    }
    if(SP_para_current.v_th_valid == true)
    {
//        if(r<SP_para_current.v_th_No_min || r>height-SP_para_current.v_th_No_min || (r>SP_para_current.v_th_No_max && r<height-SP_para_current.v_th_No_max))
//        {
//            return;
//        }
        if((r<(height/2 - SP_para_current.v_th_No_max))
                || (r>(height/2+SP_para_current.v_th_No_max))
                || ((r>(height/2 - SP_para_current.v_th_No_min))&&(r<(height/2+SP_para_current.v_th_No_min))))
        {
            return;
        }
    }

    if(SP_para_current.r_th_valid == true)
    {
        if(c<SP_para_current.r_th_No_min || c>SP_para_current.r_th_No_max)
        {
            return;
        }
    }

    mean_row = integral_image_row[r * width + c] * cfar_th_r / n_row;
    mean_column = integral_image_column[r * width + c] * cfar_th_v / n_column;
    dst_data[r * width + c] = ((src_data[r * width + c] - mean_row > EPS) && (src_data[r * width + c] - mean_column > EPS) && (src_data[r * width + c] > SP_para_current.snr_th)) ? 1 : 0;// && (src_data[r * width + c] > 5e4)
}

//矩形窗
void ca_cfar_2d_rec(float* src_data, int width, int height, float* padded_data, float* integral_image, float pfa,
                    int refer_left_width, int refer_right_width, int refer_top_height, int refer_bottom_height,
                    int guard_left_width, int guard_right_width, int guard_top_height, int guard_bottom_height,
                    float* dst_data)
{
    dim3 blocks_num1(iDivUp(height, BLOCK_DIM), 1, 1);
    dim3 blocks_num2(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    integral_row_kernel <<< blocks_num1, threads_per_block >>> (padded_data, width, height, integral_image,
            refer_left_width, refer_right_width, guard_left_width, guard_right_width);
    integral_column_kernel <<< blocks_num2, threads_per_block >>> (padded_data, width, height, integral_image,
            refer_top_height, refer_bottom_height, guard_top_height, guard_bottom_height);
    int n = (refer_left_width - guard_left_width) + (refer_right_width - guard_right_width) +
            (refer_top_height - guard_top_height) + (refer_bottom_height - guard_bottom_height);
    float alpha = n * (powf(pfa, -1.0f / n) - 1);
    dim3 blocks_num_cfar(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block_cfar(BLOCK_2DIM, BLOCK_2DIM, 1);
    ca_cfar_2d_rec_kernel <<< blocks_num_cfar, threads_per_block_cfar>>> (src_data, width, height, integral_image, alpha,
            refer_left_width, refer_right_width, refer_top_height, refer_bottom_height,
            guard_left_width, guard_right_width, guard_top_height, guard_bottom_height,
            dst_data);
}

//十字形窗，行和列分开计算
void calculate_integral_cross(int width, int height, float* padded_data, float* integral_image_row, float* integral_image_column, cfar_para_t* cfar_para_p)
{
    int refer_left_width = cfar_para_p->refer_left_width;
    int refer_right_width = cfar_para_p->refer_right_width;
    int refer_top_height = cfar_para_p->refer_top_height;
    int refer_bottom_height = cfar_para_p->refer_bottom_height;
    int guard_left_width = cfar_para_p->guard_left_width;
    int guard_right_width = cfar_para_p->guard_right_width;
    int guard_top_height = cfar_para_p->guard_top_height;
    int guard_bottom_height = cfar_para_p->guard_bottom_height;
    dim3 blocks_num1(iDivUp(height, BLOCK_DIM), 1, 1);
    dim3 blocks_num2(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    integral_row_kernel3 <<< blocks_num1, threads_per_block >>> (padded_data, width, height, integral_image_row,
            refer_left_width, refer_right_width, guard_left_width, guard_right_width);
    integral_column_kernel3 <<< blocks_num2, threads_per_block >>> (padded_data, width, height, integral_image_column,
            refer_top_height, refer_bottom_height, guard_top_height, guard_bottom_height);
}

//距离维点迹凝聚on GPU
//连续PLOT_CENTROID_TH_R点过门限，取幅度最高的
__global__ void plot_centroid_range_kernel(unsigned char *bool_data,float* mtd_abs_sum, const wave_para_t wave_para_p)
{
    unsigned int r = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int c = 0;
    unsigned int width = wave_para_p.pulse_length;
    unsigned int height = wave_para_p.fft_number;

    unsigned int continue_plots_cnt = 0;
    unsigned int max_amp_No;
    float max_amp;

    if (r >= height)
    {
        return;
    }

    for(c=0; c<width; c++)
    {
        if(bool_data[r*width+c] == 1 && continue_plots_cnt < PLOT_CENTROID_TH_R)
        {
            continue_plots_cnt++;
        }
        else if(bool_data[r*width+c] == 1 && continue_plots_cnt == PLOT_CENTROID_TH_R)
        {
            continue_plots_cnt = 1;
        }
        else
        {
            continue_plots_cnt = 0;
        }

        if(continue_plots_cnt == 1)
        {
            max_amp_No = c;
            max_amp = mtd_abs_sum[r*width+c];
        }
        else if(continue_plots_cnt > 1)
        {
            if(mtd_abs_sum[r*width+c]>max_amp)
            {
                bool_data[r*width+max_amp_No] = 0;
                max_amp_No = c;
                max_amp = mtd_abs_sum[r*width+c];
            }
            else
            {
                bool_data[r*width+c] = 0;
            }
        }
    }
}

//速度维点迹凝聚on GPU
//连续PLOT_CENTROID_TH_V点过门限，取幅度最高的
__global__ void plot_centroid_velocity_kernel(unsigned char *bool_data,float* mtd_abs_sum, const wave_para_t wave_para_p)
{
    unsigned int c = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int r = 0;

    unsigned int width = wave_para_p.pulse_length;
    unsigned int height = wave_para_p.fft_number;

    unsigned int continue_plots_cnt = 0;
    unsigned int max_amp_No;
    float max_amp;

    if (c >= width)
    {
        return;
    }

    for(r=0; r<height; r++)
    {
        if(bool_data[r*width+c] == 1 && continue_plots_cnt < PLOT_CENTROID_TH_V)
        {
            continue_plots_cnt++;
        }
        else if(bool_data[r*width+c] == 1 && continue_plots_cnt == PLOT_CENTROID_TH_V)
        {
            continue_plots_cnt = 1;
        }
        else
        {
            continue_plots_cnt = 0;
        }

        if(continue_plots_cnt == 1)
        {
            max_amp_No = r;
            max_amp = mtd_abs_sum[r*width+c];
        }
        else if(continue_plots_cnt > 1)
        {
            if(mtd_abs_sum[r*width+c]>max_amp)
            {
                bool_data[max_amp_No*width+c] = 0;
                max_amp_No = r;
                max_amp = mtd_abs_sum[r*width+c];
            }
            else
            {
                bool_data[r*width+c] = 0;
            }
        }
    }
}

//距离维点迹凝聚on GPU
//相邻两个过门限点取幅度最高
__global__ void plot_centroid_range_kernal111()
{

}

//点迹凝聚
void plot_centroid(unsigned char *bool_data,float* mtd_abs_sum, const wave_para_t wave_para_p)
{
    int width = wave_para_p.pulse_length;
    int height = wave_para_p.fft_number;

    dim3 blocks_num1(iDivUp(height, BLOCK_DIM), 1, 1);
    dim3 blocks_num2(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);

    //距离维凝聚
    plot_centroid_range_kernel <<< blocks_num1, threads_per_block >>> (bool_data, mtd_abs_sum, wave_para_p);
    //速度维凝聚
    plot_centroid_velocity_kernel <<< blocks_num2, threads_per_block >>> (bool_data, mtd_abs_sum, wave_para_p);

}

__device__ void lock(int* mutex)
{
    while(atomicCAS(mutex, 0, 1) != 0);
}

__device__ void unlock(int* mutex)
{
    atomicExch(mutex, 0);
}


//二次下变频
//lORs_flag: 0-长脉冲  1-短脉冲
void sec_ddc(cuComplex* echo_sum, cuComplex* echo_azi, cuComplex* echo_ele,
             cuComplex* lpf_out_sum, cuComplex* lpf_out_azi, cuComplex* lpf_out_ele,
             cuComplex* ddc_coel_0l, cuComplex* ddc_coel_0s,
             cuComplex* ddc_coel_1l, cuComplex* ddc_coel_1s,
             float* lpf, int l_lpf,
             wave_para_t* wave_para_p, int lORs_flag)
{
    int width = wave_para_p->pulse_length;
    int height = wave_para_p->pulse_number;
    cuComplex* ddc_coel;
    switch(wave_para_p->waveCode)
    {
    case 0:
        ddc_coel = ddc_coel_0l;
        break;
    case 1:
        ddc_coel = ddc_coel_1l;
        break;
    case 9:
        ddc_coel = ddc_coel_0s;
        break;
    case 10:
        ddc_coel = ddc_coel_1s;
        break;
    default:
        return;
    }
    cuComplex* ddc_out_sum, *ddc_out_azi, *ddc_out_ele;
    cudaMalloc(&ddc_out_sum, sizeof(cuComplex)*width * height);
    cudaMalloc(&ddc_out_azi, sizeof(cuComplex)*width * height);
    cudaMalloc(&ddc_out_ele, sizeof(cuComplex)*width * height);

    dim3 blocks_num(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block(BLOCK_2DIM, BLOCK_2DIM, 1);
    DDConGPU_kernal <<< blocks_num, threads_per_block>>>(echo_sum, ddc_coel, lpf_out_sum, width, height);
    DDConGPU_kernal <<< blocks_num, threads_per_block>>>(echo_azi, ddc_coel, lpf_out_azi, width, height);
    DDConGPU_kernal <<< blocks_num, threads_per_block>>>(echo_ele, ddc_coel, lpf_out_ele, width, height);
    //DDConGPU_kernal <<< blocks_num, threads_per_block>>>(echo_sum, echo_fw, echo_fy, ddc_coel, ddc_out_sum, ddc_out_fw, ddc_out_fy, width, height);
    //LPFonGPU_kernal <<< blocks_num, threads_per_block>>>(ddc_out_sum, ddc_out_fw, ddc_out_fy, lpf, lpf_out_sum, lpf_out_fw, lpf_out_fy, l_lpf, width, height);
    cudaFree(ddc_out_sum);
    cudaFree(ddc_out_azi);
    cudaFree(ddc_out_ele);
    return;
}

__global__ void platformSpeedCompensationKernel(cuComplex* inData, cuComplex* outData, float platformDoppler,
                                                float samplingRateMHz, unsigned int prtLength, unsigned int pw_length, int width, int height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;
    int id = r * width + c;

    float tTemp;
    cuComplex compensationValue;

    if ((c < width) && (r < height))
    {
        tTemp = (r * prtLength + pw_length + DDS_R_TDELAY_NUM + c) / samplingRateMHz/1e6;   //每个脉冲前面会扣掉pw_length + DDS_R_TDELAY_NUM个采样点
        compensationValue.x = cos(-2*M_PI*platformDoppler * tTemp);
        compensationValue.y = sin(-2*M_PI*platformDoppler * tTemp);
        outData[id] = cuCmulf(inData[id], compensationValue);
    }

    return;
}

//平台速度补偿
void platformSpeedCompensation(cuComplex* inData, cuComplex* outData,
                               float platformSpeed, float radarFrequency, const wave_para_t ParaWave)
{
    int width = ParaWave.pulse_length;
    int height = ParaWave.pulse_number;
    float platformDoppler = 2 * platformSpeed * radarFrequency / 0.3; //radarFrequency:GHz
    unsigned int prtLength = floor(ParaWave.prt_us * ParaWave.sampleFreqMHz);
    unsigned int pw_length = floor(ParaWave.pw_us * ParaWave.sampleFreqMHz);
    int samplingRateMHz = ParaWave.sampleFreqMHz;

    dim3 blocks_num(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block(BLOCK_2DIM, BLOCK_2DIM, 1);
    platformSpeedCompensationKernel <<< blocks_num, threads_per_block>>> (inData, outData, platformDoppler,
                                                                       samplingRateMHz, prtLength, pw_length, width, height);
    return;
}

//扩展脉压长度
void pad_pc_data(float* src_echo_data, float* dst_data, unsigned int width , unsigned int height, unsigned int border_width)
{
    //int width = wave_para_p.pulse_length;
    //int border_width = wave_para_p.fft_length;
    //int height = wave_para_p.pulse_number;
    // add border
    NppiSize size1;
    size1.width = width * 2;
    size1.height = height;
    NppiSize size2;
    size2.width = border_width * 2;
    size2.height = height;

#if CUDART_VERSION >= 13000
    NppStreamContext nppCtx = BuildDefaultNppStreamContext();
    nppiCopyConstBorder_32f_C1R_Ctx(reinterpret_cast<Npp32f *>(src_echo_data), width * sizeof(cuComplex), size1,
                                    reinterpret_cast<Npp32f *>(dst_data), border_width * sizeof(cuComplex), size2, 0, 0, 0, nppCtx);
#else
    nppiCopyConstBorder_32f_C1R(src_echo_data, width * sizeof(cuComplex), size1,
                                dst_data, border_width * sizeof(cuComplex), size2, 0, 0, 0);
#endif

    return;
}

__global__ void pad_pc_data_three_kernel(const cuComplex* sum, const cuComplex* azi,
                                         const cuComplex* ele, cuComplex* dst,
                                         unsigned int width, unsigned int height,
                                         unsigned int border_width)
{
    const unsigned int col = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int row = blockIdx.y * blockDim.y + threadIdx.y;
    if (col >= border_width || row >= 3 * height)
    {
        return;
    }

    cuComplex value = make_cuComplex(0.0f, 0.0f);
    if (col < width)
    {
        const cuComplex* src = row < height ? sum : (row < 2 * height ? azi : ele);
        value = src[(row % height) * width + col];
    }
    dst[row * border_width + col] = value;
}

void pad_pc_data_three(const cuComplex* sum, const cuComplex* azi, const cuComplex* ele,
                       cuComplex* dst, unsigned int width, unsigned int height, unsigned int border_width)
{
    dim3 threads(BLOCK_2DIM, BLOCK_2DIM);
    dim3 blocks(iDivUp(border_width, BLOCK_2DIM), iDivUp(3 * height, BLOCK_2DIM));
    pad_pc_data_three_kernel<<<blocks, threads>>>(sum, azi, ele, dst, width, height, border_width);
}

__global__ void pulse_compress_kernal(cuComplex* src_data, cuComplex* coel_data, cuComplex* dst_data, int width, int height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;
    int id = r * width + c;
    if ((c < width) && (r < height))
    {
        dst_data[id].x = (src_data[id].x * coel_data[c].x - src_data[id].y * coel_data[c].y)/width;
        dst_data[id].y = (src_data[id].x * coel_data[c].y + src_data[id].y * coel_data[c].x)/width;
    }
}

//脉压系数相乘
void pulse_compress(cuComplex* src_data, cuComplex* dst_data,cuComplex* coel_data, unsigned int height, unsigned int width)
{
    //int width = wave_para_p.fft_length;
    //int height = wave_para_p.pulse_number;
    cuComplex* coel_data_t;

    coel_data_t = coel_data;

    dim3 blocks_num(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block(BLOCK_2DIM, BLOCK_2DIM, 1);
    pulse_compress_kernal <<< blocks_num, threads_per_block>>> (src_data, coel_data_t, dst_data, width, height);
    return;
}

//加权均值滤波:加权
__global__ void SPWeighedMeanFilStep1Kernal(cuComplex* InData, cuComplex* OutData, float* CoelWin, int width, int height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;

    if (c < width && r < height)
    {
        OutData[r * width + c].x = InData[r * width + c].x * CoelWin[r];
        OutData[r * width + c].y = InData[r * width + c].y * CoelWin[r];
    }

    return;
}

//加权均值滤波:均值
//归并累加算法
__global__ void SPWeighedMeanFilStep2Kernal(cuComplex* InData, cuComplex* OutData, int width, int height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r;
    if(c >= width)
    {
        return;
    }
    for(r = 0; r < height; r++)
    {
        OutData[c].x += InData[r*width + c].x;
        OutData[c].y += InData[r*width + c].y;
    }
    return;

//    int c = blockDim.x * blockIdx.x + threadIdx.x;
//    int r = blockDim.y * blockIdx.y + threadIdx.y;
//    int s;
//    __shared__ cuComplex mem[1638400];
//    mem[r * width + c] = InData[r * width + c];     //将global　memory的数据复制到shared memory中
//    for(s = 1; s < height; s*=2)                //循环(>=log2N的最小整数 ）次,   S初始值为1，之后依次为2，4，8，16.....
//    {
//        if(r%(2*s) == 0 && (r+s) < height)                    //所有的偶数线程，开始为每隔两个，之后依次为每隔4个，8个，16个，32个
//        {
//            mem[r * width + c].x += mem[(r+s) * width + c].x;  //将上次循环的相邻两和相加
//            mem[r * width + c].y += mem[(r+s) * width + c].y;
//        }
//        __syncthreads();                            //线程同期（很重要，如果不同期，相邻的和就会读取错误）
//    }
//    if(r == 0)
//    {
//        OutData[c] = mem[c];                        //将和包存在global memory的首地址中
//    }
}

//加权均值滤波:滤波
__global__ void SPWeighedMeanFilStep3Kernal(cuComplex* InData, cuComplex* OutData, cuComplex* CoelMean, float* CoelWin, int width, int height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;

    if (c < width && r < height)
    {
        OutData[r * width + c].x = (InData[r * width + c].x - CoelMean[c].x)*CoelWin[r];
        OutData[r * width + c].y = (InData[r * width + c].y - CoelMean[c].y)*CoelWin[r];
    }

    return;
}

//加权均值滤波
void SPWeighedMeanFil(cuComplex* InData, cuComplex* OutData, float* win1[], float* win2[],const wave_para_t ParaWave , unsigned int width , unsigned int height)
{
    //int width = ParaWave.fft_length;
    //int height = ParaWave.pulse_number;
    float* CoelData1;
    float* CoelData2;

    CoelData1 = win1[ParaWave.waveCode];
    CoelData2 = win2[ParaWave.waveCode];

    cuComplex* WeighedData;
    cuComplex* MeanData;
    cudaMalloc(&WeighedData, sizeof(cuComplex)*width * height);
    cudaMalloc(&MeanData, sizeof(cuComplex)*width);
    cudaMemset(WeighedData, 0, sizeof(cuComplex)*width*height);
    cudaMemset(MeanData,0,sizeof (cuComplex)*width);


    dim3 blocks_num2d(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block2d(BLOCK_2DIM, BLOCK_2DIM, 1);
    dim3 blocks_num1d(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block1d(BLOCK_DIM, 1, 1);

    SPWeighedMeanFilStep1Kernal <<< blocks_num2d, threads_per_block2d >>> (InData, WeighedData, CoelData1, width, height);
    SPWeighedMeanFilStep2Kernal <<< blocks_num1d, threads_per_block1d >>> (WeighedData, MeanData, width, height);
    SPWeighedMeanFilStep3Kernal <<< blocks_num2d, threads_per_block2d >>> (InData, OutData, MeanData, CoelData2, width, height);

    cudaFree(WeighedData);
    cudaFree(MeanData);
    return;
}

void SPWeighedMeanFil_512(cuComplex* InData, cuComplex* OutData, float* win1, float* win2, unsigned int width , unsigned int height)
{
    //int width = ParaWave.fft_length;
    //int height = ParaWave.pulse_number;
    float* CoelData1;
    float* CoelData2;

    CoelData1 = win1;
    CoelData2 = win2;

    cuComplex* WeighedData;
    cuComplex* MeanData;
    cudaMalloc(&WeighedData, sizeof(cuComplex)*width * height);
    cudaMalloc(&MeanData, sizeof(cuComplex)*width);
    cudaMemset(WeighedData, 0, sizeof(cuComplex)*width*height);
    cudaMemset(MeanData,0,sizeof (cuComplex)*width);


    dim3 blocks_num2d(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block2d(BLOCK_2DIM, BLOCK_2DIM, 1);
    dim3 blocks_num1d(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block1d(BLOCK_DIM, 1, 1);

    SPWeighedMeanFilStep1Kernal <<< blocks_num2d, threads_per_block2d >>> (InData, WeighedData, CoelData1, width, height);
    SPWeighedMeanFilStep2Kernal <<< blocks_num1d, threads_per_block1d >>> (WeighedData, MeanData, width, height);
    SPWeighedMeanFilStep3Kernal <<< blocks_num2d, threads_per_block2d >>> (InData, OutData, MeanData, CoelData2, width, height);

    cudaFree(WeighedData);
    cudaFree(MeanData);
    return;
}

//加权均值滤波:加权
__global__ void SPMTDWinKernal(cuComplex* InData, cuComplex* OutData, float* CoelWin, int width, int height)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;

    if (c < width && r < height)
    {
        OutData[r * width + c].x = InData[r * width + c].x * CoelWin[r];
        OutData[r * width + c].y = InData[r * width + c].y * CoelWin[r];
    }

    return;
}

//MTD加窗
void SPMTDWin(cuComplex* InData, cuComplex* OutData, float* CoelWin, const wave_para_t ParaWave)
{
    int width = ParaWave.fft_length;
    int height = ParaWave.pulse_number;

    dim3 blocks_num2d(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block2d(BLOCK_2DIM, BLOCK_2DIM, 1);

    SPMTDWinKernal <<< blocks_num2d, threads_per_block2d >>> (InData, OutData, CoelWin, width, height);
}

void pad_mtd_data(float* src_data, float* dst_data, const wave_para_t wave_para_p)
{
    int width = wave_para_p.fft_length;
    int height = wave_para_p.pulse_number;
    int border_height = wave_para_p.fft_number;
    // add border
    NppiSize size1;
    size1.width = width * 2;
    size1.height = height;
    NppiSize size2;
    size2.width = width * 2;
    size2.height = border_height;
#if CUDART_VERSION >= 13000
    NppStreamContext nppCtx = BuildDefaultNppStreamContext();
    nppiCopyConstBorder_32f_C1R_Ctx(reinterpret_cast<Npp32f *>(src_data), width * sizeof(cuComplex), size1,
                                    reinterpret_cast<Npp32f *>(dst_data), width * sizeof(cuComplex), size2, 0, 0, 0, nppCtx);
#else
    nppiCopyConstBorder_32f_C1R(src_data, width * sizeof(cuComplex), size1,
                                dst_data, width * sizeof(cuComplex), size2, 0, 0, 0);
#endif
}

void cuda_abs(cuComplex* src_data, float* dst_data, const wave_para_t wave_para_p, unsigned int height, unsigned int width)
{
    //int width = wave_para_p.pulse_length;
    //int height = wave_para_p.fft_number;
    dim3 blocks_num(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block(BLOCK_2DIM, BLOCK_2DIM, 1);
    cuda_abs_kernel <<< blocks_num, threads_per_block >>> (src_data, width, height, dst_data);
}

//十字形窗，行和列分开计算
//void calculate_cross_serve(float* mtd_abs_data,
//                           float* integral_image_row, float* integral_image_column,
//                           unsigned int width,unsigned int height , const cfar_para_t cfar_para_p)
//{
//    //int width = wave_para_p.pulse_length;
//    //int height = wave_para_p.fft_number;
//    int refer_left_width = cfar_para_p.refer_left_width;
//    int refer_right_width = cfar_para_p.refer_right_width;
//    int refer_top_height = cfar_para_p.refer_top_height;
//    int refer_bottom_height = cfar_para_p.refer_bottom_height;
//    int guard_left_width = cfar_para_p.guard_left_width;
//    int guard_right_width = cfar_para_p.guard_right_width;
//    int guard_top_height = cfar_para_p.guard_top_height;
//    int guard_bottom_height = cfar_para_p.guard_bottom_height;
//    dim3 blocks_num1(iDivUp(height, BLOCK_DIM), 1, 1);
//    dim3 blocks_num2(iDivUp(width, BLOCK_DIM), 1, 1);
//    dim3 threads_per_block(BLOCK_DIM, 1, 1);
//    integral_row_kernel4 <<< blocks_num1, threads_per_block >>> (mtd_abs_data, width, height, integral_image_row,
//            refer_left_width, refer_right_width, guard_left_width, guard_right_width);
//    integral_column_kernel4 <<< blocks_num2, threads_per_block >>> (mtd_abs_data, width, height, integral_image_column,
//            refer_top_height, refer_bottom_height, guard_top_height, guard_bottom_height);
//}

void ca_cfar_2d_cross(float* integral_image_row, float* integral_image_column,
                      float* mtd_abs_data, unsigned char* dst_data,
                      unsigned int width , unsigned int height,const wave_para_t wave_para_p,
                      const cfar_para_t cfar_para_p,
                      const sp_para_t SP_para_current)
{
    //int width = wave_para_p.pulse_length;
    //int height = wave_para_p.fft_number;
    int refer_left_width = cfar_para_p.refer_left_width;
    int refer_right_width = cfar_para_p.refer_right_width;
    int refer_top_height = cfar_para_p.refer_top_height;
    int refer_bottom_height = cfar_para_p.refer_bottom_height;
    int guard_left_width = cfar_para_p.guard_left_width;
    int guard_right_width = cfar_para_p.guard_right_width;
    int guard_top_height = cfar_para_p.guard_top_height;
    int guard_bottom_height = cfar_para_p.guard_bottom_height;

    dim3 blocks_num_cfar(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block_cfar(BLOCK_2DIM, BLOCK_2DIM, 1);
    ca_cfar_2d_cross_kernel <<< blocks_num_cfar, threads_per_block_cfar>>> (mtd_abs_data, width, height, integral_image_row, integral_image_column,
            SP_para_current,wave_para_p,
            refer_left_width, refer_right_width, refer_top_height, refer_bottom_height,
            guard_left_width, guard_right_width, guard_top_height, guard_bottom_height,
            dst_data);
}

__global__ void calculate_cross_serve_kernel(float* src_data, int width, int height, float* intergral_r, float* intergral_v,
                                            int refer_r, int guard_r, int refer_v, int guard_v)
{
    int r_num = blockIdx.x * blockDim.x + threadIdx.x;
    int v_num = blockIdx.y * blockDim.y + threadIdx.y;

    if (r_num >= width || v_num >= height)
    {
        return;
    }


    float rs = 0.0f;
    if (r_num <= refer_r)
    {
         for (int i = r_num + guard_r + 1; i <= r_num + refer_r; i++)
         {
              rs += src_data[v_num * width + i ];
         }
         intergral_r[v_num * width + r_num ] = rs * 2;
         rs = 0.0f;
    }
    else if(r_num > refer_r && r_num <= width - refer_r)
    {
          for (int i = r_num - refer_r; i <= r_num - guard_r - 1; i++)
          {
              rs += src_data[v_num * width + i ];
          }
          for (int i = r_num + guard_r + 1; i <= r_num + refer_r; i++)
          {
              rs += src_data[v_num * width + i ];
          }
          intergral_r[v_num * width + r_num ] = rs;
          rs = 0.0f;
    }
    else
    {
          for (int i = r_num - refer_r; i <= r_num - guard_r - 1; i++)
          {
              rs += src_data[v_num * width + i ];
          }
          intergral_r[v_num * width + r_num ] = rs * 2;
          rs = 0.0f;
    }


    if (v_num <= refer_v)
    {
          for (int i = v_num + guard_v + 1; i <= v_num + refer_v; i++)
          {
              rs += src_data[i * width + r_num];
          }
          intergral_v[v_num * width + r_num] = rs * 2;
          rs = 0.0f;
     }
    else if (v_num > refer_v && v_num <= height - refer_v)
    {
          for (int i = v_num - refer_v; i <= v_num - guard_v - 1; i++)
          {
              rs += src_data[i * width + r_num];
          }
          for (int i = v_num + guard_v + 1; i <= v_num + refer_v; i++)
          {
              rs += src_data[i  * width + r_num];
          }
          intergral_v[v_num  * width + r_num] = rs;
          rs = 0.0f;
    }
    else
    {
          for (int i = v_num - refer_v; i <= v_num - guard_v - 1; i++)
          {
              rs += src_data[i  * width + r_num];
          }
          intergral_v[v_num  * width + r_num] = rs * 2;
          rs = 0.0f;
    }

}

void calculate_cross_serve(float* mtd_abs_data,const cfar_para_t cfar_para_p,
                           float* integral_image_row, float* integral_image_column,
                           unsigned int width,unsigned int height)
{
    //int width = wave_para_p.pulse_length;
    //int height = wave_para_p.fft_number;
//    int refer_r = 16;
//    int refer_v = 16;
//    int guard_r = 3;
//    int guard_v = 3;
    int refer_r = cfar_para_p.refer_left_width;
    int refer_v = cfar_para_p.refer_top_height;
    int guard_r = cfar_para_p.guard_left_width;
    int guard_v = cfar_para_p.guard_top_height;

    dim3 blocks_num2d(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block2d(BLOCK_2DIM, BLOCK_2DIM, 1);
    calculate_cross_serve_kernel<<< blocks_num2d, threads_per_block2d >>> (mtd_abs_data, width, height, integral_image_row, integral_image_column,
                                                refer_r, guard_r, refer_v, guard_v);
}

//距离维点迹凝聚
//过门限点取幅度最高
__global__ void plot_centroid_neighbour_range_kernel(unsigned char *bool_data,float* mtd_abs_sum, unsigned int width , unsigned int height)
{
    unsigned int r = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int c = 0;
    //unsigned int width = wave_para_p.pulse_length;
    //unsigned int height = wave_para_p.fft_number;

    if (c>=width || r >= height)
    {
        return;
    }

    for(c=0; c<width-2; c++)
    {
        if(bool_data[r*width+c] == 1 && bool_data[r*width+(c+1)] == 1)
        {
            if(mtd_abs_sum[r*width+c] > mtd_abs_sum[r*width+(c+1)])
            {
                bool_data[r*width+(c+1)] = 0;
            }
            else
            {
                bool_data[r*width+c] = 0;
            }
        }
        if(bool_data[r*width+c] == 1 && bool_data[r*width+(c+2)] == 1)
        {
            if(mtd_abs_sum[r*width+c] > mtd_abs_sum[r*width+(c+2)])
            {
                bool_data[r*width+(c+2)] = 0;
            }
            else
            {
                bool_data[r*width+c] = 0;
            }
        }
    }
}

__global__ void plot_centroid_neighbour_range_shared_kernel(unsigned char* bool_data,
        const float* mtd_abs_sum, unsigned int width, unsigned int height)
{
    const unsigned int row = blockIdx.x;
    if(row >= height)
    {
        return;
    }

    extern __shared__ __align__(4) unsigned char row_data[];
    unsigned char* flags = row_data;
    const unsigned int aligned_width = (width + 3u) & ~3u;
    float* amplitudes = reinterpret_cast<float*>(row_data + aligned_width);
    const unsigned int offset = row * width;

    for(unsigned int col = threadIdx.x; col < width; col += blockDim.x)
    {
        flags[col] = bool_data[offset + col];
        amplitudes[col] = mtd_abs_sum[offset + col];
    }
    __syncthreads();

    if(threadIdx.x == 0)
    {
        for(unsigned int col = 0; col + 2 < width; ++col)
        {
            if(flags[col] == 1 && flags[col + 1] == 1)
            {
                if(amplitudes[col] > amplitudes[col + 1])
                {
                    flags[col + 1] = 0;
                }
                else
                {
                    flags[col] = 0;
                }
            }
            if(flags[col] == 1 && flags[col + 2] == 1)
            {
                if(amplitudes[col] > amplitudes[col + 2])
                {
                    flags[col + 2] = 0;
                }
                else
                {
                    flags[col] = 0;
                }
            }
        }
    }
    __syncthreads();

    for(unsigned int col = threadIdx.x; col < width; col += blockDim.x)
    {
        bool_data[offset + col] = flags[col];
    }
}

//速度维点迹凝聚
//门限点取幅度最高
__global__ void plot_centroid_neighbour_velocity_kernel(unsigned char *bool_data,float* mtd_abs_sum, unsigned int width , unsigned int height)
{
    unsigned int c = blockDim.x * blockIdx.x + threadIdx.x;
    unsigned int r = 0;

    //unsigned int width = wave_para_p.pulse_length;
    //unsigned int height = wave_para_p.fft_number;

    if (c>=width || r >= height)
    {
        return;
    }

    for(r=0; r<height-2; r++)
    {
        if(bool_data[r*width+c] == 1 && bool_data[(r+1)*width+c] == 1)
        {
            if(mtd_abs_sum[r*width+c] > mtd_abs_sum[(r+1)*width+c])
            {
                bool_data[(r+1)*width+c] = 0;
            }
            else
            {
                bool_data[r*width+c] = 0;
            }
        }
        if(bool_data[r*width+c] == 1 && bool_data[(r+2)*width+c] == 1)
        {
            if(mtd_abs_sum[r*width+c] > mtd_abs_sum[(r+2)*width+c])
            {
                bool_data[(r+2)*width+c] = 0;
            }
            else
            {
                bool_data[r*width+c] = 0;
            }
        }
    }
}

//点迹凝聚
//判断
void plot_centroid_neighbour(unsigned char *bool_data,float* mtd_abs_sum,unsigned int width , unsigned int height)
{
    //int width = wave_para_p.pulse_length;
    //int height = wave_para_p.fft_number;

    dim3 blocks_num1(iDivUp(height, BLOCK_DIM), 1, 1);
    dim3 blocks_num2(iDivUp(width, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);

    //距离维凝聚
    if(width <= 4096 && height > 0)
    {
        const unsigned int aligned_width = (width + 3u) & ~3u;
        const size_t shared_bytes = aligned_width + width * sizeof(float);
        plot_centroid_neighbour_range_shared_kernel<<<height, 256, shared_bytes>>>(
            bool_data, mtd_abs_sum, width, height);
    }
    else
    {
        plot_centroid_neighbour_range_kernel <<< blocks_num1, threads_per_block>>> (bool_data, mtd_abs_sum, width , height);
    }
    //速度维凝聚
    plot_centroid_neighbour_velocity_kernel <<< blocks_num2, threads_per_block>>> (bool_data, mtd_abs_sum, width , height);
}


__global__ void SPCalculateCeilParaKernel(float* mtd_abs_sum,float* integral_image_row, float* integral_image_column,
                                          cufftComplex* mtd_sum, cufftComplex* mtd_fw, cufftComplex* mtd_fy,
                                          DianJi_para_t* OutParaDots,unsigned int width , unsigned int height,
                                          const wave_para_t ParaWave, const data_pack_para_t ParaArray,const sp_para_t SP_para_current,
                                          unsigned char* bool_data, float* clutter)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;

    //int width = ParaWave.pulse_length;
    //int height = ParaWave.fft_number;
    float prt = ParaWave.prt_us;

    unsigned short range_add_num;

    cufftComplex azim_t, elev_t, azim, elev;
    cufftComplex azi_jiaozhun, ele_jiaozhun,hechaqi_ele_jiaozhun;//jiaozhunzhi
    float azim_amp, elev_amp;

    float freq = ParaArray.freq;
    float velocity_t;

    float dr, dv;
//    dr = 3e8/dataFromDataPar->mWaveInfo.mSampleFreqHz/2;
//    dv = 3e8 / dataFromDataPar->mWaveInfo.mRadarFreq / (dataFromDataPar->mWaveInfo.mPRTUs * 1e-6) / dataFromDataPar->mWaveInfo.mPulseAmount /2;

    if(c >= width || r >= height)
    {
        return;
    }

    //测距
    if(ParaWave.waveCode == 0 || ParaWave.waveCode == 1)
    {
        range_add_num = PW_GAP_NUM+50; //50=1us
    }
    else
    {
        range_add_num = 0;
    }

    velocity_t = (r-int(height/2))*3e5/2.0/height/prt/freq*1e9;

    //和差测角
    azim_t = cuCdivf(mtd_fw[r * width + c],mtd_sum[r * width + c]);
    elev_t = cuCdivf(mtd_fy[r * width + c],mtd_sum[r * width + c]);

#if defined Signalprocess_Radar_chaofeng
    azim = azim_t;
    elev = elev_t;

    if(azim.y > 0)
    {
        azim_amp = -cuCabsf(azim);
    }
    else
    {
        azim_amp = cuCabsf(azim);
    }
    if(elev.y > 0)
    {
        elev_amp = -cuCabsf(elev);
    }
    else
    {
        elev_amp = cuCabsf(elev);
    }
#elif defined Signalprocess_Radar_Archiwave
    //20230531芯片阵测校准值
    azi_jiaozhun.x = -0.92538;
    azi_jiaozhun.y = -0.33758;
    ele_jiaozhun.x = -0.53758;
    ele_jiaozhun.y = -0.5384;
    azim = cuCmulf(azim_t,azi_jiaozhun);
    elev = cuCmulf(elev_t,ele_jiaozhun);
    if(azim.y > 0)
    {
        azim_amp = cuCabsf(azim);
    }
    else
    {
        azim_amp = -cuCabsf(azim);
    }
    if(elev.y > 0)
    {
        elev_amp = cuCabsf(elev);
    }
    else
    {
        elev_amp = -cuCabsf(elev);
    }
#elif defined Signalprocess_Radar_RDV
    azim = cuCmulf(azim_t,azi_jiaozhun);
    elev = cuCmulf(elev_t,ele_jiaozhun);
    if(azim.y > 0)
    {
        azim_amp = -cuCabsf(azim);
    }
    else
    {
        azim_amp = cuCabsf(azim);
    }
    if(elev.y > 0)
    {
        elev_amp = -cuCabsf(elev);
    }
    else
    {
        elev_amp = cuCabsf(elev);
    }
#endif

    const float range_resolution = 300/(2 * ParaWave.sampleFreqMHz);
    float current_range, cfar_th_r, cfar_th_v;
    current_range = c * range_resolution;
    if ((current_range >= 0) && (current_range <= 1000.0f))
    {
        cfar_th_r = SP_para_current.cfar_th_r1;
        cfar_th_v = SP_para_current.cfar_th_v1;
    }
    else if ((current_range > 1000.0f) && (current_range <= 2000.0f))
    {
        cfar_th_r = SP_para_current.cfar_th_r2;
        cfar_th_v = SP_para_current.cfar_th_v2;
    }
    else
    {
        cfar_th_r = SP_para_current.cfar_th_r3;
        cfar_th_v = SP_para_current.cfar_th_v3;
    }

    if(bool_data[r*width+c] == 1)
    {

        OutParaDots[r*width+c].rangeCeilIdx = c;
        OutParaDots[r*width+c].dopCeilIdx = r;
        OutParaDots[r*width+c].range = (c+range_add_num+1)*300.0/ParaWave.sampleFreqMHz/2.0;
        OutParaDots[r*width+c].velocity = velocity_t;
        OutParaDots[r*width+c].amplitude = mtd_abs_sum[r * width + c];
        OutParaDots[r*width+c].ampStatic_Clutter = clutter[r * width + c];
        OutParaDots[r*width+c].aziRatio = azim_amp;
        OutParaDots[r*width+c].eleRatio = elev_amp;
        OutParaDots[r*width+c].I_azi = azim.x;
        OutParaDots[r*width+c].Q_azi = azim.y;
        OutParaDots[r*width+c].I_ele = elev.x;
        OutParaDots[r*width+c].Q_ele = elev.y;
        OutParaDots[r*width+c].RCS = pow(OutParaDots[r*width+c].range,4)*pow(OutParaDots[r*width+c].amplitude,2)/1e23;
        OutParaDots[r*width+c].cfar_th_r = cfar_th_r;
        OutParaDots[r*width+c].cfar_th_v = cfar_th_v;
        OutParaDots[r*width+c].mean_row = integral_image_row[r * width + c] / 26;
        OutParaDots[r*width+c].mean_col = integral_image_column[r * width + c] / 26;
    }
}

//计算每个单元的参数
void SPCalculateCeilPara(float* mtd_abs_sum,float* integral_image_row, float* integral_image_column,
                         cufftComplex* mtd_sum, cufftComplex* mtd_fw, cufftComplex* mtd_fy,
                         DianJi_para_t* OutParaDots,
                         unsigned int width , unsigned int height,const wave_para_t ParaWave,const sp_para_t SP_para_current,
                         const data_pack_para_t ParaArray,unsigned char* bool_data, float* clutter)
{
    //int width = ParaWave.pulse_length;
    //int height = ParaWave.fft_number;
//    printf("xxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
//    printf("%d\n",height);
    dim3 blocks_num_cfar(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block_cfar(BLOCK_2DIM, BLOCK_2DIM, 1);
    SPCalculateCeilParaKernel <<< blocks_num_cfar, threads_per_block_cfar>>> (mtd_abs_sum, integral_image_row, integral_image_column,
                                                                              mtd_sum, mtd_fw, mtd_fy,
                                                                              OutParaDots,
                                                                              width, height, ParaWave, ParaArray, SP_para_current, bool_data, clutter);
}

__global__ void countSignalPointsKernel(const unsigned char* bool_data,
                                        const DianJi_para_t* dots, unsigned int count,
                                        unsigned int* blockCounts)
{
    const unsigned int index = blockIdx.x * blockDim.x + threadIdx.x;
    const bool valid = index < count && bool_data[index] == 1 &&
                       fabsf(dots[index].aziRatio) <= 1.0f &&
                       fabsf(dots[index].eleRatio) <= 1.0f;
    const unsigned int mask = __ballot_sync(0xffffffff, valid);
    __shared__ unsigned int warpCounts[8];
    if ((threadIdx.x & 31) == 0)
    {
        warpCounts[threadIdx.x >> 5] = __popc(mask);
    }
    __syncthreads();
    if (threadIdx.x == 0)
    {
        unsigned int total = 0;
        for (int warp = 0; warp < 8; ++warp)
        {
            total += warpCounts[warp];
        }
        blockCounts[blockIdx.x] = total;
    }
}

__global__ void prefixSignalPointCountsKernel(const unsigned int* blockCounts,
                                              unsigned int* blockOffsets,
                                              unsigned int blocks, CompactDotResult* result)
{
    unsigned int total = 0;
    for (unsigned int block = 0; block < blocks; ++block)
    {
        blockOffsets[block] = total;
        total = total + blockCounts[block] < MAX_DOTS_NUM
                    ? total + blockCounts[block] : MAX_DOTS_NUM;
    }
    result->count = total;
}

__global__ void gatherSignalPointsKernel(const unsigned char* bool_data,
                                         const DianJi_para_t* dots, unsigned int count,
                                         const unsigned int* blockOffsets,
                                         CompactDotResult* result)
{
    const unsigned int offset = blockOffsets[blockIdx.x];
    if (offset >= MAX_DOTS_NUM)
    {
        return;
    }

    const unsigned int index = blockIdx.x * blockDim.x + threadIdx.x;
    const bool valid = index < count && bool_data[index] == 1 &&
                       fabsf(dots[index].aziRatio) <= 1.0f &&
                       fabsf(dots[index].eleRatio) <= 1.0f;
    const unsigned int mask = __ballot_sync(0xffffffff, valid);
    const unsigned int lane = threadIdx.x & 31;
    const unsigned int warp = threadIdx.x >> 5;
    __shared__ unsigned int warpOffsets[8];
    if (lane == 0)
    {
        warpOffsets[warp] = __popc(mask);
    }
    __syncthreads();
    if (threadIdx.x == 0)
    {
        unsigned int total = 0;
        for (int w = 0; w < 8; ++w)
        {
            const unsigned int current = warpOffsets[w];
            warpOffsets[w] = total;
            total += current;
        }
    }
    __syncthreads();
    const unsigned int rank = offset + warpOffsets[warp] +
                              __popc(mask & ((1u << lane) - 1u));
    if (valid && rank < MAX_DOTS_NUM)
    {
        result->dots[rank].index = index;
        result->dots[rank].para = dots[index];
    }
}

void compactSignalPoints(const unsigned char* bool_data, const DianJi_para_t* dots,
                         unsigned int count, unsigned int* blockCounts,
                         unsigned int* blockOffsets, CompactDotResult* result)
{
    if (count == 0)
    {
        cudaMemset(result, 0, sizeof(unsigned int));
        return;
    }
    const unsigned int blocks = (count + 255) / 256;
    countSignalPointsKernel<<<blocks, 256>>>(bool_data, dots, count, blockCounts);
    prefixSignalPointCountsKernel<<<1, 1>>>(blockCounts, blockOffsets, blocks, result);
    gatherSignalPointsKernel<<<blocks, 256>>>(bool_data, dots, count, blockOffsets, result);
}

//**********************************************************************************//
//                                      capon                                       //
//**********************************************************************************//
//求自相关
//x: the input data array, matric pulse_number*fft_length
//corrx: the output data array, matric mOrder*pulse_length
//mOrder: the order of the covariance toeplitz matrix
__global__ void caponXcorrKernel(cuComplex *x, cuComplex *corrx, unsigned int mOrder, const wave_para_t wave_para_p)
{
    uint c = blockDim.x * blockIdx.x + threadIdx.x;
    uint r;
    uint m;
    uint x_width = wave_para_p.fft_length;
    uint width = wave_para_p.pulse_length;
    cuComplex tmp = {0,0};
    cuComplex A,B;

    if(c >= wave_para_p.pulse_length || mOrder >= wave_para_p.pulse_number)
    {
        return;
    }

    //sum(x(i:end).*conj(x(1:end-i)))/N
    for(m=0; m<mOrder; m++)
    {
        tmp.x = 0;
        tmp.y = 0;
        for(r=m; r<wave_para_p.pulse_number; r++)
        {
            A = x[r*x_width+c];
            B = x[(r-m)*x_width+c];
            tmp = cuCaddf(tmp, cuCmulf(A, cuConjf(B)));
//            if(c == 790 && m == 0 &&r == 0)
//            {
//                printf("%f %f %f %f %f\n",B.x*A.y,  A.x*B.y,  B.x*A.y-A.x*B.y, tmp.x, tmp.y);
//            }
        }
        corrx[m*width+c].x = tmp.x/wave_para_p.pulse_number;
        corrx[m*width+c].y = tmp.y/wave_para_p.pulse_number;
//        if(c == 790 && m == 0)
//        {
//            printf("%f %f %f %f \n",corrx[m*width+c].x,  corrx[m*width+c].y,  tmp.x, tmp.y);
//        }
    }



}

//求托普利茨矩阵
//corrxtp:matric pulse_length*mOrder*mOrder
__global__ void caponToeplitzKernel(cuComplex *corrx, cuComplex *corrxtp, unsigned int mOrder, const wave_para_t wave_para_p)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;
    int r = blockDim.y * blockIdx.y + threadIdx.y;
    int m;

    if(c >= wave_para_p.pulse_length || r >= mOrder)
    {
        return;
    }

    for(m=0; m<r; m++)
    {
        corrxtp[c*mOrder*mOrder + r*mOrder + m] = cuConjf(corrx[abs(m-r)*wave_para_p.pulse_length + c]);
    }
    for(m=r; m<mOrder; m++)
    {
        corrxtp[c*mOrder*mOrder + r*mOrder + m] = corrx[abs(m-r)*wave_para_p.pulse_length + c];
    }
}

void caponInvert(cuComplex* src, cuComplex* dst, int n, int batchSize)
{
//    int *P, *INFO;

//    cudaMalloc(&P, n * batchSize * sizeof(int));
//    cudaMalloc(&INFO,  batchSize * sizeof(int));

//    cuComplex **A;
//    A = (cuComplex **)malloc(batchSize*sizeof(cuComplex *));
//    cuComplex **A_d, *A_dflat;

//    cudaMalloc(&A_d,batchSize*sizeof(cuComplex *));
//    cudaMalloc(&A_dflat, n*n*batchSize*sizeof(cuComplex));

//    A[0] = A_dflat;
//    for (int i = 1; i < batchSize; i++)
//        A[i] = A[i-1]+(n*n);

//    cudaMemcpy(A_d,A,batchSize*sizeof(cuComplex *),cudaMemcpyHostToDevice);


//    for (int i = 0; i < batchSize; i++)
//        cudaMemcpy(A_dflat+(i*n*n), src+(i*n*n), n*n*sizeof(cuComplex), cudaMemcpyDeviceToDevice);
    cudaMemcpy(A_dflat, src, n*n*batchSize*sizeof(cuComplex), cudaMemcpyDeviceToDevice);

    cublasCgetrfBatched(blasHandle,n,A_d,n,P,INFO,batchSize);

//    cuComplex **C;
//    C= (cuComplex **)malloc(batchSize*sizeof(cuComplex *));
//    cuComplex **C_d, *C_dflat;

//    cudaMalloc(&C_d,batchSize*sizeof(cuComplex *));
//    cudaMalloc(&C_dflat, n*n*batchSize*sizeof(cuComplex));
//    C[0] = C_dflat;
//    for (int i = 1; i < batchSize; i++)
//        C[i] = C[i-1] + (n*n);
//    cudaMemcpy(C_d,C,batchSize*sizeof(cuComplex *),cudaMemcpyHostToDevice);
    cublasCgetriBatched(blasHandle,n,(const cuComplex **)A_d,n,P,C_d,n,INFO,batchSize);

//    for (int i = 0; i < batchSize; i++)
//        cudaMemcpy(dst + (i*n*n), C_dflat + (i*n*n), n*n*sizeof(cuComplex), cudaMemcpyDeviceToDevice);
    cudaMemcpy(dst, C_dflat, n*n*batchSize*sizeof(cuComplex), cudaMemcpyDeviceToDevice);

//    cudaFree(A_d); cudaFree(A_dflat);
//    free(A);
//    cudaFree(C_d); cudaFree(C_dflat); free(C);
//    cudaFree(P); cudaFree(INFO);
}

//计算功率谱估计结果
__global__ void caponPowerSpectraEstimationKernel(cuComplex *S, cuComplex *corrxtpinv, unsigned int mOrder, const wave_para_t wave_para_p)
{
    uint i = blockDim.x * blockIdx.x + threadIdx.x; //pulse_length
    uint j = blockDim.y * blockIdx.y + threadIdx.y; //pulse_number
    uint m, k;
    cuComplex e[MCAPONORDER];
    cuComplex eH[MCAPONORDER];
    cuComplex tmp[MCAPONORDER];
    uint width = wave_para_p.pulse_length;
    cuComplex A = {0,0};

    if(i >= wave_para_p.pulse_length || j >= wave_para_p.pulse_number)
    {
        return;
    }

//    cudaMalloc(&e, sizeof(cuComplex)*mOrder);
//    cudaMalloc(&eH, sizeof(cuComplex)*mOrder);
//    cudaMalloc(&tmp, sizeof(cuComplex)*mOrder);

    for(m=0; m<mOrder; m++)
    {
        e[m].x = cos(-2*M_PI*m*j/wave_para_p.pulse_number);
        e[m].y = sin(-2*M_PI*m*j/wave_para_p.pulse_number);
        eH[m].x = e[m].x;
        eH[m].y = -e[m].y;
    }

    for(m=0; m<mOrder; m++)
    {
        tmp[m].x = 0;
        tmp[m].y = 0;
        for(k=0; k<mOrder; k++)
        {
            tmp[m] = cuCaddf(tmp[m], cuCmulf(eH[k], corrxtpinv[i*mOrder*mOrder+k*mOrder+m ]));
        }        
        A = cuCaddf(A, cuCmulf(tmp[m], e[m]));
    }
    S[j*width+i].x =  A.x/(A.x*A.x+A.y*A.y);
    S[j*width+i].y = -A.y/(A.x*A.x+A.y*A.y);

    //    cudaFree(e);
//    cudaFree(eH);
//    cudaFree(tmp);
}

//capon功率谱估计
void caponPowerSpectraEstimation(cuComplex *x, cuComplex *S,
                                 cuComplex *corrx, cuComplex *corrxtp, cuComplex *corrxtpinv,
                                 unsigned int mOrder, const wave_para_t wave_para_p)
{
    //求自相关
    dim3 blocks_num1(iDivUp(wave_para_p.fft_length, BLOCK_DIM), 1, 1);
    dim3 threads_per_block1(BLOCK_DIM, 1, 1);
    caponXcorrKernel <<< blocks_num1, threads_per_block1 >>> (x, corrx, mOrder, wave_para_p);

    //求托普利茨矩阵
    dim3 blocks_num2(iDivUp(wave_para_p.pulse_length, BLOCK_2DIM), iDivUp(mOrder, BLOCK_2DIM), 1);
    dim3 threads_per_block2(BLOCK_2DIM, BLOCK_2DIM, 1);
    caponToeplitzKernel <<< blocks_num2, threads_per_block2>>> (corrx, corrxtp, mOrder, wave_para_p);

    //求逆矩阵
    caponInvert(corrxtp, corrxtpinv, mOrder, wave_para_p.pulse_length);

    //计算功率谱估计结果
    dim3 blocks_num3(iDivUp(wave_para_p.pulse_length, BLOCK_2DIM), iDivUp(wave_para_p.pulse_number, BLOCK_2DIM),1);
    dim3 threads_per_block3(BLOCK_2DIM, BLOCK_2DIM, 1);
    caponPowerSpectraEstimationKernel <<< blocks_num3, threads_per_block3>>> (S, corrxtpinv, mOrder, wave_para_p);

    return;
}
__global__ void Get_pc_data_sum_kernel(cuComplex *pc_data_sum, cuComplex *pc_data_sum_part, unsigned int idxRange, const wave_para_t wave_para_p)
{
    int c = blockDim.x * blockIdx.x + threadIdx.x;

    if(c >= wave_para_p.pulse_number)
    {
        return;
    }
    //幅度归一化
    //pc_data_sum_part[c].x = (pc_data_sum[idxRange + c * wave_para_p.fft_length]).x/(wave_para_p.pw_us * wave_para_p.sampleFreqMHz) * (150/wave_para_p.sampleFreqMHz*idxRange) * (150/wave_para_p.sampleFreqMHz*idxRange);
    //pc_data_sum_part[c].y = (pc_data_sum[idxRange + c * wave_para_p.fft_length]).y/(wave_para_p.pw_us * wave_para_p.sampleFreqMHz) * (150/wave_para_p.sampleFreqMHz*idxRange) * (150/wave_para_p.sampleFreqMHz*idxRange);
    pc_data_sum_part[c] = (pc_data_sum[idxRange + c * wave_para_p.fft_length]);


}
void Get_pc_data_sum(cuComplex *pc_data_sum, cuComplex *pc_data_sum_part, unsigned int idxRange, const wave_para_t wave_para_p)
{
    dim3 blocks_num(iDivUp(wave_para_p.pulse_number, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    Get_pc_data_sum_kernel <<< blocks_num, threads_per_block >>> (pc_data_sum, pc_data_sum_part, idxRange, wave_para_p);

}

__global__ void Get_mtd_data_sum_kernel(cuComplex *mtd_sum, cuComplex *mtd_sum_part, unsigned int idxRange, const wave_para_t wave_para_p)
{
    int r = blockDim.x * blockIdx.x + threadIdx.x;

    if(r >= wave_para_p.fft_number)
    {
        return;
    }
    // MTD 数据按 [doppler, range] 排列，这里固定距离 idxRange，沿多普勒维抽取一列。
    mtd_sum_part[r] = mtd_sum[r * wave_para_p.pulse_length + idxRange];
}

void Get_mtd_data_sum(cuComplex *mtd_sum, cuComplex *mtd_sum_part, unsigned int idxRange, const wave_para_t wave_para_p)
{
    dim3 blocks_num(iDivUp(wave_para_p.fft_number, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    // 导出单个距离单元的整列 MTD 复数数据，供 CPU 侧保存到点迹结构。
    Get_mtd_data_sum_kernel <<< blocks_num, threads_per_block >>> (mtd_sum, mtd_sum_part, idxRange, wave_para_p);
}

void fftshift(cufftDoubleComplex** matrix)
{
    int half_rows = 512 / 2;
    int cols = 256;
    for (int j = 0; j < cols; ++j)
    {
        cufftDoubleComplex temp;
        for (int i = 0; i < half_rows; ++i)
        {
            temp = matrix[i][j];
            matrix[i][j] = matrix[i + half_rows][j];
            matrix[i + half_rows][j] = temp;
        }
    }
}

void stft(double *f, int f_length, double wd, cufftDoubleComplex **TF)
{

    int cntr = f_length / 2;
    double sigma = wd;
    int fsz = f_length;

    double *z = (double *)malloc(fsz * sizeof(double));
    double *gwin = (double *)malloc(fsz * sizeof(double));
    cufftHandle fft_handle_stft;
    cufftPlan1d(&fft_handle_stft , 512 , CUFFT_R2C , 1);
    for (int m = 0; m < fsz; m++)
    {
        z[m] = exp(-pow(m - fsz / 2, 2) / (2 * sigma * sigma)) / (sqrt(2 * M_PI) * sigma);
    }

    for (int m = 0; m < fsz; m++)
    {
        int mm = m - cntr + fsz / 2;

        if (mm < 0)
        {
            mm = (mm + fsz) % fsz;
        }
        else if (mm >= fsz)
        {
            mm = mm % fsz;
        }

        gwin[m] = z[mm];
    }

    int winsz = fsz;
    double *x = (double *)calloc(fsz + winsz, sizeof(double)); // zero padding
    for (int i = 0; i < fsz; i++)
    {
        x[winsz / 2 + i] = f[i];
    }
    cufftDoubleReal *X = (double *)calloc(winsz, sizeof(cufftDoubleReal));
    for (int j = 0; j < fsz; j++)
    {
        for (int k = 0; k < winsz; k++)
        {
            X[k] = gwin[k] * x[j + k];

        }
         cufftExecD2Z(fft_handle_stft , X , TF[j]);
    }
    free(X);
    free(x);
    free(z);
    free(gwin);
}

void microDopplerSignature(unsigned int M_range , double* f , const wave_para_t ParaWave , float** TF_DB)
{
    int np = 2048;
    float dT = ParaWave.prt_us;
    int wd = 512;
    int wdd2 = wd/2;
    int wdd8 = wd/8;
    int ns = np/wd;
    int a = (M_range % ParaWave.pulse_length);
    //float* f = (float*)malloc(sizeof(float)*2048);
    double* sig = (double*)malloc(sizeof(double)*wd);
    cufftDoubleComplex** TMP = (cufftDoubleComplex**)malloc(sizeof(cufftDoubleComplex*)*wd);
    cufftDoubleComplex** TF2 = (cufftDoubleComplex**)malloc(sizeof(cufftDoubleComplex*)*wd);
    cufftDoubleComplex** TF1 = (cufftDoubleComplex**)malloc(sizeof(cufftDoubleComplex*)*wd);
    cufftDoubleComplex** TF = (cufftDoubleComplex**)malloc(sizeof(cufftDoubleComplex*)*wd);
    //float** TFdb = (float**)malloc(sizeof(float*)*wd);
    for(int i = 0; i < wd; i++)
    {
        TMP[i] = (cufftDoubleComplex*)malloc(sizeof(cufftDoubleComplex)*512);
        TF2[i] = (cufftDoubleComplex*)malloc(sizeof(cufftDoubleComplex)*256);
        TF1[i] = (cufftDoubleComplex*)malloc(sizeof(cufftDoubleComplex)*256);
        TF[i] = (cufftDoubleComplex*)malloc(sizeof(cufftDoubleComplex)*256);
        //TFdb[i] = (float*)malloc(sizeof(float)*256);
    }
    //for(int i = 0; i < 2048; i++)
    //{
    //    f[i] = cuCabsf(pc_data_sum[a + i * ParaWave.pulse_length]);
    //}


    for(int k = 0; k < ns; k++)
    {
        for(int i = 0; i < wd; i++)
        {
            sig[i] = f[i + k * wd];
        }
        stft(sig , 512 , 16 , TMP);
        for(int i = 0; i < wd; i++)
        {
            for(int j = 0; j < 64; j++)
            {
               TF2[i][j + k * wdd8] = TMP[i][j*8];
            }
        }
    }
    TF = TF2;
    for(int k = 0; k < ns - 1; k++)
    {
        for(int i = 0; i < wd; i++)
        {
            sig[i] = f[i + k * wd + wdd2];
        }
        stft(sig , 512 , 16 , TMP);
        for(int i = 0; i < wd; i++)
        {
            for(int j = 0; j < 64; j++)
            {
               TF1[i][j+ k * wdd8] = TMP[i][j*8];
            }
        }
    }

    for(int k = 0; k < ns - 1; k++)
    {
       for(int i = 0; i < 512; i++)
       {
           for(int j = -8; j <= 8; j++)
           {
              TF[i][(k + 1) * wdd8 + j]  = TF1[i][k * wdd8 + wdd8/2 + j];
           }
       }
    }
    fftshift(TF);
    for (int i = 0; i < 512; i++)
    {
        for(int j = 0; j < 256; j++)
        {
            TF_DB[i][j] = 20 * log10f(cuCabs(TF[i][j]) + 1e-10f);
        }
    }
    free(TF);
    free(TF1);
    free(TF2);
    free(TMP);
    free(sig);
}


__global__ void convertIntToCuComplexKernel(short* shortArray, cuComplex* cuComplexArray, int size)
{
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < size)
    {
        short real = shortArray[2 * idx];
        short imag = shortArray[2 * idx + 1];

        // 转换为 cuComplex
        cuComplexArray[idx] = make_cuComplex((float)real, (float)imag);
    }
}
void convertIntToCuComplex(short* shortArray, cuComplex* cuComplexArray, const wave_para_t wave_para_p)
{
    dim3 blocks_num(iDivUp(wave_para_p.pulse_number * wave_para_p.pulse_length, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    int Size = wave_para_p.pulse_length * wave_para_p.pulse_number;
    convertIntToCuComplexKernel <<< blocks_num, threads_per_block >>> (shortArray, cuComplexArray, Size);

}

__global__ void fftshift_2d_rows(cuComplex* input, cuComplex* output, int height, int width) {
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;

//    if (col < width && row < height) {
//        int half_width = width / 2;
//        if (col < half_width) {
//            output[row * width + col + half_width] = input[row * width + col];
//        } else {
//            output[row * width + col - half_width] = input[row * width + col];
//        }
//    }

    if (col < width && row < height)
    {
        int half_height = height / 2;
        if (row < half_height)
        {
            output[(row + half_height) * width + col] = input[row * width + col];
        } else
        {
            output[(row - half_height) * width + col] = input[row * width + col];
        }
    }
}

void fftshift_2d_rows_cpu(cuComplex* InData, cuComplex* OutData, int height, int width)
{

    dim3 blocks_num2d(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block2d(BLOCK_2DIM, BLOCK_2DIM, 1);

    fftshift_2d_rows <<< blocks_num2d, threads_per_block2d >>> (InData, OutData, height, width);
}





__global__ void DefClutterMemoryUpdate(float* InData,  float* OutData, int height, int width, float RateWeight)
{
    int col = blockIdx.x * blockDim.x + threadIdx.x;
    int row = blockIdx.y * blockDim.y + threadIdx.y;

    if (col < width && row < height)
    {
        int idx = row * width + col;
        OutData[idx] = (1.0f - RateWeight)* OutData[idx] + RateWeight * InData[idx];
    }
}


void DefClutterMemoryUpdate_cpu(float* InData, float*OutData, int height, int width, float RateWeight)
{
    dim3 blocks_num2d(iDivUp(width, BLOCK_2DIM), iDivUp(height, BLOCK_2DIM), 1);
    dim3 threads_per_block2d(BLOCK_2DIM, BLOCK_2DIM, 1);
    DefClutterMemoryUpdate<<<blocks_num2d, threads_per_block2d>>>(InData, OutData, height, width, RateWeight);
}
__global__ void Get_doppler_data_sum_kernel(cuComplex *doppler_data_sum, cuComplex *doppler_data_sum_part, unsigned int idxRange, const wave_para_t wave_para_p)
{
    int r = blockDim.x * blockIdx.x + threadIdx.x;

    if(r >= wave_para_p.pulse_length)
    {
        return;
    }
    doppler_data_sum_part[r] = (doppler_data_sum[idxRange * wave_para_p.pulse_length + r]);


}
void Get_doppler_data_sum(cuComplex *doppler_data_sum, cuComplex *doppler_data_sum_part, unsigned int idxRange, const wave_para_t wave_para_p)
{
    dim3 blocks_num(iDivUp(wave_para_p.pulse_length, BLOCK_DIM), 1, 1);
    dim3 threads_per_block(BLOCK_DIM, 1, 1);
    Get_doppler_data_sum_kernel <<< blocks_num, threads_per_block >>> (doppler_data_sum, doppler_data_sum_part, idxRange, wave_para_p);

}
