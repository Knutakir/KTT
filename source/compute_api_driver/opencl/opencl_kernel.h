#pragma once

#include <string>

#include "CL/cl.h"
#include "opencl_utility.h"

namespace ktt
{

class OpenclKernel
{
public:
    explicit OpenclKernel(const cl_program program, const std::string& kernelName) :
        program(program),
        kernelName(kernelName),
        argumentsCount(0)
    {
        cl_int result;
        kernel = clCreateKernel(program, &kernelName[0], &result);
        checkOpenclError(result, std::string("clCreateKernel"));
    }

    ~OpenclKernel()
    {
        checkOpenclError(clReleaseKernel(kernel), std::string("clReleaseKernel"));
    }

    void setKernelArgumentVector(const void* buffer)
    {
        checkOpenclError(clSetKernelArg(kernel, argumentsCount, sizeof(cl_mem), buffer), std::string("clSetKernelArg"));
        argumentsCount++;
    }

    template <typename T> void setKernelArgumentScalar(const T value)
    {
        checkOpenclError(clSetKernelArg(kernel, argumentsCount, sizeof(T), &value), std::string("clSetKernelArg"));
        argumentsCount++;
    }

    cl_program getProgram() const
    {
        return program;
    }

    std::string getKernelName() const
    {
        return kernelName;
    }

    cl_kernel getKernel() const
    {
        return kernel;
    }

    cl_uint getArgumentsCount() const
    {
        return argumentsCount;
    }

private:
    cl_program program;
    std::string kernelName;
    cl_kernel kernel;
    cl_uint argumentsCount;
};

} // namespace ktt
