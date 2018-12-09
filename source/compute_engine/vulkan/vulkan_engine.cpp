#ifdef KTT_PLATFORM_VULKAN

#include <compute_engine/vulkan/vulkan_engine.h>
#include <utility/logger.h>
#include <utility/timer.h>

namespace ktt
{

VulkanEngine::VulkanEngine(const DeviceIndex deviceIndex, const uint32_t queueCount) :
    deviceIndex(deviceIndex),
    queueCount(queueCount),
    compilerOptions(std::string("")),
    globalSizeType(GlobalSizeType::Vulkan),
    globalSizeCorrection(false),
    kernelCacheFlag(true),
    kernelCacheCapacity(10),
    persistentBufferFlag(true),
    nextEventId(0)
{
    std::vector<const char*> instanceExtensions;
    std::vector<const char*> validationLayers;

    #ifdef KTT_CONFIGURATION_DEBUG
    instanceExtensions.emplace_back("VK_EXT_debug_report");
    validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
    #endif

    Logger::getLogger().log(LoggingLevel::Debug, "Initializing Vulkan instance");
    instance = std::make_unique<VulkanInstance>("KTT", instanceExtensions, validationLayers);

    std::vector<VulkanPhysicalDevice> devices = instance->getPhysicalDevices();
    if (deviceIndex >= devices.size())
    {
        throw std::runtime_error(std::string("Invalid device index: ") + std::to_string(deviceIndex));
    }

    Logger::getLogger().log(LoggingLevel::Debug, "Initializing Vulkan device and queues");
    device = std::make_unique<VulkanDevice>(devices.at(deviceIndex), queueCount, VK_QUEUE_COMPUTE_BIT, std::vector<const char*>{}, validationLayers);
    queues = device->getQueues();

    Logger::getLogger().log(LoggingLevel::Debug, "Initializing Vulkan command pool");
    commandPool = std::make_unique<VulkanCommandPool>(device->getDevice(), device->getQueueFamilyIndex());
}

KernelResult VulkanEngine::runKernel(const KernelRuntimeData& kernelData, const std::vector<KernelArgument*>& argumentPointers,
    const std::vector<OutputDescriptor>& outputDescriptors)
{
    EventId eventId = runKernelAsync(kernelData, argumentPointers, getDefaultQueue());
    KernelResult result = getKernelResult(eventId, outputDescriptors);
    return result;
}

EventId VulkanEngine::runKernelAsync(const KernelRuntimeData& kernelData, const std::vector<KernelArgument*>& argumentPointers, const QueueId queue)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

KernelResult VulkanEngine::getKernelResult(const EventId id, const std::vector<OutputDescriptor>& outputDescriptors) const
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::getKernelOverhead(const EventId id) const
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

void VulkanEngine::setCompilerOptions(const std::string& options)
{
    compilerOptions = options;
}

void VulkanEngine::setGlobalSizeType(const GlobalSizeType type)
{
    globalSizeType = type;
}

void VulkanEngine::setAutomaticGlobalSizeCorrection(const bool flag)
{
    globalSizeCorrection = flag;
}

void VulkanEngine::setKernelCacheUsage(const bool flag)
{
    if (!flag)
    {
        clearKernelCache();
    }
    kernelCacheFlag = flag;
}

void VulkanEngine::setKernelCacheCapacity(const size_t capacity)
{
    kernelCacheCapacity = capacity;
}

void VulkanEngine::clearKernelCache()
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

QueueId VulkanEngine::getDefaultQueue() const
{
    return 0;
}

std::vector<QueueId> VulkanEngine::getAllQueues() const
{
    std::vector<QueueId> result;

    for (size_t i = 0; i < queues.size(); ++i)
    {
        result.push_back(static_cast<QueueId>(i));
    }

    return result;
}

void VulkanEngine::synchronizeQueue(const QueueId queue)
{
    if (queue >= queues.size())
    {
        throw std::runtime_error(std::string("Invalid queue index: ") + std::to_string(queue));
    }

    queues[queue].waitIdle();
}

void VulkanEngine::synchronizeDevice()
{
    device->waitIdle();
}

void VulkanEngine::clearEvents()
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::uploadArgument(KernelArgument& kernelArgument)
{
    if (kernelArgument.getUploadType() != ArgumentUploadType::Vector)
    {
        return 0;
    }

    EventId eventId = uploadArgumentAsync(kernelArgument, getDefaultQueue());
    return getArgumentOperationDuration(eventId);
}

EventId VulkanEngine::uploadArgumentAsync(KernelArgument& kernelArgument, const QueueId queue)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::updateArgument(const ArgumentId id, const void* data, const size_t dataSizeInBytes)
{
    EventId eventId = updateArgumentAsync(id, data, dataSizeInBytes, getDefaultQueue());
    return getArgumentOperationDuration(eventId);
}

EventId VulkanEngine::updateArgumentAsync(const ArgumentId id, const void* data, const size_t dataSizeInBytes, const QueueId queue)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::downloadArgument(const ArgumentId id, void* destination, const size_t dataSizeInBytes) const
{
    EventId eventId = downloadArgumentAsync(id, destination, dataSizeInBytes, getDefaultQueue());
    return getArgumentOperationDuration(eventId);
}

EventId VulkanEngine::downloadArgumentAsync(const ArgumentId id, void* destination, const size_t dataSizeInBytes, const QueueId queue) const
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

KernelArgument VulkanEngine::downloadArgumentObject(const ArgumentId id, uint64_t* downloadDuration) const
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::copyArgument(const ArgumentId destination, const ArgumentId source, const size_t dataSizeInBytes)
{
    EventId eventId = copyArgumentAsync(destination, source, dataSizeInBytes, getDefaultQueue());
    return getArgumentOperationDuration(eventId);
}

EventId VulkanEngine::copyArgumentAsync(const ArgumentId destination, const ArgumentId source, const size_t dataSizeInBytes, const QueueId queue)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::persistArgument(KernelArgument& kernelArgument, const bool flag)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

uint64_t VulkanEngine::getArgumentOperationDuration(const EventId id) const
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

void VulkanEngine::resizeArgument(const ArgumentId id, const size_t newSize, const bool preserveData)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

void VulkanEngine::setPersistentBufferUsage(const bool flag)
{
    persistentBufferFlag = flag;
}

void VulkanEngine::clearBuffer(const ArgumentId id)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

void VulkanEngine::clearBuffers()
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

void VulkanEngine::clearBuffers(const ArgumentAccessType accessType)
{
    throw std::runtime_error("Vulkan API is not yet supported");
}

void VulkanEngine::printComputeAPIInfo(std::ostream& outputTarget) const
{
    outputTarget << "Platform 0: " << "Vulkan" << std::endl;
    std::vector<VulkanPhysicalDevice> devices = instance->getPhysicalDevices();

    for (size_t i = 0; i < devices.size(); i++)
    {
        outputTarget << "Device " << i << ": " << devices.at(i).getName() << std::endl;
    }
    outputTarget << std::endl;
}

std::vector<PlatformInfo> VulkanEngine::getPlatformInfo() const
{
    PlatformInfo info(0, "Vulkan");
    info.setVendor("N/A");
    info.setVersion(instance->getAPIVersion());

    std::vector<std::string> extensions = instance->getExtensions();
    std::string mergedExtensions("");
    for (size_t i = 0; i < extensions.size(); ++i)
    {
        mergedExtensions += extensions[i];
        if (i != extensions.size() - 1)
        {
            mergedExtensions += ", ";
        }
    }
    info.setExtensions(mergedExtensions);
    return std::vector<PlatformInfo>{info};
}

std::vector<DeviceInfo> VulkanEngine::getDeviceInfo(const PlatformIndex) const
{
    std::vector<DeviceInfo> result;
    std::vector<VulkanPhysicalDevice> devices = instance->getPhysicalDevices();

    for (const auto& device : devices)
    {
        result.push_back(device.getDeviceInfo());
    }

    return result;
}

DeviceInfo VulkanEngine::getCurrentDeviceInfo() const
{
    return getDeviceInfo(0).at(deviceIndex);
}

#ifdef KTT_PROFILING

EventId VulkanEngine::runKernelWithProfiling(const KernelRuntimeData& kernelData, const std::vector<KernelArgument*>& argumentPointers,
    const QueueId queue)
{
    throw std::runtime_error("Kernel profiling is not supported for Vulkan backend");
}

KernelResult VulkanEngine::getKernelResultWithProfiling(const EventId id, const std::vector<OutputDescriptor>& outputDescriptors) const
{
    throw std::runtime_error("Kernel profiling is not supported for Vulkan backend");
}

bool VulkanEngine::hasProfilingData(const std::string& kernelName, const std::string& kernelSource) const
{
    throw std::runtime_error("Kernel profiling is not supported for Vulkan backend");
}

uint64_t VulkanEngine::getRemainingKernelProfilingRuns(const std::string& kernelName, const std::string& kernelSource) const
{
    throw std::runtime_error("Kernel profiling is not supported for Vulkan backend");
}

#endif // KTT_PROFILING

} // namespace ktt

#endif // KTT_PLATFORM_VULKAN
