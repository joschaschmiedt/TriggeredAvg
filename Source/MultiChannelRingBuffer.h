#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <mutex>

namespace TriggeredAverage
{

using SampleNumber = std::int64_t;

enum class RingBufferReadResult : std::int_fast8_t
{
    Success = 0,
    NotEnoughNewData = 1,
    DataInRingBufferTooOld = 2,
    InvalidParameters = 3
};

class MultiChannelRingBuffer
{
public:
    MultiChannelRingBuffer (int numChannels, int bufferSize);
    ~MultiChannelRingBuffer() = default;
    auto addData (const juce::AudioBuffer<float>& inputBuffer, SampleNumber firstSampleNumber)
        -> void;
    auto readAroundSample (SampleNumber triggerSample,
                            int preSamples,
                            int postSamples,
                            const juce::Array<int>& channelIndices,
                            juce::AudioBuffer<float>& outputBuffer) const -> RingBufferReadResult;

    auto getCurrentSampleNumber() const -> SampleNumber { return m_nextSampleNumber.load(); }
    auto getBufferSize() const -> int { return m_bufferSize; }
    auto getStartSampleForTriggeredRead (SampleNumber centerSample, int preSamples, int postSamples) const
        -> std::pair<RingBufferReadResult, std::optional<int>>;

    void reset();

private:
    juce::AudioBuffer<float> m_buffer;
    juce::Array<SampleNumber> m_sampleNumbers;

    std::atomic<SampleNumber> m_nextSampleNumber = 0;
    std::atomic<int> m_writeIndex = 0;
    std::atomic<int> m_nValidSamplesInBuffer =
        0; // number of valid samples currently stored (<= bufferSize)

    int m_nChannels;
    int m_bufferSize;

    mutable std::recursive_mutex writeLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiChannelRingBuffer)
    JUCE_DECLARE_NON_MOVEABLE (MultiChannelRingBuffer)
};
} // namespace TriggeredAverage
