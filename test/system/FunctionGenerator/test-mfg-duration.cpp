
#include "SAFTd_Proxy.hpp"
#include "MasterFunctionGenerator_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SCUbusActionSink_Proxy.hpp"
#include "CommonFunctions.hpp"
#include "CommonHelpers.hpp"

#include <random>
#include <algorithm>

using namespace test::system::FunctionGenerator::Helpers;

namespace {
    std::vector<bool> refillRequested;
    std::vector<std::string> FUNCTION_GENERATOR_NAMES;

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    std::default_random_engine generator;

    static constexpr uint8_t DURATION_IN_SECONDS = 16;

    // needed for the calculation of the waveform duration
    // duration in  nanoseconds depending on steps
    static constexpr std::array<int, 8> SAMPLES = {250, 500, 1000, 2000, 4000, 8000, 16000, 32000};

    // duration in nanoseconds depending on frequency parameter
    static constexpr std::array<int, 8> SAMPLE_LEN = {
             62500, // 16kHz in ns
             31250, // 32kHz
             15625, // 64kHz
              8000, // 125kHz
              4000, // 250kHz
              2000, // 500kHz
              1000, // 1GHz
               500  // 2GHz
    };

    // duration = samples[step] * sample_len[freq]
    // {ns, steps, frequency}
    constexpr static std::array<std::array<int, 3>, 64> ALL_DURATIONS = {{
        {125000,0,7},       {250000,1,7},       {250000,0,6},       {500000,2,7},
        {500000,0,5},       {500000,1,6},       {1000000,0,4},      {1000000,3,7},
        {1000000,1,5},      {1000000,2,6},      {2000000,3,6},      {2000000,4,7},
        {2000000,1,4},      {2000000,0,3},      {2000000,2,5},      {3906250,0,2},
        {4000000,2,4},      {4000000,1,3},      {4000000,5,7},      {4000000,3,5},
        {4000000,4,6},      {7812500,0,1},      {7812500,1,2},      {8000000,2,3},
        {8000000,5,6},      {8000000,4,5},      {8000000,6,7},      {8000000,3,4},
        {15625000,2,2},     {15625000,1,1},     {15625000,0,0},     {16000000,7,7},
        {16000000,4,4},     {16000000,5,5},     {16000000,6,6},     {16000000,3,3},
        {31250000,1,0},     {31250000,2,1},     {31250000,3,2},     {32000000,4,3},
        {32000000,7,6},     {32000000,6,5},     {32000000,5,4},     {62500000,4,2},
        {62500000,2,0},     {62500000,3,1},     {64000000,5,3},     {64000000,7,5},
        {64000000,6,4},     {125000000,3,0},    {125000000,4,1},    {125000000,5,2},
        {128000000,6,3},    {128000000,7,4},    {250000000,6,2},    {250000000,5,1},
        {250000000,4,0},    {256000000,7,3},    {500000000,5,0},    {500000000,7,2},
        {500000000,6,1},    {1000000000,7,1},   {1000000000,6,0},   {2000000000,7,0}
    }};

    // In MIL-bus settings: minimum step parameter of 3 is necessary
    // {ns, steps, frequency}
    constexpr static std::array<std::array<int, 3>, 40> MIL_DURATIONS = {{
        {1000000,3,7},      {2000000,3,6},      {2000000,4,7},      {4000000,5,7},
        {4000000,3,5},      {4000000,4,6},      {8000000,5,6},      {8000000,4,5},
        {8000000,6,7},      {8000000,3,4},      {16000000,7,7},     {16000000,4,4},
        {16000000,5,5},     {16000000,6,6},     {16000000,3,3},     {31250000,3,2},
        {32000000,4,3},     {32000000,7,6},     {32000000,6,5},     {32000000,5,4},
        {62500000,4,2},     {62500000,3,1},     {64000000,5,3},     {64000000,7,5},
        {64000000,6,4},     {125000000,3,0},    {125000000,4,1},    {125000000,5,2},
        {128000000,6,3},    {128000000,7,4},    {250000000,6,2},    {250000000,5,1},
        {250000000,4,0},    {256000000,7,3},    {500000000,5,0},    {500000000,7,2},
        {500000000,6,1},    {1000000000,7,1},   {1000000000,6,0},   {2000000000,7,0}
    }};

