#pragma once

#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <map>

#include <EmbeddedCPUCondition_Proxy.hpp>
#include <FunctionGeneratorFirmware_Proxy.hpp>

#include "ParameterSet.hpp"

namespace saftlib
{
    class SAFTd_Proxy;
    class TimingReceiver_Proxy;
    class MasterFunctionGenerator_Proxy;
    class SCUbusActionSink_Proxy;
}

namespace test::system::FunctionGenerator::Helpers
{
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
        std::shared_ptr<saftlib::MasterFunctionGenerator_Proxy> masterFunctionGeneratorProxy;
        std::shared_ptr<saftlib::SCUbusActionSink_Proxy> scuProxy;
        std::shared_ptr<saftlib::FunctionGeneratorFirmware_Proxy> fgFirmwareProxy;
    };

    void PrintFunctionGeneratorStates(saftlib::MasterFunctionGenerator_Proxy &fgProxy, std::vector<bool> &refillRequested);
    bool IsInStartState(saftlib::MasterFunctionGenerator_Proxy &fgProxy);

    bool AllArmed(saftlib::MasterFunctionGenerator_Proxy &fgProxy);

    bool AllUnarmed(saftlib::MasterFunctionGenerator_Proxy &fgProxy);

    void StopFunctionGenerator(saftlib::MasterFunctionGenerator_Proxy &fgProxy);

    void BringFunctionGeneratorIntoKnownState(saftlib::MasterFunctionGenerator_Proxy &fgProxy);

    enum class FunctionGeneratorScanMode
    {
        SkipScan,
        ScanSeparateFunctionGenerators,
        ScanMasterFunctionGenerator
    };

    std::variant<SaftlibComponents, SaftlibInitializationError>
    CreateSaftlibComponents(FunctionGeneratorScanMode scanMode);

    std::variant<SaftlibComponents, SaftlibInitializationError>
    CreateSaftlibComponents();

    ParameterSet GenerateRandomWaveformParameters(size_t numberOfParameters);
    ParameterTuple GenerateRandomTuple();
    std::vector<ParameterTuple> GenerateRandomTupleSet(size_t minLength, size_t maxLength);

    void FillFunctionGeneratorBuffer(saftlib::MasterFunctionGenerator_Proxy &fgProxy, ParameterSets &parameterSets);

    void FillSingleFunctionGeneratorBuffer(saftlib::MasterFunctionGenerator_Proxy &fgProxy,
                                           size_t functionGeneratorIndex,
                                           ParameterSets &parameterSets);

    void HandleFillRequest(saftlib::MasterFunctionGenerator_Proxy &fgProxy,
                           std::vector<bool> &refillRequested,
                           ParameterSets &parameterSets);

    std::shared_ptr<saftlib::EmbeddedCPUCondition_Proxy> RegisterFireEventCondition(const std::shared_ptr<saftlib::TimingReceiver_Proxy> &receiver);
    void TriggerFireCondition(const std::shared_ptr<saftlib::TimingReceiver_Proxy> &receiver);

    struct StepFreqDuration
    {
        std::chrono::nanoseconds duration;
        uint8_t step;
        uint8_t freq;
    };

    StepFreqDuration GenerateRandomDurationTuple(std::chrono::nanoseconds max_duration_ns);
    std::vector<ParameterTuple> GenerateRandomParameterTuples(std::chrono::nanoseconds whole_duration_ns);
}