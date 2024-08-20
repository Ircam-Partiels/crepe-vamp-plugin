#pragma once
#include <cstddef>
#include <cstring>

namespace Cvp
{
    extern const void* model_tiny;
    extern const size_t model_tiny_size;

    extern const void* model_small;
    extern const size_t model_small_size;

    extern const void* model_medium;
    extern const size_t model_medium_size;

    extern const void* model_large;
    extern const size_t model_large_size;

    extern const void* model_full;
    extern const size_t model_full_size;

    static auto constexpr modelSampleRate = 16000;
    static auto constexpr modelBlockSize = 1024;
    static auto constexpr modelHopeSize = 160;
    static auto constexpr modelActivationSize = 360;

} // namespace Cvp
