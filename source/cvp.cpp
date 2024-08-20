#include "cvp.h"
#include "cvp_model.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <vamp-sdk/PluginAdapter.h>

#if defined(_MSC_VER)
#define forcedinline __forceinline
#else
#define forcedinline inline __attribute__((always_inline))
#endif

#ifndef NDEBUG
#define CvpDbg(message) std::cout << "Cvp: " << message << "\n"
#define CvpErr(message) std::cerr << "Cvp: " << message << "\n";

[[maybe_unused]] static void print(TfLiteInterpreter* interpreter)
{
    CvpDbg("Prepare");
    auto const printTensor = [&](TfLiteTensor const* tensor, int index)
    {
        auto const numDims = TfLiteTensorNumDims(tensor);
        CvpDbg("  Index " << index << " " << TfLiteTensorName(tensor) << " Num Dims " << numDims << " Type " << TfLiteTensorType(tensor));
        for(int32_t dimIndex = 0; dimIndex < numDims; ++dimIndex)
        {
            auto const dimLength = TfLiteTensorDim(tensor, dimIndex);
            CvpDbg("      dim" << dimIndex << " Size " << dimLength);
        }
    };

    auto const numInputs = TfLiteInterpreterGetInputTensorCount(interpreter);
    CvpDbg("Num Inputs " << numInputs);
    for(auto inputIndex = 0; inputIndex < numInputs; ++inputIndex)
    {
        auto const* input = TfLiteInterpreterGetInputTensor(interpreter, inputIndex);
        printTensor(input, inputIndex);
    }

    auto const numOutputs = TfLiteInterpreterGetOutputTensorCount(interpreter);
    CvpDbg("Num Outputs " << numOutputs);
    for(auto outputIndex = 0; outputIndex < numOutputs; ++outputIndex)
    {
        auto const* output = TfLiteInterpreterGetOutputTensor(interpreter, outputIndex);
        printTensor(output, outputIndex);
    }
}

#else
#define CvpDbg(message)
#define CvpErr(message)

[[maybe_unused]] static void print([[maybe_unused]] TfLiteInterpreter* interpreter)
{
}

#endif

namespace ResamplerUtils
{
    template <int k>
    struct LagrangeResampleHelper
    {
        static forcedinline void calc(float& a, float b) noexcept { a *= b * (1.0f / k); }
    };

    template <>
    struct LagrangeResampleHelper<0>
    {
        static forcedinline void calc(float&, float) noexcept {}
    };

    template <int k>
    static float calcCoefficient(float input, float offset) noexcept
    {
        LagrangeResampleHelper<0 - k>::calc(input, -2.0f - offset);
        LagrangeResampleHelper<1 - k>::calc(input, -1.0f - offset);
        LagrangeResampleHelper<2 - k>::calc(input, 0.0f - offset);
        LagrangeResampleHelper<3 - k>::calc(input, 1.0f - offset);
        LagrangeResampleHelper<4 - k>::calc(input, 2.0f - offset);
        return input;
    }

    static float valueAtOffset(const float* inputs, float offset, int index) noexcept
    {
        auto result = 0.0f;
        result += calcCoefficient<0>(inputs[index], offset);
        index = (++index % 5);
        result += calcCoefficient<1>(inputs[index], offset);
        index = (++index % 5);
        result += calcCoefficient<2>(inputs[index], offset);
        index = (++index % 5);
        result += calcCoefficient<3>(inputs[index], offset);
        index = (++index % 5);
        result += calcCoefficient<4>(inputs[index], offset);
        return result;
    }
} // namespace ResamplerUtils

void Cvp::Plugin::Resampler::prepare(double sampleRate)
{
    mSourceSampleRate = sampleRate;
    reset();
}

