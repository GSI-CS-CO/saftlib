
    #pragma once

    #include <vector>
    #include <stdint.h>

    struct ParameterTuple
    {
      int16_t coeff_a;
      int16_t coeff_b;
      int32_t coeff_c;
      uint8_t step;
      uint8_t freq;
      uint8_t shift_a;
      uint8_t shift_b;
    
      uint64_t duration() const;
    };
    
    struct ParameterSet
    {
        std::vector<int16_t> coeff_a;
        std::vector<int16_t> coeff_b;
        std::vector<int32_t> coeff_c;
        std::vector<unsigned char> step;
        std::vector<unsigned char> freq;
        std::vector<unsigned char> shift_a;
        std::vector<unsigned char> shift_b;
    };

    inline std::vector<ParameterTuple> ConvertParameterSetToTuples(const ParameterSet &parameterSet)
    {
        std::vector<ParameterTuple> result;
        const size_t numberOfTuples = parameterSet.coeff_a.size();
        result.resize(numberOfTuples);
        for (size_t i = 0; i < numberOfTuples; ++i)
        {
            result[i].coeff_a = parameterSet.coeff_a[i];
            result[i].coeff_b = parameterSet.coeff_b[i];
            result[i].coeff_c = parameterSet.coeff_c[i];
            result[i].step = parameterSet.step[i];
            result[i].freq = parameterSet.freq[i];
            result[i].shift_a = parameterSet.shift_a[i];
            result[i].shift_b = parameterSet.shift_b[i];
        }
        return result;
    }

    struct ParameterSetReference
    {
        std::vector<int16_t> &coeff_a;
        std::vector<int16_t> &coeff_b;
        std::vector<int32_t> &coeff_c;
        std::vector<unsigned char> &step;
        std::vector<unsigned char> &freq;
        std::vector<unsigned char> &shift_a;
        std::vector<unsigned char> &shift_b;
    };

    struct ParameterSets
    {
        ParameterSets(size_t numberOfParameterSets)
        {
            coeff_a.resize(numberOfParameterSets);
            coeff_b.resize(numberOfParameterSets);
            coeff_c.resize(numberOfParameterSets);
            step.resize(numberOfParameterSets);
            freq.resize(numberOfParameterSets);
            shift_a.resize(numberOfParameterSets);
            shift_b.resize(numberOfParameterSets);
        }

        size_t size() const
        {
            return coeff_a.size();
        }

        ParameterSetReference operator[](size_t index)
        {
            return ParameterSetReference{
                .coeff_a = coeff_a[index],
                .coeff_b = coeff_b[index],
                .coeff_c = coeff_c[index],
                .step = step[index],
                .freq = freq[index],
                .shift_a = shift_a[index],
                .shift_b = shift_b[index]};
        }

        std::vector<std::vector<int16_t>> coeff_a;
        std::vector<std::vector<int16_t>> coeff_b;
        std::vector<std::vector<int32_t>> coeff_c;
        std::vector<std::vector<unsigned char>> step;
        std::vector<std::vector<unsigned char>> freq;
        std::vector<std::vector<unsigned char>> shift_a;
        std::vector<std::vector<unsigned char>> shift_b;
    };