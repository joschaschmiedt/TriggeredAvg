#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <mutex>

namespace TriggeredAverage
{

using SampleNumber = std::int64_t;

enum class RingBufferReadResult : std::int_fast8_t
{
    UnknownError = -1,
    Success = 0,
    NotEnoughNewData = 1,
    DataInRingBufferTooOld = 2,
    InvalidParameters = 3
};

class MultiChannelRingBuffer
{
public:
    MultiChannelRingBuffer() = delete;
    MultiChannelRingBuffer (int numChannels, int bufferSize);
    ~MultiChannelRingBuffer() = default;

    void addData (const juce::AudioBuffer<float>& inputBuffer, SampleNumber firstSampleNumber);
    RingBufferReadResult readAroundSample (SampleNumber centerSample,
                                           int preSamples,
                                           int postSamples,
                                           juce::AudioBuffer<float>& outputBuffer) const;

    SampleNumber getCurrentSampleNumber() const { return m_nextSampleNumber.load(); }
    int getBufferSize() const { return m_bufferSize; }
    std::pair<RingBufferReadResult, std::optional<int>>
        getStartSampleForTriggeredRead (SampleNumber centerSample,
                                        int preSamples,
                                        int postSamples) const;
    void reset();

private:
    juce::AudioBuffer<float> m_buffer;
    juce::Array<SampleNumber> m_sampleNumbers;

    std::atomic<SampleNumber> m_nextSampleNumber = 0;
    std::atomic<int> m_writeIndex = 0;
    std::atomic<int> m_nValidSamplesInBuffer =
        0; // number of valid samples currently stored (<= bufferSize)

    const int m_nChannels;
    int m_bufferSize;

    mutable std::recursive_mutex writeLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiChannelRingBuffer)
    JUCE_DECLARE_NON_MOVEABLE (MultiChannelRingBuffer)
};
} // namespace TriggeredAverage
