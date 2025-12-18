#include "CommonHelpers.hpp"
#include <random>

#include <saftlib.h>
#include <MasterFunctionGenerator_Proxy.hpp>
#include <TimingReceiver_Proxy.hpp>
#include <SCUbusActionSink_Proxy.hpp>
#include <SAFTd_Proxy.hpp>

namespace
{
    /// Indicate if the function generator has filled enough data so it would signal the refill signal when running
    bool fillLow = false;

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    std::default_random_engine generator;

    // needed for the calculation of the waveform duration
    static constexpr std::array<int, 8> SAMPLES = {250, 500, 1000, 2000, 4000, 8000, 16000, 32000};

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

    static constexpr auto DURATIONS = []() {
        std::array<std::array<int, SAMPLES.size()>, SAMPLE_LEN.size()> result{};
        for (size_t i = 0; i < SAMPLES.size(); ++i) {
            for (size_t j = 0; j < SAMPLE_LEN.size(); ++j) {
                result[i][j] = SAMPLES[i] * SAMPLE_LEN[j];
            }
        }
        return result;
    }();

    // static constexpr auto SORTED_DURATIONS = []() {
    //     std::array<int, DURATIONS.size() * DURATIONS[0].size()> flattened_durations{};

    //     auto flattened_view = std::ranges::views::join(DURATIONS);
    //     std::ranges::copy(flattened_view, flattened.begin());

    //     std::ranges::sort(flattened_durations);

    //     return flattened_durations;
    // }();

} // namespace

namespace test::system::FunctionGenerator::Helpers
{
    void PrintFunctionGeneratorStates(saftlib::MasterFunctionGenerator_Proxy &fgProxy, std::vector<bool> &refillRequested)
    {
        auto isArmed = fgProxy.ReadArmed();
        auto isEnabled = fgProxy.ReadEnabled();
        auto isRunning = fgProxy.ReadRunning();
        auto numFunctionGenerators = isArmed.size();
        auto fillLevels = fgProxy.ReadFillLevels();

        std::cout << "Function generator states:\n";
        std::cout << "\tFG #\t|\tArmed\t|\tEnabled\t|\tRunning\t|\tRefill Requested\t|\tFill Level\n";
        std::cout << "\t-----------------------------------------------------------------------------------------------------------------------------------------------------------\n";
        for (size_t i = 0; i < numFunctionGenerators; ++i)
        {
            std::cout
                << "\t " << i
                << "\t|\t" << (isArmed[i] ? " true " : " false")
                << "\t|\t" << (isEnabled[i] ? " true " : " false")
                << "\t|\t" << (isRunning[i] ? " true " : " false")
                << "\t|\t\t" << (refillRequested[i] ? " true " : " false")
                << "\t\t|\t" << fillLevels[i] / 1e6 << " ms\n";
        }
    }

    bool IsInStartState(saftlib::MasterFunctionGenerator_Proxy &fgProxy)
    {
        auto isArmed = fgProxy.ReadArmed();
        auto isEnabled = fgProxy.ReadEnabled();
        auto isRunning = fgProxy.ReadRunning();

        auto anyArmed = std::any_of(isArmed.begin(), isArmed.end(), [](int32_t armed)
                                    { return armed != 0; });
        auto anyEnabled = std::any_of(isEnabled.begin(), isEnabled.end(), [](int32_t enabled)
                                      { return enabled != 0; });
        auto anyRunning = std::any_of(isRunning.begin(), isRunning.end(), [](int32_t running)
                                      { return running != 0; });

        return !anyArmed && !anyEnabled && !anyRunning;
    }

    bool AllArmed(saftlib::MasterFunctionGenerator_Proxy &fgProxy)
    {
        auto isArmed = fgProxy.ReadArmed();
        return std::all_of(isArmed.begin(), isArmed.end(), [](int32_t armed)
                           { return armed != 0; });
    }

    bool AllUnarmed(saftlib::MasterFunctionGenerator_Proxy &fgProxy)
    {
        auto isArmed = fgProxy.ReadArmed();
        return std::all_of(isArmed.begin(), isArmed.end(), [](int32_t armed)
                           { return armed == 0; });
    }

