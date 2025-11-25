
#include "SAFTd_Proxy.hpp"
#include "FunctionGenerator_Proxy.hpp"
#include "TimingReceiver_Proxy.hpp"
#include "SCUbusActionSink_Proxy.hpp"
#include "CommonFunctions.hpp"

#include <condition_variable>
#include <variant>
#include <optional>
#include <random>

namespace
{

    enum KnownStates
    {
        UNARMED,
        ARMED,
        RUNNING,
        UNFILLED
    };

    struct SendDataError
    {
        std::string message;
    };

    struct SaftlibInitializationError
    {
        std::string message;
    };

    struct SaftlibComponents
    {
        std::shared_ptr<saftlib::SAFTd_Proxy> saftd;
        std::shared_ptr<saftlib::TimingReceiver_Proxy> receiver;
        std::shared_ptr<saftlib::FunctionGenerator_Proxy> functionGeneratorProxy;
        std::shared_ptr<saftlib::SCUbusActionSink_Proxy> scuProxy;
    };

    /// Indicate if the function generator has filled enough data so it would signal the refill signal when running
    bool fillLow = false;
    bool refillRequested = false;

    std::uniform_real_distribution<double> distribution(0.0, 1.0);
    std::default_random_engine generator;

    void PrintFunctionGeneratorStates(saftlib::FunctionGenerator_Proxy &fgProxy)
    {
        auto isArmed = fgProxy.getArmed();
        auto isEnabled = fgProxy.getEnabled();
        auto isRunning = fgProxy.getRunning();
        std::cout << "Function generator states - "
                  << "Armed: " << (isArmed ? "true" : "false") << ", "
                  << "Enabled: " << (isEnabled ? "true" : "false") << ", "
                  << "Running: " << (isRunning ? "true" : "false")
                  << "\n";
    }

    void StopFunctionGenerator(saftlib::FunctionGenerator_Proxy &fgProxy)
    {
        bool isStopped = !fgProxy.getRunning();
        [[maybe_unused]] auto stoppedConnection = fgProxy.SigRunning.connect([&fgProxy, &isStopped](bool running)
                                                                             {
            std::cout << "Function generator running: " << (running ? "true" : "false") << "\n";
            if(!running)
            {
                std::cout << "Function generator has stopped" << "\n";
            } });

        // during the first call and the connect method the state could already have changed
        // As I do not know when the getRunning will be return false, we may have missed the signal
        // so we check again here
        isStopped = !fgProxy.getRunning();
        if (!isStopped)
        {
            fgProxy.Abort();
        }

        while (!isStopped)
        {
            // @assumption: Signal will be processed by this thread so no contention on isStopped
            saftlib::wait_for_signal(10);
            isStopped = !fgProxy.getRunning();
        }
    }

