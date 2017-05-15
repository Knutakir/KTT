#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

#include "tuning_runner.h"
#include "searcher/annealing_searcher.h"
#include "searcher/full_searcher.h"
#include "searcher/pso_searcher.h"
#include "searcher/random_searcher.h"
#include "../utility/ktt_utility.h"
#include "../utility/timer.h"

namespace ktt
{

TuningRunner::TuningRunner(ArgumentManager* argumentManager, KernelManager* kernelManager, Logger* logger, ComputeApiDriver* computeApiDriver) :
    argumentManager(argumentManager),
    kernelManager(kernelManager),
    logger(logger),
    computeApiDriver(computeApiDriver),
    resultValidator(logger),
    argumentPrinter(logger),
    manipulatorInterfaceImplementation(std::make_unique<ManipulatorInterfaceImplementation>(computeApiDriver))
{}

std::vector<TuningResult> TuningRunner::tuneKernel(const size_t id)
{
    if (id >= kernelManager->getKernelCount())
    {
        throw std::runtime_error(std::string("Invalid kernel id: ") + std::to_string(id));
    }

    std::vector<TuningResult> results;
    Kernel* kernel = kernelManager->getKernel(id);
    std::unique_ptr<Searcher> searcher = getSearcher(kernel->getSearchMethod(), kernel->getSearchArguments(),
        kernelManager->getKernelConfigurations(id), kernel->getParameters());
    size_t configurationsCount = searcher->getConfigurationsCount();

    for (size_t i = 0; i < configurationsCount; i++)
    {
        KernelConfiguration currentConfiguration = searcher->getNextConfiguration();
        KernelRunResult result;
        uint64_t manipulatorDuration = 0;

        try
        {
            auto resultPair = runKernel(kernel, currentConfiguration, i, configurationsCount);
            result = resultPair.first;
            manipulatorDuration = resultPair.second;
        }
        catch (const std::runtime_error& error)
        {
            logger->log(std::string("Kernel run failed, reason: ") + error.what() + "\n");
        }

        searcher->calculateNextConfiguration(static_cast<double>(result.getDuration() + manipulatorDuration));
        if (processResult(kernel, result, manipulatorDuration, currentConfiguration))
        {
            results.emplace_back(TuningResult(kernel->getName(), result.getDuration(), manipulatorDuration, currentConfiguration));
        }
    }

    computeApiDriver->clearCache();
    resultValidator.clearReferenceResults();
    return results;
}

void TuningRunner::setValidationMethod(const ValidationMethod& validationMethod, const double toleranceThreshold, const size_t validationRange)
{
    resultValidator.setValidationMethod(validationMethod);
    resultValidator.setToleranceThreshold(toleranceThreshold);
    resultValidator.setValidationRange(validationRange);
}

void TuningRunner::enableArgumentPrinting(const size_t argumentId, const std::string& filePath, const ArgumentPrintCondition& argumentPrintCondition)
{
    argumentPrinter.setArgumentPrintData(argumentId, filePath, argumentPrintCondition);
}

std::pair<KernelRunResult, uint64_t> TuningRunner::runKernel(Kernel* kernel, const KernelConfiguration& currentConfiguration,
    const size_t currentConfigurationIndex, const size_t configurationsCount)
{
    KernelRunResult result;
    size_t kernelId = kernel->getId();
    std::string kernelName = kernel->getName();
    std::string source = kernelManager->getKernelSourceWithDefines(kernelId, currentConfiguration);
    std::stringstream stream;

    if (kernel->hasTuningManipulator())
    {
        stream << "Launching kernel <" << kernelName << "> (custom manipulator detected) with configuration (" << currentConfigurationIndex + 1
            << " / " << configurationsCount << "): " << currentConfiguration;
        logger->log(stream.str());
        auto kernelDataVector = getKernelDataVector(kernelId, KernelRuntimeData(kernelName, source, currentConfiguration.getGlobalSize(),
            currentConfiguration.getLocalSize(), kernel->getArgumentIndices()), kernel->getTuningManipulator()->getUtilizedKernelIds(),
            currentConfiguration);
        return runKernelWithManipulator(kernel->getTuningManipulator(), kernelDataVector, currentConfiguration);
    }

    stream << "Launching kernel <" << kernelName << "> with configuration (" << currentConfigurationIndex + 1  << " / " << configurationsCount
        << "): " << currentConfiguration;
    logger->log(stream.str());
    result = computeApiDriver->runKernel(source, kernel->getName(), convertDimensionVector(currentConfiguration.getGlobalSize()),
        convertDimensionVector(currentConfiguration.getLocalSize()), getKernelArguments(kernelId));
    return std::make_pair(result, 0);
}

std::pair<KernelRunResult, uint64_t> TuningRunner::runKernelWithManipulator(TuningManipulator* manipulator,
    const std::vector<std::pair<size_t, KernelRuntimeData>>& kernelDataVector, const KernelConfiguration& currentConfiguration)
{
    manipulator->manipulatorInterface = manipulatorInterfaceImplementation.get();
    std::vector<KernelArgument> kernelArguments;
    for (const auto& kernelData : kernelDataVector)
    {
        manipulatorInterfaceImplementation->addKernel(kernelData.first, kernelData.second);
        auto currentArguments = getKernelArguments(kernelData.first);
        for (const auto& argument : currentArguments)
        {
            if (!elementExists(argument, kernelArguments))
            {
                kernelArguments.push_back(argument);
            }
        }
    }
    manipulatorInterfaceImplementation->setKernelArguments(kernelArguments);

    Timer timer;
    timer.start();
    manipulator->launchComputation(kernelDataVector.at(0).first, currentConfiguration.getGlobalSize(), currentConfiguration.getLocalSize(),
        currentConfiguration.getParameterValues());
    timer.stop();

    KernelRunResult result = manipulatorInterfaceImplementation->getCurrentResult();
    size_t manipulatorDuration = timer.getElapsedTime();
    manipulatorDuration -= result.getOverhead();

    manipulatorInterfaceImplementation->clearData();
    manipulator->manipulatorInterface = nullptr;
    return std::make_pair(result, manipulatorDuration);
}

std::unique_ptr<Searcher> TuningRunner::getSearcher(const SearchMethod& searchMethod, const std::vector<double>& searchArguments,
    const std::vector<KernelConfiguration>& configurations, const std::vector<KernelParameter>& parameters) const
{
    std::unique_ptr<Searcher> searcher;

    switch (searchMethod)
    {
    case SearchMethod::FullSearch:
        searcher.reset(new FullSearcher(configurations));
        break;
    case SearchMethod::RandomSearch:
        searcher.reset(new RandomSearcher(configurations, searchArguments.at(0)));
        break;
    case SearchMethod::PSO:
        searcher.reset(new PSOSearcher(configurations, parameters, searchArguments.at(0), static_cast<size_t>(searchArguments.at(1)),
            searchArguments.at(2), searchArguments.at(3), searchArguments.at(4)));
        break;
    default:
        searcher.reset(new AnnealingSearcher(configurations, searchArguments.at(0), searchArguments.at(1)));
    }

    return searcher;
}

std::vector<KernelArgument> TuningRunner::getKernelArguments(const size_t kernelId) const
{
    std::vector<KernelArgument> result;

    std::vector<size_t> argumentIndices = kernelManager->getKernel(kernelId)->getArgumentIndices();
    
    for (const auto index : argumentIndices)
    {
        result.push_back(argumentManager->getArgument(index));
    }

    return result;
}

std::vector<std::pair<size_t, KernelRuntimeData>> TuningRunner::getKernelDataVector(const size_t tunedKernelId,
    const KernelRuntimeData& tunedKernelData, const std::vector<std::pair<size_t, ThreadSizeUsage>>& additionalKernelData,
    const KernelConfiguration& currentConfiguration) const
{
    std::vector<std::pair<size_t, KernelRuntimeData>> result;
    result.push_back(std::make_pair(tunedKernelId, tunedKernelData));

    for (const auto& kernelDataPair : additionalKernelData)
    {
        if (kernelDataPair.first == tunedKernelId)
        {
            continue;
        }

        Kernel* kernel = kernelManager->getKernel(kernelDataPair.first);
        std::string source = kernelManager->getKernelSourceWithDefines(kernelDataPair.first, currentConfiguration);

        if (kernelDataPair.second == ThreadSizeUsage::Basic)
        {
            result.push_back(std::make_pair(kernelDataPair.first, KernelRuntimeData(kernel->getName(), source, kernel->getGlobalSize(),
                kernel->getLocalSize(), kernel->getArgumentIndices())));
        }
        else
        {
            KernelConfiguration configuration = kernelManager->getKernelConfiguration(kernelDataPair.first,
                currentConfiguration.getParameterValues());
            result.push_back(std::make_pair(kernelDataPair.first, KernelRuntimeData(kernel->getName(), source, configuration.getGlobalSize(),
                configuration.getLocalSize(), kernel->getArgumentIndices())));
        }
    }

    return result;
}

bool TuningRunner::processResult(const Kernel* kernel, const KernelRunResult& result, const uint64_t manipulatorDuration,
    const KernelConfiguration& kernelConfiguration)
{
    if (!result.isValid())
    {
        return false;
    }

    bool resultIsCorrect = true;
    if (kernel->hasReferenceClass() || kernel->hasReferenceKernel())
    {
        resultIsCorrect = validateResult(kernel, result);
    }

    if (resultIsCorrect)
    {
        logger->log(std::string("Kernel run completed successfully in ") + std::to_string((result.getDuration() + manipulatorDuration) / 1'000'000)
            + "ms\n");
    }
    else
    {
        logger->log("Kernel run completed successfully, but results differ\n");
    }

    for (const auto& argument : result.getResultArguments())
    {
        if (argumentPrinter.argumentPrintDataExists(argument.getId()))
        {
            argumentPrinter.printArgument(argument, kernel->getName(), kernelConfiguration, resultIsCorrect);
        }
    }

    return resultIsCorrect;
}

bool TuningRunner::validateResult(const Kernel* kernel, const KernelRunResult& result)
{
    bool validationResult = true;

    if (kernel->hasReferenceClass())
    {
        validationResult &= validateResult(kernel, result, true);
    }

    if (kernel->hasReferenceKernel())
    {
        validationResult &= validateResult(kernel, result, false);
    }

    return validationResult;
}

bool TuningRunner::validateResult(const Kernel* kernel, const KernelRunResult& result, bool useReferenceClass)
{
    std::vector<size_t> indices = kernel->getArgumentIndices();
    std::vector<size_t> referenceIndices;
    if (useReferenceClass)
    {
        referenceIndices = kernel->getReferenceClassArgumentIds();
    }
    else
    {
        referenceIndices = kernel->getReferenceKernelArgumentIds();
    }

    for (const auto argumentId : referenceIndices)
    {
        if (!elementExists(argumentId, indices))
        {
            throw std::runtime_error(std::string("Reference argument with following id is not associated with given kernel: ")
                + std::to_string(argumentId));
        }

        if (argumentManager->getArgument(argumentId).getArgumentMemoryType() == ArgumentMemoryType::ReadOnly)
        {
            throw std::runtime_error(std::string("Reference argument with following id is marked as read only: ") + std::to_string(argumentId));
        }
    }

    if (useReferenceClass && !resultValidator.hasReferenceClassResult(kernel->getId()))
    {
        std::vector<KernelArgument> referenceClassResult = getReferenceResultFromClass(kernel->getReferenceClass(), referenceIndices);
        if (referenceClassResult.size() != referenceIndices.size())
        {
            throw std::runtime_error(std::string("Reference class argument count does not match tuned kernel argument count for kernel with id: ")
                + std::to_string(kernel->getId()));
        }
        resultValidator.setReferenceClassResult(kernel->getId(), referenceClassResult);
    }
    else if (!useReferenceClass && !resultValidator.hasReferenceKernelResult(kernel->getId()))
    {
        size_t referenceKernelId = kernel->getReferenceKernelId();
        std::vector<KernelArgument> referenceKernelResult = getReferenceResultFromKernel(referenceKernelId,
            kernel->getReferenceKernelConfiguration(), referenceIndices);
        if (referenceKernelResult.size() != referenceIndices.size())
        {
            throw std::runtime_error(std::string("Reference kernel argument count does not match tuned kernel argument count for kernel with id: ")
                + std::to_string(kernel->getId()));
        }
        resultValidator.setReferenceKernelResult(kernel->getId(), referenceKernelResult);
    }

    std::vector<size_t> argumentIndicesInResult;
    const auto& resultArguments = result.getResultArguments();

    for (size_t i = 0; i < resultArguments.size(); i++)
    {
        if (elementExists(resultArguments.at(i).getId(), referenceIndices))
        {
            argumentIndicesInResult.push_back(i);
        }
    }

    std::vector<KernelArgument> argumentsToValidate;
    for (const auto index : argumentIndicesInResult)
    {
        argumentsToValidate.push_back(resultArguments.at(index));
    }

    if (useReferenceClass)
    {
        return resultValidator.validateArgumentWithClass(kernel->getId(), argumentsToValidate);
    }
    return resultValidator.validateArgumentWithKernel(kernel->getId(), argumentsToValidate);
}

std::vector<KernelArgument> TuningRunner::getReferenceResultFromClass(const ReferenceClass* referenceClass,
    const std::vector<size_t>& referenceArgumentIndices) const
{
    std::vector<KernelArgument> resultArguments;

    for (const auto referenceArgumentId : referenceArgumentIndices)
    {
        size_t numberOfElements = referenceClass->getNumberOfElements(referenceArgumentId);
        if (numberOfElements == 0)
        {
            throw std::runtime_error(std::string("Data provided by reference class for argument with following id is empty: ")
                + std::to_string(referenceArgumentId));
        }
        size_t elementSize = referenceClass->getElementSizeInBytes(referenceArgumentId);
        if (elementSize == 0)
        {
            throw std::runtime_error(std::string("Invalid element size provided by reference class for argument with following id: ")
                + std::to_string(referenceArgumentId));
        }
        ArgumentDataType dataType = referenceClass->getDataType(referenceArgumentId);

        resultArguments.emplace_back(KernelArgument(referenceArgumentId, referenceClass->getData(referenceArgumentId), numberOfElements, dataType,
            ArgumentMemoryType::ReadWrite, ArgumentUploadType::Vector));
    }
    
    return resultArguments;
}

std::vector<KernelArgument> TuningRunner::getReferenceResultFromKernel(const size_t referenceKernelId,
    const std::vector<ParameterValue>& referenceKernelConfiguration, const std::vector<size_t>& referenceArgumentIndices) const
{
    const Kernel* referenceKernel = kernelManager->getKernel(referenceKernelId);
    KernelConfiguration configuration = kernelManager->getKernelConfiguration(referenceKernelId, referenceKernelConfiguration);
    std::string source = kernelManager->getKernelSourceWithDefines(referenceKernelId, configuration);

    auto result = computeApiDriver->runKernel(source, referenceKernel->getName(), convertDimensionVector(configuration.getGlobalSize()),
        convertDimensionVector(configuration.getLocalSize()), getKernelArguments(referenceKernelId));
    std::vector<KernelArgument> resultArguments;

    for (const auto& argument : result.getResultArguments())
    {
        if (elementExists(argument.getId(), referenceArgumentIndices))
        {
            resultArguments.push_back(argument);
        }
    }

    return resultArguments;
}

} // namespace ktt