    void StopFunctionGenerator(saftlib::MasterFunctionGenerator_Proxy &fgProxy)
    {
        auto isRunning = !IsInStartState(fgProxy);
        if (isRunning)
        {
            fgProxy.Abort();
        }

        while (isRunning)
        {
            // @assumption: Signal will be processed by this thread so no contention on isStopped
            saftlib::wait_for_signal(10);
            isRunning = !IsInStartState(fgProxy);
        }
    }

    void BringFunctionGeneratorIntoKnownState(saftlib::MasterFunctionGenerator_Proxy &fgProxy)
    {
        StopFunctionGenerator(fgProxy);
        std::cout << "Master Function generator is now in known state: UNARMED" << "\n";
    }

    std::variant<SaftlibComponents, SaftlibInitializationError>
    CreateSaftlibComponents()
    {
        auto saftd = saftlib::SAFTd_Proxy::create();

        if (!saftd)
        {
            return SaftlibInitializationError{"Failed to create SAFTd proxy"};
        }

        auto devices = saftd->getDevices();

        if (0 == devices.size())
        {
            return SaftlibInitializationError{"No devices found"};
        }

        if (1 < devices.size())
        {
            std::cerr << "Multiple devices found, picking the first one: " << devices.begin()->second << "\n";
        }

        auto receiver = saftlib::TimingReceiver_Proxy::create(devices.begin()->second);

        if (!receiver)
        {
            return SaftlibInitializationError{"Failed to create TimingReceiver proxy"};
        }

        auto fg_names = receiver->getInterfaces()["MasterFunctionGenerator"];
        auto scu_names = receiver->getInterfaces()["SCUbusActionSink"];

        if (fg_names.empty())
        {
            return SaftlibInitializationError{"No Master Function Generator found on device " + receiver->getName()};
        }

        if (scu_names.empty())
        {
            return SaftlibInitializationError{"No SCUbusActionSink found on device " + receiver->getName()};
        }

        auto scuActionSink = saftlib::SCUbusActionSink_Proxy::create(scu_names.begin()->second);
        if (!scuActionSink)
        {
            return SaftlibInitializationError{"Failed to create SCUbusActionSink proxy"};
        }

        auto masterFunctionGeneratorProxy = saftlib::MasterFunctionGenerator_Proxy::create(fg_names.begin()->second);
        if (!masterFunctionGeneratorProxy)
        {
            return SaftlibInitializationError{"Failed to create MasterFunctionGenerator proxy"};
        }
        else
        {
            std::cout << "Successfully created MasterFunctionGenerator proxy for " << fg_names.begin()->first << "\n";
        }

        return SaftlibComponents{
            .saftd = std::move(saftd),
            .receiver = std::move(receiver),
            .masterFunctionGeneratorProxy = std::move(masterFunctionGeneratorProxy),
            .scuProxy = std::move(scuActionSink)};
    }

    ParameterSet GenerateRandomWaveformParameters(size_t numberOfParameters)
    {
        ParameterSet params;
        params.coeff_a.resize(numberOfParameters);
        params.coeff_b.resize(numberOfParameters);
        params.coeff_c.resize(numberOfParameters);
        params.step.resize(numberOfParameters);
        params.freq.resize(numberOfParameters);
        params.shift_a.resize(numberOfParameters);
        params.shift_b.resize(numberOfParameters);
        // Generate random waveform parameters
        for (size_t i = 0; i < numberOfParameters; ++i)
        {
            params.coeff_a[i] = 0;
            params.coeff_b[i] = 100;
            params.coeff_c[i] = 4000000000;
            params.step[i] = 2 + static_cast<unsigned char>(distribution(generator) * 5);
            params.freq[i] = 6;     // 0-7
            params.shift_a[i] = 0;  // 0-63
            params.shift_b[i] = 48; // 0-63
        }
        return params;
    }

    std::vector<ParameterTuple> GenerateRandomTupleSet(size_t minLength, size_t maxLength)
    {
        auto length = minLength + static_cast<size_t>(distribution(generator) * (maxLength - minLength));
        std::vector<ParameterTuple> result;
        result.reserve(length);

        for (size_t i = 0; i < length; ++i)
        {
            result.push_back(GenerateRandomTuple());
        }
        return result;
    }

    ParameterTuple GenerateRandomTuple()
    {
        ParameterTuple data;
        data.coeff_a = 0;
        data.coeff_b = 100;
        data.coeff_c = 4000000000;
        data.step = 2 + static_cast<unsigned char>(distribution(generator) * 5);
        data.freq = 6;
        data.shift_a = 0;
        data.shift_b = 48;
        return data;
    }