    void BringFunctionGeneratorIntoKnownState(saftlib::FunctionGenerator_Proxy &fgProxy)
    {
        auto isArmed = fgProxy.getArmed();     // just to print the current state
        auto isEnabled = fgProxy.getEnabled(); // just to print the current state
        auto isRunning = fgProxy.getRunning(); // just to print the current state
        if (isArmed || isEnabled || isRunning)
        {
            StopFunctionGenerator(fgProxy);
        }

        std::cout << "Function generator is now in known state: UNARMED" << "\n";
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

        auto fg_names = receiver->getInterfaces()["FunctionGenerator"];
        auto scu_names = receiver->getInterfaces()["SCUbusActionSink"];

        if (fg_names.empty())
        {
            return SaftlibInitializationError{"No Function Generator found on device " + receiver->getName()};
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

        auto functionGeneratorProxy = saftlib::FunctionGenerator_Proxy::create(fg_names.begin()->second);
        if (!functionGeneratorProxy)
        {
            return SaftlibInitializationError{"Failed to create FunctionGenerator proxy"};
        }
        else
        {
            std::cout << "Successfully created FunctionGenerator proxy for " << fg_names.begin()->first << "\n";
        }

        return SaftlibComponents{
            .saftd = std::move(saftd),
            .receiver = std::move(receiver),
            .functionGeneratorProxy = std::move(functionGeneratorProxy),
            .scuProxy = std::move(scuActionSink)};
    }

    void GenerateRandomWaveformParameters(size_t numberOfParameters,
                                          std::vector<int16_t> &coeff_a,
                                          std::vector<int16_t> &coeff_b,
                                          std::vector<int32_t> &coeff_c,
                                          std::vector<unsigned char> &step,
                                          std::vector<unsigned char> &freq,
                                          std::vector<unsigned char> &shift_a,
                                          std::vector<unsigned char> &shift_b)
    {
        coeff_a.resize(numberOfParameters);
        coeff_b.resize(numberOfParameters);
        coeff_c.resize(numberOfParameters);
        step.resize(numberOfParameters);
        freq.resize(numberOfParameters);
        shift_a.resize(numberOfParameters);
        shift_b.resize(numberOfParameters);
        // Generate random waveform parameters
        for (size_t i = 0; i < coeff_a.size(); ++i)
        {
            coeff_a[i] = 0;
            coeff_b[i] = 100;
            coeff_c[i] = 4000000000;
            step[i] = 2 + static_cast<unsigned char>(distribution(generator) * 5);
            freq[i] = 6;     // 0-7
            shift_a[i] = 0;  // 0-63
            shift_b[i] = 48; // 0-63
        }
    }

    void FillFunctionGeneratorBuffer(saftlib::FunctionGenerator_Proxy &fgProxy,
                                     const std::vector<int16_t> &coeff_a,
                                     const std::vector<int16_t> &coeff_b,
                                     const std::vector<int32_t> &coeff_c,
                                     const std::vector<unsigned char> &step,
                                     const std::vector<unsigned char> &freq,
                                     const std::vector<unsigned char> &shift_a,
                                     const std::vector<unsigned char> &shift_b)
    {
        size_t totalParameters = coeff_a.size();
        size_t parametersSent = 0;
        const size_t maxParametersPerCall = 100;

        while (parametersSent < totalParameters)
        {
            size_t parametersToSend = std::min(maxParametersPerCall, totalParameters - parametersSent);

            fillLow = fgProxy.AppendParameterSet(
                std::vector<int16_t>(coeff_a.begin() + parametersSent, coeff_a.begin() + parametersSent + parametersToSend),
                std::vector<int16_t>(coeff_b.begin() + parametersSent, coeff_b.begin() + parametersSent + parametersToSend),
                std::vector<int32_t>(coeff_c.begin() + parametersSent, coeff_c.begin() + parametersSent + parametersToSend),
                std::vector<unsigned char>(step.begin() + parametersSent, step.begin() + parametersSent + parametersToSend),
                std::vector<unsigned char>(freq.begin() + parametersSent, freq.begin() + parametersSent + parametersToSend),
                std::vector<unsigned char>(shift_a.begin() + parametersSent, shift_a.begin() + parametersSent + parametersToSend),
                std::vector<unsigned char>(shift_b.begin() + parametersSent, shift_b.begin() + parametersSent + parametersToSend));

            parametersSent += parametersToSend;
        }

        std::cout << "Sent " << parametersSent << " parameters to function generator." << "\n";
        std::cout << "Function generator FillLevel: " << fgProxy.ReadFillLevel() << " ns" << "\n";
        std::cout << "Function generator fillLow: " << (fillLow ? "true" : "false") << "\n";
    }
}

// const std::vector< std::vector< int16_t > >& coeff_a,
// const std::vector< std::vector< int16_t > >& coeff_b,
// const std::vector< std::vector< int32_t > >& coeff_c,
// const std::vector< std::vector< unsigned char > >& step,
// const std::vector< std::vector< unsigned char > >& freq,
// const std::vector< std::vector< unsigned char > >& shift_a,
// const std::vector< std::vector< unsigned char > >& shift_b,
// bool arm
// bool wait_for_arm_ack

std::condition_variable refill_variable;
std::mutex refill_mutex;
bool refill_needed = false;

int main(int argc, char **argv)
{
    auto componentsOrError = CreateSaftlibComponents();
    if (std::holds_alternative<SaftlibInitializationError>(componentsOrError))
    {
        auto error = std::get<SaftlibInitializationError>(componentsOrError);
        std::cerr << "Error during saftlib components creation: " << error.message << "\n";
        return 1;
    }

    auto components = std::get<SaftlibComponents>(componentsOrError);

    [[maybe_unused]] sigc::connection refillConnection = components.functionGeneratorProxy->Refill.connect([&]()
                                                                                                           {
                                                            std::cout << "\tFunction generator requests refill." << "\n";
                                                            refillRequested = true; });

    if (refillConnection.connected())
    {
        std::cout << "Successfully connected to FunctionGenerator Refill signal." << "\n";
    }
    else
    {
        std::cerr << "Failed to connect to FunctionGenerator Refill signal." << "\n";
    }

    components.functionGeneratorProxy->Own();

    BringFunctionGeneratorIntoKnownState(*components.functionGeneratorProxy);
    PrintFunctionGeneratorStates(*components.functionGeneratorProxy);

    std::vector<int16_t> coeff_a;
    std::vector<int16_t> coeff_b;
    std::vector<int32_t> coeff_c;
    std::vector<unsigned char> step;
    std::vector<unsigned char> freq;
    std::vector<unsigned char> shift_a;
    std::vector<unsigned char> shift_b;

    GenerateRandomWaveformParameters(1000,
                                     coeff_a,
                                     coeff_b,
                                     coeff_c,
                                     step,
                                     freq,
                                     shift_a,
                                     shift_b);

    std::cout << "Generated random waveform parameters." << "\n";
    std::cout << "Filling function generator buffer." << "\n";

    FillFunctionGeneratorBuffer(*components.functionGeneratorProxy,
                                coeff_a,
                                coeff_b,
                                coeff_c,
                                step,
                                freq,
                                shift_a,
                                shift_b);

    PrintFunctionGeneratorStates(*components.functionGeneratorProxy);
    std::cout << "Function generator buffer filled." << "\n";
    PrintFunctionGeneratorStates(*components.functionGeneratorProxy);

    // Arm and start the function generator
    std::cout << "Arming and starting the function generator." << "\n";
    {
        std::cout << "Setting StartTag to 1337 and arming function generator." << "\n";
        components.functionGeneratorProxy->setStartTag(1337);
        PrintFunctionGeneratorStates(*components.functionGeneratorProxy);

        std::cout << "Arming function generator." << "\n";
        components.functionGeneratorProxy->Arm();
        PrintFunctionGeneratorStates(*components.functionGeneratorProxy);

        while (components.functionGeneratorProxy->getArmed() == false)
        {

            PrintFunctionGeneratorStates(*components.functionGeneratorProxy);
            saftlib::wait_for_signal(10);
        }

        std::cout << "Function generator armed. Injecting tag to start waveform generation." << "\n";
        components.scuProxy->InjectTag(1337);
        PrintFunctionGeneratorStates(*components.functionGeneratorProxy);

        while (components.functionGeneratorProxy->getArmed() != false)
        {
            PrintFunctionGeneratorStates(*components.functionGeneratorProxy);
            saftlib::wait_for_signal(10);
        }
    }

    for (int32_t i = 0; i < 20; ++i)
    {
        if (refillRequested)
        {
            FillFunctionGeneratorBuffer(*components.functionGeneratorProxy,
                                        coeff_a,
                                        coeff_b,
                                        coeff_c,
                                        step,
                                        freq,
                                        shift_a,
                                        shift_b);
            refillRequested = false;
        }
        saftlib::wait_for_signal(1000);
        PrintFunctionGeneratorStates(*components.functionGeneratorProxy);
    }

    StopFunctionGenerator(*components.functionGeneratorProxy);
    BringFunctionGeneratorIntoKnownState(*components.functionGeneratorProxy);
    PrintFunctionGeneratorStates(*components.functionGeneratorProxy);

    FillFunctionGeneratorBuffer(*components.functionGeneratorProxy,
                                coeff_a,
                                coeff_b,
                                coeff_c,
                                step,
                                freq,
                                shift_a,
                                shift_b);

    components.functionGeneratorProxy->setStartTag(1337);
    components.functionGeneratorProxy->Arm();

    while (components.functionGeneratorProxy->getArmed() == false)
    {
        PrintFunctionGeneratorStates(*components.functionGeneratorProxy);
        saftlib::wait_for_signal(10);
    }

    components.scuProxy->InjectTag(1337);

    while(true)
    {
        if (refillRequested)
        {
            FillFunctionGeneratorBuffer(*components.functionGeneratorProxy,
                                        coeff_a,
                                        coeff_b,
                                        coeff_c,
                                        step,
                                        freq,
                                        shift_a,
                                        shift_b);
            refillRequested = false;
        }
        saftlib::wait_for_signal(1000);
        PrintFunctionGeneratorStates(*components.functionGeneratorProxy);
    }

    // Abort the function generator
    StopFunctionGenerator(*components.functionGeneratorProxy);
    BringFunctionGeneratorIntoKnownState(*components.functionGeneratorProxy);

    return 0;
}