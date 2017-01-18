#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../../include/ktt.h"

int main(int argc, char** argv)
{
    // Initialize platform and device index
    int platformIndex = 0;
    int deviceIndex = 0;

    if (argc >= 2)
    {
        platformIndex = std::stoi(std::string{ argv[1] });
        if (argc >= 3)
        {
            deviceIndex = std::stoi(std::string{ argv[2] });
        }
    }

    // Declare constants
    const float UPPER_INTERVAL_BOUNDARY = 1000.0f; // used for generating random test data
    const std::string kernelName = std::string("simple_kernel.cl");
    const std::string referenceKernelName = std::string("simple_reference_kernel.cl");

    // Declare kernel parameters
    const int numberOfElements = 4096 * 4096;
    std::vector<size_t> ndRangeDimensions{ 4096 * 4096, 1 };
    std::vector<size_t> workGroupDimensions{ 256, 1 };

    // Declare data variables
    std::vector<float> a(numberOfElements);
    std::vector<float> b(numberOfElements);
    std::vector<float> result(numberOfElements, 0.0f);

    // Initialize data
    srand((unsigned)time(0));

    for (int i = 0; i < numberOfElements; i++)
    {
        a.at(i) = (float)rand() / (RAND_MAX / UPPER_INTERVAL_BOUNDARY);
        b.at(i) = (float)rand() / (RAND_MAX / UPPER_INTERVAL_BOUNDARY);
    }
    
    // to do

    return 0;
}