    static ParameterTuple GenerateRandomDurationTuple(uint64_t max_duration_ns)
    {
        uint8_t step = 3 + static_cast<unsigned char>(distribution(generator) * 4);
        uint8_t freq = 0 + static_cast<unsigned char>(distribution(generator) * 7);
        ParameterTuple data;

        uint64_t current_duration = DURATIONS[step][freq];

        while (current_duration > max_duration_ns)
        {
            freq -=  freq + static_cast<unsigned char>(distribution(generator) * (7-freq));
            current_duration = DURATIONS[step][freq];
        }

        data.coeff_a = 0;
        data.coeff_b = 100;
        data.coeff_c = 4000000000;
        data.step = step;
        data.freq = freq;
        data.shift_a = 0;
        data.shift_b = 48;

        return data;
    }

    // std::chrono::nanoseconds defaults to int64 with max
    // 9.223.372.036.854.775.807 ns = 166 days 3 hours 42 minutes 31.14 seconds
    std::vector<ParameterTuple> GenerateDurationTupleSet(uint64_t whole_duration_ns)
    {

        std::vector<ParameterTuple> result;

        auto time_left = whole_duration_ns;
        while(time_left > 0)
        {
            std::cout << "Time left: " << time_left << std::endl;
            // mising execption catching eg timout handling
            result.push_back(GenerateRandomDurationTuple(time_left));
            time_left -= DURATIONS[result.back().step][result.back().freq];
        }

        return result;
    }

    void FillFunctionGeneratorBuffer(saftlib::MasterFunctionGenerator_Proxy &fgProxy, ParameterSets &parameterSets)
    {
        fillLow = fgProxy.AppendParameterSets(parameterSets.coeff_a,
                                              parameterSets.coeff_b,
                                              parameterSets.coeff_c,
                                              parameterSets.step,
                                              parameterSets.freq,
                                              parameterSets.shift_a,
                                              parameterSets.shift_b);
    }

    void FillSingleFunctionGeneratorBuffer(saftlib::MasterFunctionGenerator_Proxy &fgProxy,
                                           size_t functionGeneratorIndex,
                                           ParameterSets &parameterSets)
    {
        std::vector<std::vector<int16_t>> coeff_a(parameterSets.size());
        std::vector<std::vector<int16_t>> coeff_b(parameterSets.size());
        std::vector<std::vector<int32_t>> coeff_c(parameterSets.size());
        std::vector<std::vector<unsigned char>> step(parameterSets.size());
        std::vector<std::vector<unsigned char>> freq(parameterSets.size());
        std::vector<std::vector<unsigned char>> shift_a(parameterSets.size());
        std::vector<std::vector<unsigned char>> shift_b(parameterSets.size());

        // Forgive me for I have sinned
        coeff_a[functionGeneratorIndex] = parameterSets.coeff_a[functionGeneratorIndex];
        coeff_b[functionGeneratorIndex] = parameterSets.coeff_b[functionGeneratorIndex];
        coeff_c[functionGeneratorIndex] = parameterSets.coeff_c[functionGeneratorIndex];
        step[functionGeneratorIndex] = parameterSets.step[functionGeneratorIndex];
        freq[functionGeneratorIndex] = parameterSets.freq[functionGeneratorIndex];
        shift_a[functionGeneratorIndex] = parameterSets.shift_a[functionGeneratorIndex];
        shift_b[functionGeneratorIndex] = parameterSets.shift_b[functionGeneratorIndex];

        fgProxy.AppendParameterSets(coeff_a,
                                    coeff_b,
                                    coeff_c,
                                    step,
                                    freq,
                                    shift_a,
                                    shift_b);
    }

    void HandleFillRequest(saftlib::MasterFunctionGenerator_Proxy &fgProxy,
                           std::vector<bool> &refillRequested,
                           ParameterSets &parameterSets)
    {
        auto itFirstFill = std::find_if(refillRequested.begin(), refillRequested.end(), [](bool v)
                                        { return v; });
        auto fillIndex = std::distance(refillRequested.begin(), itFirstFill);
        if (itFirstFill != refillRequested.end())
        {
            FillSingleFunctionGeneratorBuffer(fgProxy,
                                              fillIndex,
                                              parameterSets);
            refillRequested[fillIndex] = false;
        }
    }
}