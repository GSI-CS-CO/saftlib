
#include "SAFTd_Proxy.hpp"
#include "MasterFunctionGenerator_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SCUbusActionSink_Proxy.hpp"
#include "CommonFunctions.hpp"
#include "CommonHelpers.hpp"

#include <condition_variable>
#include <variant>
#include <optional>
#include <random>


using namespace test::system::FunctionGenerator::Helpers;

namespace {
    std::vector<bool> refillRequested;
    std::vector<std::string> FUNCTION_GENERATOR_NAMES;
}

int main(int argc, char **argv)
{
    auto componentsOrError = CreateSaftlibComponents();
    if (std::holds_alternative<SaftlibInitializationError>(componentsOrError))
    {
        auto error = std::get<SaftlibInitializationError>(componentsOrError);
        std::cerr << "Error during saftlib components creation: " << error.message << "\n";
        return 1;
    }

    auto& components = std::get<SaftlibComponents>(componentsOrError);

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

    if (refillConnection.connected())
    {
        std::cout << "Successfully connected to FunctionGenerator Refill signal." << "\n";
    }
    else
    {
        std::cerr << "Failed to connect to FunctionGenerator Refill signal." << "\n";
    }

    components.masterFunctionGeneratorProxy->Own();
    components.masterFunctionGeneratorProxy->setGenerateIndividualSignals(true);

    BringFunctionGeneratorIntoKnownState(*components.masterFunctionGeneratorProxy);

    FUNCTION_GENERATOR_NAMES = components.masterFunctionGeneratorProxy->ReadAllNames();
    refillRequested.resize(FUNCTION_GENERATOR_NAMES.size(), false);
    PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
    components.masterFunctionGeneratorProxy->SetActiveFunctionGenerators(FUNCTION_GENERATOR_NAMES);
    const auto numberOfFunctionGenerators = FUNCTION_GENERATOR_NAMES.size();

    ParameterSets parameterSets(numberOfFunctionGenerators);
    for (size_t i = 0; i < numberOfFunctionGenerators; ++i)
    {
        auto parameterSet = GenerateRandomWaveformParameters(1000);
        parameterSets[i].coeff_a = std::move(parameterSet.coeff_a);
        parameterSets[i].coeff_b = std::move(parameterSet.coeff_b);
        parameterSets[i].coeff_c = std::move(parameterSet.coeff_c);
        parameterSets[i].step = std::move(parameterSet.step);
        parameterSets[i].freq = std::move(parameterSet.freq);
        parameterSets[i].shift_a = std::move(parameterSet.shift_a);
        parameterSets[i].shift_b = std::move(parameterSet.shift_b);
    }

    std::cout << "Generated random waveform parameters." << "\n";
    std::cout << "Filling function generator buffer." << "\n";

    FillFunctionGeneratorBuffer(*components.masterFunctionGeneratorProxy,
                                parameterSets);

    PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
    std::cout << "Function generator buffer filled." << "\n";
    PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);

    // Arm and start the function generator
    std::cout << "Arming and starting the function generator." << "\n";
    {
        std::cout << "Setting StartTag to 1337 and arming function generator." << "\n";
        components.masterFunctionGeneratorProxy->setStartTag(1337);
        PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);

        std::cout << "Arming function generator." << "\n";
        components.masterFunctionGeneratorProxy->Arm();
        PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);

        while (!AllArmed(*components.masterFunctionGeneratorProxy))
        {
            PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
            saftlib::wait_for_signal(10);
        }

        std::cout << "Function generator armed. Injecting tag to start waveform generation." << "\n";
        components.scuProxy->InjectTag(1337);
        PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);

        while (!AllUnarmed(*components.masterFunctionGeneratorProxy))
        {
            PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
            saftlib::wait_for_signal(10);
        }
    }

    for (int32_t i = 0; i < 20; ++i)
    {
        HandleFillRequest(*components.masterFunctionGeneratorProxy,
                            refillRequested,
                            parameterSets);
        saftlib::wait_for_signal(1000);
        PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
    }

    StopFunctionGenerator(*components.masterFunctionGeneratorProxy);
    BringFunctionGeneratorIntoKnownState(*components.masterFunctionGeneratorProxy);
    PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);

    FillFunctionGeneratorBuffer(*components.masterFunctionGeneratorProxy,
                                parameterSets);

    components.masterFunctionGeneratorProxy->setStartTag(1337);
    components.masterFunctionGeneratorProxy->Arm();

    while (!AllArmed(*components.masterFunctionGeneratorProxy))
    {
        PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
        saftlib::wait_for_signal(10);
    }

    components.scuProxy->InjectTag(1337);

    while (true)
    {   
        HandleFillRequest(*components.masterFunctionGeneratorProxy,
                            refillRequested,
                            parameterSets);
        saftlib::wait_for_signal(1000);
        PrintFunctionGeneratorStates(*components.masterFunctionGeneratorProxy, refillRequested);
    }

    // Abort the function generator
    StopFunctionGenerator(*components.masterFunctionGeneratorProxy);
    BringFunctionGeneratorIntoKnownState(*components.masterFunctionGeneratorProxy);

    return 0;
}