std::tuple<size_t, size_t> Cvp::Plugin::Resampler::process(size_t numInputSamples, float const* inputBuffer, size_t numOutputSamples, float* outputBuffer)
{
    double const speedRatio = getRatio();
    size_t numGeneratedSamples = 0;
    size_t numUsedSamples = 0;
    auto subSamplePos = mSubSamplePos;
    while(numUsedSamples < numInputSamples && numGeneratedSamples < numOutputSamples)
    {
        while(subSamplePos >= 1.0 && numUsedSamples < numInputSamples)
        {
            mLastInputSamples[mIndexBuffer] = inputBuffer[numUsedSamples++];
            if(++mIndexBuffer == mLastInputSamples.size())
            {
                mIndexBuffer = 0;
            }
            subSamplePos -= 1.0;
        }
        if(subSamplePos < 1.0)
        {
            outputBuffer[numGeneratedSamples++] = ResamplerUtils::valueAtOffset(mLastInputSamples.data(), static_cast<float>(subSamplePos), static_cast<int>(mIndexBuffer));
            subSamplePos += speedRatio;
        }
    }
    while(subSamplePos >= 1.0 && numUsedSamples < numInputSamples)
    {
        mLastInputSamples[mIndexBuffer] = inputBuffer[numUsedSamples++];
        if(++mIndexBuffer == mLastInputSamples.size())
        {
            mIndexBuffer = 0;
        }
        subSamplePos -= 1.0;
    }
    mSubSamplePos = subSamplePos;
    return std::make_tuple(numUsedSamples, numGeneratedSamples);
}

void Cvp::Plugin::Resampler::reset()
{
    mIndexBuffer = 0;
    mSubSamplePos = 1.0;
    std::fill(mLastInputSamples.begin(), mLastInputSamples.end(), 0.0f);
}

void Cvp::Plugin::Resampler::setTargetSampleRate(double sampleRate) noexcept
{
    mTargetSampleRate = sampleRate;
}

double Cvp::Plugin::Resampler::getRatio() const noexcept
{
    return mSourceSampleRate / mTargetSampleRate;
}

Cvp::Plugin::Plugin(float inputSampleRate)
: Vamp::Plugin(inputSampleRate)
, mModel(model_uptr(TfLiteModelCreate(Cvp::model, Cvp::model_size), [](TfLiteModel* m)
                    {
                        if(m != nullptr)
                        {
                            TfLiteModelDelete(m);
                        }
                    }))
{
    if(mModel == nullptr)
    {
        CvpErr("TfLite failed to allocate model!");
    }
    mResampler.prepare(static_cast<double>(inputSampleRate));
}

bool Cvp::Plugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if(channels != static_cast<size_t>(1) || stepSize != blockSize)
    {
        return false;
    }
    reset();
    mBlockSize = blockSize;
    return mInterpreter != nullptr;
}

std::string Cvp::Plugin::getIdentifier() const
{
    return "ircamcrepe";
}

std::string Cvp::Plugin::getName() const
{
    return "Crepe";
}

std::string Cvp::Plugin::getDescription() const
{
    return "Monophonic pitch tracker Crepe model.";
}

std::string Cvp::Plugin::getMaker() const
{
    return "Ircam";
}

int Cvp::Plugin::getPluginVersion() const
{
    return CVP_PLUGIN_VERSION;
}

std::string Cvp::Plugin::getCopyright() const
{
    return "Crepe model by Jong Wook Kim, Justin Salamon, Peter Li & Juan Pablo Bello. Crepe Vamp Plugin by Pierre Guillot. Copyright 2024 Ircam. All rights reserved.";
}

Cvp::Plugin::InputDomain Cvp::Plugin::getInputDomain() const
{
    return TimeDomain;
}

size_t Cvp::Plugin::getPreferredBlockSize() const
{
    return static_cast<size_t>(1024);
}

size_t Cvp::Plugin::getPreferredStepSize() const
{
    return static_cast<size_t>(0);
}

Cvp::Plugin::OutputList Cvp::Plugin::getOutputDescriptors() const
{
    OutputDescriptor d;
    d.identifier = "pitch";
    d.name = "Pitch";
    d.description = "Pitch estimated from the input signal";
    d.unit = "Hertz";
    d.hasFixedBinCount = true;
    d.binCount = static_cast<size_t>(1);
    d.hasKnownExtents = true;
    d.minValue = 0.0f;
    d.maxValue = getInputSampleRate() / 2.0f;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::SampleType::VariableSampleRate;
    d.hasDuration = false;
    return {d};
}

