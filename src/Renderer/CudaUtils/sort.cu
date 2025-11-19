#include <iostream>
#include <cuda_runtime.h>
#include <cub/cub.cuh>
#include <cstdlib>
#include <ctime>

// 错误检查宏
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA Error: " << cudaGetErrorString(error) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// 对数组进行就地排序
// 参数:
//   data: 待排序的浮点数组（输入/输出）
//   N: 数组元素个数
//   verbose: 是否打印调试信息和计时
// 返回: 0表示成功，-1表示失败
int cudaSort(float* data, int N, bool verbose = false) {
    if (data == nullptr || N <= 0) {
        std::cerr << "error: invalid input parameters" << std::endl;
        return -1;
    }
    
    size_t size = N * sizeof(float);
    
    if (verbose && N >= 10) {
        std::cout << "first 10 original data: ";
        for (int i = 0; i < 10; i++) {
            std::cout << data[i] << " ";
        }
        std::cout << std::endl;
    }
    
    float *d_in, *d_out;
    CUDA_CHECK(cudaMalloc(&d_in, size));
    CUDA_CHECK(cudaMalloc(&d_out, size));
    CUDA_CHECK(cudaMemcpy(d_in, data, size, cudaMemcpyHostToDevice));
    
    void *d_temp_storage = nullptr;
    size_t temp_storage_bytes = 0;
    cub::DeviceRadixSort::SortKeys(d_temp_storage, temp_storage_bytes, 
                                    d_in, d_out, N);
    
    CUDA_CHECK(cudaMalloc(&d_temp_storage, temp_storage_bytes));
    
    cudaEvent_t start, stop;
    float milliseconds = 0;
    
    if (verbose) {
        CUDA_CHECK(cudaEventCreate(&start));
        CUDA_CHECK(cudaEventCreate(&stop));
        CUDA_CHECK(cudaEventRecord(start));
    }
    
    cub::DeviceRadixSort::SortKeys(d_temp_storage, temp_storage_bytes, 
                                    d_in, d_out, N);
    
    if (verbose) {
        CUDA_CHECK(cudaEventRecord(stop));
        CUDA_CHECK(cudaEventSynchronize(stop));
        CUDA_CHECK(cudaEventElapsedTime(&milliseconds, start, stop));
    } else {
        CUDA_CHECK(cudaDeviceSynchronize());
    }
    
    CUDA_CHECK(cudaMemcpy(data, d_out, size, cudaMemcpyDeviceToHost));
    
    if (verbose) {
        if (N >= 10) {
            std::cout << "first 10 data after sorting: ";
            for (int i = 0; i < 10; i++) {
                std::cout << data[i] << " ";
            }
            std::cout << std::endl;
            
            std::cout << "last 10 data after sorting: ";
            for (int i = N - 10; i < N; i++) {
                std::cout << data[i] << " ";
            }
            std::cout << std::endl;
        }
        
        bool sorted = true;
        for (int i = 0; i < N - 1; i++) {
            if (data[i] > data[i + 1]) {
                sorted = false;
                break;
            }
        }
        
        if (sorted) {
            std::cout << "✓ verification passed! sorting correct." << std::endl;
        } else {
            std::cout << "✗ verification failed!" << std::endl;
        }
        
        std::cout << "sorting time: " << milliseconds << " ms" << std::endl;
        
        CUDA_CHECK(cudaEventDestroy(start));
        CUDA_CHECK(cudaEventDestroy(stop));
    }
    
    CUDA_CHECK(cudaFree(d_in));
    CUDA_CHECK(cudaFree(d_out));
    CUDA_CHECK(cudaFree(d_temp_storage));
    
    return 0;
}

