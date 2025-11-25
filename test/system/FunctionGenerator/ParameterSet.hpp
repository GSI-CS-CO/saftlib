
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