#pragma once
#include <JuceHeader.h>

namespace TriggeredAverage
{

class MultiChannelRingBuffer
{
public:
    MultiChannelRingBuffer (int numChannels, int bufferSize);
    ~MultiChannelRingBuffer() = default;
    void addData (const AudioBuffer<float>& inputBuffer, int64 firstSampleNumber);
    bool readTriggeredData (int64 triggerSample,
                            int preSamples,
                            int postSamples,
                            Array<int> channelIndices,
                            AudioBuffer<float>& outputBuffer) const;

    int64 getCurrentSampleNumber() const { return currentSampleNumber.load(); }

    bool hasEnoughDataForRead (int64 triggerSample, int preSamples, int postSamples) const;

    void reset();

private:
    AudioBuffer<float> buffer;
    Array<int64> sampleNumbers;

    std::atomic<int64> currentSampleNumber;
    std::atomic<int> writeIndex;
    std::atomic<int> validSize; // number of valid samples currently stored (<= bufferSize)

    int numChannels;
    int bufferSize;

    CriticalSection writeLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiChannelRingBuffer)
    JUCE_DECLARE_NON_MOVEABLE (MultiChannelRingBuffer)
};
} // namespace TriggeredAverage
