#include "SAFTd_Proxy.hpp"
#include "MasterFunctionGenerator_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SCUbusActionSink_Proxy.hpp"
#include "CommonFunctions.hpp"
#include "CommonHelpers.hpp"

#include "FunctionGeneratorSharedMemory.hpp"

#include <condition_variable>
#include <variant>
#include <optional>
#include <random>

using namespace test::system::FunctionGenerator::Helpers;

namespace
{
    std::vector<bool> refillRequested;
    std::vector<std::string> FUNCTION_GENERATOR_NAMES;
}

void FillSharedMemoryWithRandomData()
{

}

int main(int argc, char **argv)
{
    auto saftlibComponentsOrError = CreateSaftlibComponents();
    if (std::holds_alternative<SaftlibInitializationError>(saftlibComponentsOrError))
    {
        auto &error = std::get<SaftlibInitializationError>(saftlibComponentsOrError);
        std::cerr << "Error while creating saftlib components:" << error.message << "\n";
        return -1;
    }

    auto &components = std::get<SaftlibComponents>(saftlibComponentsOrError);

    auto onRefillRequested = [&](std::string requestingFunctionGeneratorName)
    {
        auto it = std::find(FUNCTION_GENERATOR_NAMES.begin(), FUNCTION_GENERATOR_NAMES.end(), requestingFunctionGeneratorName);
        if (it != FUNCTION_GENERATOR_NAMES.end())
        {
            std::cout << "\t\tRefill requested by function generator: " << requestingFunctionGeneratorName << "\n";
            size_t index = std::distance(FUNCTION_GENERATOR_NAMES.begin(), it);
            refillRequested[index] = true;
        }
        else
        {
            std::cerr << "Received refill request from unknown function generator: " << requestingFunctionGeneratorName << "\n";
        }
    };

    [[maybe_unused]] sigc::connection refillConnection = components.masterFunctionGeneratorProxy->Refill.connect(std::move(onRefillRequested));

    FUNCTION_GENERATOR_NAMES = components.masterFunctionGeneratorProxy->ReadAllNames();
    const auto numberOfFunctionGenerators = FUNCTION_GENERATOR_NAMES.size();

    test::system::FunctionGenerator::FunctionGeneratorSharedMemory sharedMemory("MasterFunctionGeneratorTest", 40000000);

    for(uint32_t beamProcess = 0; beamProcess < 100; ++beamProcess)
    {
        auto tupleSet = GenerateRandomTupleSet(3000, 3000);
        auto tupleSetSize = tupleSet.size();
        for(size_t functionGeneratorIndex = 0; functionGeneratorIndex < numberOfFunctionGenerators; ++functionGeneratorIndex)
        {
            sharedMemory.PrepareFgData(tupleSet, functionGeneratorIndex, beamProcess);
        }
    }

    ParameterSets parameterSets(numberOfFunctionGenerators);
    for (size_t i = 0; i < numberOfFunctionGenerators; ++i)
    {
        auto parameterSet = GenerateRandomWaveformParameters(100);
        parameterSets[i].coeff_a = std::move(parameterSet.coeff_a);
        parameterSets[i].coeff_b = std::move(parameterSet.coeff_b);
        parameterSets[i].coeff_c = std::move(parameterSet.coeff_c);
        parameterSets[i].step = std::move(parameterSet.step);
        parameterSets[i].freq = std::move(parameterSet.freq);
        parameterSets[i].shift_a = std::move(parameterSet.shift_a);
        parameterSets[i].shift_b = std::move(parameterSet.shift_b);
    }
    components.masterFunctionGeneratorProxy->Own();
    components.masterFunctionGeneratorProxy->InitializeSharedMemory("MasterFunctionGeneratorTest");

    components.masterFunctionGeneratorProxy->setGenerateIndividualSignals(true);


    refillRequested.resize(FUNCTION_GENERATOR_NAMES.size(), false);
    components.masterFunctionGeneratorProxy->SetActiveFunctionGenerators(FUNCTION_GENERATOR_NAMES);

    auto start = std::chrono::steady_clock::now();

    BringFunctionGeneratorIntoKnownState(*components.masterFunctionGeneratorProxy);

    // PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);

    /*
    FillFunctionGeneratorBuffer(*components.masterFunctionGeneratorProxy,
                                parameterSets);
    */
   /*
    for (size_t functionGenerator = 0; functionGenerator < numberOfFunctionGenerators; ++functionGenerator)
    {
        FillSingleFunctionGeneratorBuffer(*components.masterFunctionGeneratorProxy, functionGenerator, parameterSets);
    }

    */
    components.masterFunctionGeneratorProxy->AppendParameterTuplesForBeamProcess(1, false, false);
    components.masterFunctionGeneratorProxy->setStartTag(1337);
    components.masterFunctionGeneratorProxy->Arm();
    while (!AllArmed(*components.masterFunctionGeneratorProxy))
    {
        // PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
    }

    auto end = std::chrono::steady_clock::now();

    auto durationInMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Upload and arm took: " << durationInMs.count() << std::endl;

    PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
}