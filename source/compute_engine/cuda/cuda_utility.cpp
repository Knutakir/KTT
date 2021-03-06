#ifdef KTT_PLATFORM_CUDA

#include <stdexcept>
#include <compute_engine/cuda/cuda_utility.h>

namespace ktt
{

std::string getCUDAEnumName(const CUresult value)
{
    const char* name;
    cuGetErrorName(value, &name);
    return name;
}

std::string getNvrtcEnumName(const nvrtcResult value)
{
    std::string name = nvrtcGetErrorString(value);
    return name;
}

void checkCUDAError(const CUresult value, const std::string& message)
{
    if (value != CUDA_SUCCESS)
    {
        throw std::runtime_error(std::string("CUDA error: ") + getCUDAEnumName(value) + "\nAdditional info: " + message);
    }
}

void checkCUDAError(const nvrtcResult value, const std::string& message)
{
    if (value != NVRTC_SUCCESS)
    {
        throw std::runtime_error(std::string("CUDA NVRTC error: ") + getNvrtcEnumName(value) + "\nAdditional info: " + message);
    }
}

float getEventCommandDuration(const CUevent start, const CUevent end)
{
    float result;
    checkCUDAError(cuEventElapsedTime(&result, start, end), "cuEventElapsedTime");

    return result * 1'000'000.0f; // return duration in nanoseconds
}

#if defined(KTT_PROFILING_CUPTI_LEGACY) || defined(KTT_PROFILING_CUPTI)
std::string getCUPTIEnumName(const CUptiResult value)
{
    const char* name;
    cuptiGetResultString(value, &name);
    return name;
}

void checkCUPTIError(const CUptiResult value, const std::string& message)
{
    if (value != CUPTI_SUCCESS)
    {
        throw std::runtime_error(std::string("CUPTI error: ") + getCUPTIEnumName(value) + "\nAdditional info: " + message);
    }
}
#endif // KTT_PROFILING_CUPTI_LEGACY || KTT_PROFILING_CUPTI

} // namespace ktt

#endif // KTT_PLATFORM_CUDA