void Cvp::Plugin::reset()
{
    mInterpreter.reset();
    auto options = interpreter_options_uptr(TfLiteInterpreterOptionsCreate(), [](TfLiteInterpreterOptions* o)
                                            {
                                                if(o != nullptr)
                                                {
                                                    TfLiteInterpreterOptionsDelete(o);
                                                }
                                            });
    if(options == nullptr)
    {
        CvpErr("TfLite failed to allocate option!");
    }
    else
    {
        mInterpreter = interpreter_uptr(TfLiteInterpreterCreate(mModel.get(), options.get()), [](TfLiteInterpreter* i)
                                        {
                                            if(i != nullptr)
                                            {
                                                TfLiteInterpreterDelete(i);
                                            }
                                        });
        if(mInterpreter == nullptr)
        {
            CvpErr("TfLite failed to allocate interpreter!");
        }
        else
        {
            auto const result = TfLiteInterpreterAllocateTensors(mInterpreter.get());
            if(result != TfLiteStatus::kTfLiteOk)
            {
                CvpErr("TfLite failed to allocate tensors!");
                mInterpreter.reset();
            }
            else
            {
                print(mInterpreter.get());
            }
        }
    }
    std::fill(mInputBuffer.begin(), mInputBuffer.end(), 0.0f);
    mInputBufferPosition = modelHopeSize;
    mResampler.reset();
}

Cvp::Plugin::ParameterList Cvp::Plugin::getParameterDescriptors() const
{
    return {};
}

void Cvp::Plugin::setParameter(std::string paramid, float newval)
{
    std::cerr << "Invalid parameter : " << paramid << "\n";
}

float Cvp::Plugin::getParameter(std::string paramid) const
{
    std::cerr << "Invalid parameter : " << paramid << "\n";
    return 0.0f;
}

Cvp::Plugin::OutputExtraList Cvp::Plugin::getOutputExtraDescriptors(size_t outputDescriptorIndex) const
{
    OutputExtraList list;
    if(outputDescriptorIndex == 0)
    {
        OutputExtraDescriptor d;
        d.identifier = "confidence";
        d.name = "Confidence";
        d.description = "The confidence of the result";
        d.unit = "";
        d.hasKnownExtents = true;
        d.minValue = 0.0f;
        d.maxValue = 1.0f;
        d.isQuantized = false;
        d.quantizeStep = 0.0f;
        list.push_back(std::move(d));
    }
    return list;
}

