#pragma once

#include <compute_engine/opencl/gpa/gpa_profiling_instance.h>
#include <compute_engine/opencl/opencl_utility.h>

#ifdef KTT_PROFILING_GPA
#include <gpu_perf_api/GPUPerfAPI.h>
#endif // KTT_PROFILING_GPA

#ifdef KTT_PROFILING_GPA_LEGACY
#include <gpu_perf_api_legacy/GPUPerfAPI.h>
#endif // KTT_PROFILING_GPA_LEGACY

namespace ktt
{

class GPAProfilingPass
{
public:
    explicit GPAProfilingPass(GPAFunctionTable& gpaFunctions, GPAProfilingInstance& instance) :
        gpaFunctions(gpaFunctions),
        instance(instance),
        passIndex(instance.getCurrentPassIndex())
    {
        checkGPAError(gpaFunctions.GPA_BeginCommandList(instance.getSession(), passIndex, GPA_NULL_COMMAND_LIST, GPA_COMMAND_LIST_NONE, &commandList),
            "GPA_BeginCommandList", gpaFunctions);
        checkGPAError(gpaFunctions.GPA_BeginSample(instance.getSampleId(), commandList), "GPA_BeginSample", gpaFunctions);
    }

    ~GPAProfilingPass()
    {
        checkGPAError(gpaFunctions.GPA_EndSample(commandList), "GPA_EndSample", gpaFunctions);
        checkGPAError(gpaFunctions.GPA_EndCommandList(commandList), "GPA_EndCommandList", gpaFunctions);
        instance.updateState();
    }

private:
    GPAFunctionTable& gpaFunctions;
    GPAProfilingInstance& instance;
    gpa_uint32 passIndex;
    GPA_CommandListId commandList;
};

} // namespace ktt