// 对键值对进行排序（例如：深度-索引对）
// 参数:
//   keys: 键数组（例如：深度值），排序后结果写回此数组
//   values: 值数组（例如：索引），排序后结果写回此数组
//   N: 数组元素个数
//   verbose: 是否打印调试信息和计时
// 返回: 0表示成功，-1表示失败
int cudaSortPairs(float* keys, uint32_t* values, int N, bool verbose = false) {
    if (keys == nullptr || values == nullptr || N <= 0) {
        std::cerr << "error: invalid input parameters" << std::endl;
        return -1;
    }
    
    size_t keys_size = N * sizeof(float);
    size_t values_size = N * sizeof(uint32_t);
    
    if (verbose && N >= 10) {
        std::cout << "first 10 keys and values: ";
        for (int i = 0; i < 10; i++) {
            std::cout << "(" << keys[i] << "," << values[i] << ") ";
        }
        std::cout << std::endl;
    }
    
    float *d_keys_in, *d_keys_out;
    uint32_t *d_values_in, *d_values_out;
    CUDA_CHECK(cudaMalloc(&d_keys_in, keys_size));
    CUDA_CHECK(cudaMalloc(&d_keys_out, keys_size));
    CUDA_CHECK(cudaMalloc(&d_values_in, values_size));
    CUDA_CHECK(cudaMalloc(&d_values_out, values_size));
    
    CUDA_CHECK(cudaMemcpy(d_keys_in, keys, keys_size, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_values_in, values, values_size, cudaMemcpyHostToDevice));
    
    void *d_temp_storage = nullptr;
    size_t temp_storage_bytes = 0;
    cub::DeviceRadixSort::SortPairs(d_temp_storage, temp_storage_bytes, 
                                     d_keys_in, d_keys_out,
                                     d_values_in, d_values_out, N);
    
    CUDA_CHECK(cudaMalloc(&d_temp_storage, temp_storage_bytes));
    
    cudaEvent_t start, stop;
    float milliseconds = 0;
    
    if (verbose) {
        CUDA_CHECK(cudaEventCreate(&start));
        CUDA_CHECK(cudaEventCreate(&stop));
        CUDA_CHECK(cudaEventRecord(start));
    }
    
    cub::DeviceRadixSort::SortPairs(d_temp_storage, temp_storage_bytes, 
                                     d_keys_in, d_keys_out,
                                     d_values_in, d_values_out, N);
    
    if (verbose) {
        CUDA_CHECK(cudaEventRecord(stop));
        CUDA_CHECK(cudaEventSynchronize(stop));
        CUDA_CHECK(cudaEventElapsedTime(&milliseconds, start, stop));
    } else {
        CUDA_CHECK(cudaDeviceSynchronize());
    }
    
    CUDA_CHECK(cudaMemcpy(keys, d_keys_out, keys_size, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaMemcpy(values, d_values_out, values_size, cudaMemcpyDeviceToHost));
    
    if (verbose) {
        if (N >= 10) {
            std::cout << "first 10 keys and values after sorting: ";
            for (int i = 0; i < 10; i++) {
                std::cout << "(" << keys[i] << "," << values[i] << ") ";
            }
            std::cout << std::endl;
            
            std::cout << "last 10 keys and values after sorting: ";
            for (int i = N - 10; i < N; i++) {
                std::cout << "(" << keys[i] << "," << values[i] << ") ";
            }
            std::cout << std::endl;
        }
        
        bool sorted = true;
        for (int i = 0; i < N - 1; i++) {
            if (keys[i] > keys[i + 1]) {
                sorted = false;
                break;
            }
        }
        
        if (sorted) {
            std::cout << "✓ verification passed! sorting correct." << std::endl;
        } else {
            std::cout << "✗ verification failed!" << std::endl;
        }
        
        std::cout << "CUDA sorting time: " << milliseconds << " ms" << std::endl;
        
        CUDA_CHECK(cudaEventDestroy(start));
        CUDA_CHECK(cudaEventDestroy(stop));
    }
    
    CUDA_CHECK(cudaFree(d_keys_in));
    CUDA_CHECK(cudaFree(d_keys_out));
    CUDA_CHECK(cudaFree(d_values_in));
    CUDA_CHECK(cudaFree(d_values_out));
    CUDA_CHECK(cudaFree(d_temp_storage));
    
        return 0;
}