Cvp::Plugin::FeatureList Cvp::Plugin::processModel()
{
    FeatureList fl;
    while(mInputBufferPosition >= modelBlockSize)
    {
        std::copy(mInputBuffer.cbegin(), std::next(mInputBuffer.cbegin(), modelBlockSize), mAudioBuffer.begin());
        auto const mean = std::accumulate(mAudioBuffer.cbegin(), mAudioBuffer.cend(), 0.0f) / static_cast<float>(mAudioBuffer.size());
        auto const variance = std::accumulate(mAudioBuffer.begin(), mAudioBuffer.end(), 0.0f, [mean](auto acc, auto x)
                                              {
                                                  return acc + (x - mean) * (x - mean);
                                              }) /
                              static_cast<float>(mAudioBuffer.size());
        auto const stddev = std::max(std::sqrt(variance), 1e-8f);
        std::transform(mAudioBuffer.cbegin(), mAudioBuffer.cend(), mAudioBuffer.begin(), [mean, stddev](float x)
                       {
                           return (x - mean) / stddev;
                       });

        TfLiteTensorCopyFromBuffer(TfLiteInterpreterGetInputTensor(mInterpreter.get(), 0), mAudioBuffer.data(), mAudioBuffer.size() * sizeof(float));
        TfLiteInterpreterInvoke(mInterpreter.get());
        for(size_t i = modelHopeSize; i < mInputBufferPosition; ++i)
        {
            mInputBuffer[i - modelHopeSize] = mInputBuffer.at(i);
        }
        mInputBufferPosition -= modelHopeSize;

        TfLiteTensorCopyToBuffer(TfLiteInterpreterGetOutputTensor(mInterpreter.get(), 0), mActivationBuffer.data(), mActivationBuffer.size() * sizeof(float));
        static auto constexpr hopeDuration = static_cast<double>(modelHopeSize) / static_cast<double>(modelSampleRate);

        auto const max = std::max_element(mActivationBuffer.cbegin(), mActivationBuffer.cend());
        auto const confidence = *max;
        auto const center = static_cast<long>(std::distance(mActivationBuffer.cbegin(), max));
        auto const start = std::max(0l, center - 4l);
        auto const end = std::min(static_cast<long>(modelActivationSize), center + 5l);
        double productSum = 0.0;
        double weightSum = 0.0;
        for(auto i = start; i < end; ++i)
        {
            auto const activation = static_cast<double>(mActivationBuffer.at(i));
            productSum += activation * (static_cast<float>(i) / 359.0 * 7180.0 + 1997.3794084376191);
            weightSum += activation;
        }
        auto const cents = std::abs(weightSum) > 0.0 ? productSum / weightSum : 0.0;
        auto const frequency = 10.0 * std::pow(2.0, cents / 1200.0);

        Feature feature;
        feature.hasTimestamp = true;
        feature.timestamp = Vamp::RealTime::fromSeconds(static_cast<double>(mActivationFrame) * hopeDuration);
        feature.hasDuration = false;
        feature.duration = Vamp::RealTime::fromSeconds(hopeDuration);
        feature.values = {static_cast<float>(frequency), confidence};
        fl.push_back(std::move(feature));

        ++mActivationFrame;
    }
    return fl;
}

Cvp::Plugin::FeatureSet Cvp::Plugin::process(float const* const* inputBuffers, [[maybe_unused]] Vamp::RealTime timestamp)
{
    FeatureList fl;
    auto const* inputBuffer = inputBuffers[0];
    size_t inputPosition = 0;
    auto remainingSamples = mBlockSize;
    while(remainingSamples > 0)
    {
        auto const remainingOutput = mInputBuffer.size() - mInputBufferPosition;
        auto const result = mResampler.process(remainingSamples, inputBuffer + inputPosition, remainingOutput, mInputBuffer.data() + mInputBufferPosition);
        mInputBufferPosition += std::get<1>(result);
        auto const cfl = processModel();
        fl.insert(fl.end(), cfl.cbegin(), cfl.cend());
        inputPosition += std::get<0>(result);
        remainingSamples -= std::get<0>(result);
    }
    return {{0, fl}};
}

Cvp::Plugin::FeatureSet Cvp::Plugin::getRemainingFeatures()
{
    std::fill(std::next(mInputBuffer.begin(), mInputBufferPosition), mInputBuffer.end(), 0.0f);
    mInputBufferPosition = std::min(mInputBufferPosition + modelHopeSize, mInputBuffer.size());
    return {{0, processModel()}};
}

#ifdef __cplusplus
extern "C"
{
#endif
    VampPluginDescriptor const* vampGetPluginDescriptor(unsigned int version, unsigned int index)
    {
        if(version < 1)
        {
            return nullptr;
        }
        switch(index)
        {
            case 0:
            {
                static Vamp::PluginAdapter<Cvp::Plugin> adaptater;
                return adaptater.getDescriptor();
            }
            default:
            {
                return nullptr;
            }
        }
    }

    IVE_EXTERN IvePluginDescriptor const* iveGetPluginDescriptor(unsigned int version, unsigned int index)
    {
        if(version < 1)
        {
            return nullptr;
        }
        switch(index)
        {
            case 0:
            {
                return Ive::PluginAdapter::getDescriptor<Cvp::Plugin>();
            }
            default:
            {
                return nullptr;
            }
        }
    }
#ifdef __cplusplus
}
#endif
