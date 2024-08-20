#pragma once
#include <cstddef>
#include <cstring>

namespace Cvp
{
    extern const void* model;
    extern const size_t model_size;

    static auto constexpr modelSampleRate = 16000;
    static auto constexpr modelBlockSize = 1024;
    static auto constexpr modelHopeSize = 160;
    static auto constexpr modelActivationSize = 360;

} // namespace Cvp
