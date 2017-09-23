#pragma once

#include <string>
#include <vector>

#include "cuda.h"
#include "cuda_utility.h"
#include "enum/argument_access_type.h"
#include "enum/argument_data_type.h"
#include "enum/argument_memory_location.h"

namespace ktt
{

class CudaBuffer
{
public:
    explicit CudaBuffer(const size_t kernelArgumentId, const size_t bufferSize, const size_t elementSize, const ArgumentDataType& dataType,
        const ArgumentMemoryLocation& memoryLocation, const ArgumentAccessType& accessType) :
        kernelArgumentId(kernelArgumentId),
        bufferSize(bufferSize),
        elementSize(elementSize),
        dataType(dataType),
        memoryLocation(memoryLocation),
        accessType(accessType)
    {
        checkCudaError(cuMemAlloc(&buffer, bufferSize), "cuMemAlloc");
    }

    ~CudaBuffer()
    {
        checkCudaError(cuMemFree(buffer), "cuMemFree");
    }

    void resize(const size_t newBufferSize)
    {
        if (bufferSize == newBufferSize)
        {
            return;
        }

        checkCudaError(cuMemFree(buffer), "cuMemFree");
        checkCudaError(cuMemAlloc(&buffer, newBufferSize), "cuMemAlloc");
        bufferSize = newBufferSize;
    }

    void uploadData(const void* source, const size_t dataSize)
    {
        if (bufferSize < dataSize)
        {
            resize(dataSize);
        }
        checkCudaError(cuMemcpyHtoD(buffer, source, dataSize), "cuMemcpyHtoD");
    }

    void downloadData(void* destination, const size_t dataSize) const
    {
        if (bufferSize < dataSize)
        {
            throw std::runtime_error("Size of data to download is higher than size of buffer");
        }
        checkCudaError(cuMemcpyDtoH(destination, buffer, dataSize), "cuMemcpyDtoH");
    }

    size_t getKernelArgumentId() const
    {
        return kernelArgumentId;
    }

    size_t getBufferSize() const
    {
        return bufferSize;
    }

    size_t getElementSize() const
    {
        return elementSize;
    }

    ArgumentDataType getDataType() const
    {
        return dataType;
    }

    ArgumentMemoryLocation getMemoryLocation() const
    {
        return memoryLocation;
    }

    ArgumentAccessType getAccessType() const
    {
        return accessType;
    }

    const CUdeviceptr* getBuffer() const
    {
        return &buffer;
    }

    CUdeviceptr* getBuffer()
    {
        return &buffer;
    }

private:
    size_t kernelArgumentId;
    size_t bufferSize;
    size_t elementSize;
    ArgumentDataType dataType;
    ArgumentMemoryLocation memoryLocation;
    ArgumentAccessType accessType;
    CUdeviceptr buffer;
};

} // namespace ktt