    struct StepFreqDuration
    {
      uint8_t step;
      uint8_t freq;
      uint64_t duration;
    };

}

static StepFreqDuration GenerateRandomDurationTuple(uint64_t max_duration_ns)
{
    StepFreqDuration data;
    uint8_t index = 0 + static_cast<unsigned char>(distribution(generator) * MIL_DURATIONS.size());
    uint64_t current_duration = MIL_DURATIONS[index][0];

    while (current_duration > max_duration_ns)
    {
        if (index == 0) {break;}
        index--;
        current_duration = MIL_DURATIONS[index][0];
    }

    // std::cout << "index: " << index << "\n";
    // std::cout << "MIL_DURATIONS[index][1]: " << MIL_DURATIONS[index][1]<< "\n";
    // std::cout << "MIL_DURATIONS[index][1]: " << MIL_DURATIONS[index][2]<< "\n";

    data.step = static_cast<uint8_t>(MIL_DURATIONS[index][1]);
    data.freq = static_cast<uint8_t>(MIL_DURATIONS[index][2]);
    data.duration = current_duration;

    // std::cout << std::left
    //     << std::setw(20) << max_duration_ns
    //     << std::setw(20) << data.duration
    //     << std::setw(20) << static_cast<int>(data.step)
    //     << std::setw(20) << static_cast<int>(data.freq)
    //     << "\n";

    return data;
}

static ParameterSet GenerateDurationParameterSet(uint64_t whole_duration_ns)
{
    StepFreqDuration data;
    std::vector<StepFreqDuration> result;

    const auto max_iteration = 10000;
    const auto min_duration_ns = 1000000;
    auto left_duration = whole_duration_ns;

    for (size_t i = 0; i < max_iteration; i++)
    {
        data = GenerateRandomDurationTuple(left_duration);
        result.push_back(data);
        left_duration -= data.duration;
        if (left_duration < min_duration_ns) break;
    }

    ParameterSet params;
    params.coeff_a.resize(result.size());
    params.coeff_b.resize(result.size());
    params.coeff_c.resize(result.size());
    params.step.resize(result.size());
    params.freq.resize(result.size());
    params.shift_a.resize(result.size());
    params.shift_b.resize(result.size());

    for (size_t i = 0; i < result.size(); ++i)
        {
            params.coeff_a[i] = 0;
            params.coeff_b[i] = 100;
            params.coeff_c[i] = 4000000000;
            params.step[i] = result[i].step;
            params.freq[i] = result[i].freq;
            params.shift_a[i] = 0;  // 0-63
            params.shift_b[i] = 48; // 0-63
        }

    return params;
}

int main(int argc, char **argv)
{

    uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::seconds(DURATION_IN_SECONDS)
    ).count();

    const auto min_duration_ns = 1000000;
    // add minimum duration to overstep desired duration
    // by less than 1000000 ns = 0.001 seconds
    duration += min_duration_ns;

    // Now let`s do the same things as the other tests
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
        // for each FG find a set of parameters that approx. the desired duration
        auto parameterSet = GenerateDurationParameterSet(duration);
        parameterSets[i].coeff_a = std::move(parameterSet.coeff_a);
        parameterSets[i].coeff_b = std::move(parameterSet.coeff_b);
        parameterSets[i].coeff_c = std::move(parameterSet.coeff_c);
        parameterSets[i].step = std::move(parameterSet.step);
        parameterSets[i].freq = std::move(parameterSet.freq);
        parameterSets[i].shift_a = std::move(parameterSet.shift_a);
        parameterSets[i].shift_b = std::move(parameterSet.shift_b);
    }

    std::cout << "Generated parameters with duration " << duration  << "ns." << "\n";
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

    //Is this the right moment?
    SetAndFireECPUCond(components.receiver);

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