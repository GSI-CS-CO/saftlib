
#include "SAFTd_Proxy.hpp"
#include "MasterFunctionGenerator_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SCUbusActionSink_Proxy.hpp"
#include "FunctionGeneratorSharedMemory.hpp"
#include "CommonFunctions.hpp"
#include "CommonHelpers.hpp"

#include <thread>
#include <random>
#include <algorithm>
#include <chrono>

using namespace test::system::FunctionGenerator::Helpers;

namespace
{
    std::vector<std::string> FUNCTION_GENERATOR_NAMES;
    std::chrono::steady_clock::time_point LAST_EVENT_TIME;

    void OnAllArmed(SaftlibComponents &components)
    {
        std::cout << "Function generator all armed\n";
        std::cout << "Time till armed" << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - LAST_EVENT_TIME).count() << " ms\n";
        LAST_EVENT_TIME = std::chrono::steady_clock::now();
        TriggerFireCondition(components.receiver);
    }

    void OnAllStopped(SaftlibComponents &components, saftlib::Time timestamp)
    {
        std::cout << "All function generators have stopped." << "\n";
        std::cout << "Time till stopped: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - LAST_EVENT_TIME).count() << " ms\n";
        LAST_EVENT_TIME = std::chrono::steady_clock::now();
        components.masterFunctionGeneratorProxy->AppendParameterTuplesForBeamProcess(1);
        components.masterFunctionGeneratorProxy->Arm();
    }

}

int main(int argc, char **argv)
{
    auto duration = std::chrono::milliseconds(14);

    auto componentsOrError = CreateSaftlibComponents();
    auto couldNotCreateSaftlibComponents = std::holds_alternative<SaftlibInitializationError>(componentsOrError);

    if (couldNotCreateSaftlibComponents)
    {
        auto error = std::get<SaftlibInitializationError>(componentsOrError);
        std::cerr << "Error during saftlib components creation: " << error.message << "\n";
        return 1;
    }

    auto &components = std::get<SaftlibComponents>(componentsOrError);
    auto &masterFunctionGeneratorProxy = components.masterFunctionGeneratorProxy;

    masterFunctionGeneratorProxy->Own();
    masterFunctionGeneratorProxy->setGenerateIndividualSignals(true);

    FUNCTION_GENERATOR_NAMES = masterFunctionGeneratorProxy->ReadAllNames();
    masterFunctionGeneratorProxy->SetActiveFunctionGenerators(FUNCTION_GENERATOR_NAMES);
    const auto numberOfFunctionGenerators = FUNCTION_GENERATOR_NAMES.size();

    test::system::FunctionGenerator::FunctionGeneratorSharedMemory sharedMemory("MasterFunctionGeneratorTest", 40000000);
    auto parameters = GenerateRandomParameterTuples(duration);
    for (size_t functionGeneratorIndex = 0; functionGeneratorIndex < numberOfFunctionGenerators; ++functionGeneratorIndex)
    {
        sharedMemory.PrepareFgData(parameters, functionGeneratorIndex, 0);
    }

    masterFunctionGeneratorProxy->InitializeSharedMemory("MasterFunctionGeneratorTest");

    BringFunctionGeneratorIntoKnownState(*masterFunctionGeneratorProxy);
    auto condition = RegisterFireEventCondition(components.receiver);

    std::chrono::steady_clock::time_point start;
    [[maybe_unused]] sigc::connection allArmedConnection = masterFunctionGeneratorProxy->AllArmed.connect(std::bind(&OnAllArmed, std::ref(components)));
    [[maybe_unused]] sigc::connection allStoppedConnection = masterFunctionGeneratorProxy->SigAllStopped.connect(std::bind(&OnAllStopped, std::ref(components), std::placeholders::_1));

    start = std::chrono::steady_clock::now();
    masterFunctionGeneratorProxy->AppendParameterTuplesForBeamProcess(0);
    masterFunctionGeneratorProxy->Arm();

    while (true)
    {
        saftlib::wait_for_signal(100);
    }
    return 0;
}