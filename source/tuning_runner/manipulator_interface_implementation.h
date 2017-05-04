#pragma once

#include <map>

#include "manipulator_interface.h"
#include "../compute_api_driver/compute_api_driver.h"
#include "../dto/kernel_runtime_data.h"
#include "../kernel_argument/argument_manager.h"

namespace ktt
{

class ManipulatorInterfaceImplementation : public ManipulatorInterface
{
public:
    // Constructor
    explicit ManipulatorInterfaceImplementation(ComputeApiDriver* computeApiDriver);

    // Inherited methods
    virtual std::vector<ResultArgument> runKernel(const size_t kernelId) override;
    virtual std::vector<ResultArgument> runKernel(const size_t kernelId, const DimensionVector& globalSize,
        const DimensionVector& localSize) override;
    virtual void updateArgumentScalar(const size_t argumentId, const void* argumentData) override;
    virtual void updateArgumentVector(const size_t argumentId, const void* argumentData, const size_t dataSizeInBytes) override;

    // Core methods
    void addKernel(const size_t id, const KernelRuntimeData& kernelRuntimeData);
    void setKernelArguments(const std::vector<KernelArgument>& kernelArguments);
    KernelRunResult getCurrentResult() const;
    void clearData();

private:
    // Attributes
    ComputeApiDriver* computeApiDriver;
    KernelRunResult currentResult;
    std::map<size_t, KernelRuntimeData> kernelDataMap;
    std::vector<KernelArgument> kernelArguments;

    // Helper methods
    std::vector<KernelArgument> getArguments(const std::vector<size_t>& argumentIndices);
};

} // namespace ktt
