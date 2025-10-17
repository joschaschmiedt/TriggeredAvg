#pragma once
#include "MultiChannelRingBuffer.h"

#include <JuceHeader.h>
#include <ProcessorHeaders.h>

namespace TriggeredAverage
{
class AverageBuffer;
class TriggeredAvgNode;
class TriggerSource;
class MultiChannelRingBuffer;

struct CaptureRequest
{
    TriggerSource* triggerSource;
    SampleNumber triggerSample;
    int preSamples;
    int postSamples;
};

class DataCollector : public Thread
{
public:
    DataCollector (TriggeredAvgNode* viewer, MultiChannelRingBuffer* buffer);
    ~DataCollector() override;
    void run() override;
    void registerCaptureRequest (const CaptureRequest&);

private:
    TriggeredAvgNode* viewer;
    MultiChannelRingBuffer* ringBuffer;

    CriticalSection triggerQueueLock;
    std::deque<CaptureRequest> captureRequestQueue;
    WaitableEvent newTriggerEvent;

    AudioBuffer<float> m_collectBuffer;
    std::unordered_map<TriggerSource*, AverageBuffer> m_averageBuffer;

    RingBufferReadResult processCaptureRequest (const CaptureRequest& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataCollector)
};

class AverageBuffer
{
public:
    AverageBuffer() = delete;
    AverageBuffer (int numChannels, int numSamples);
    AverageBuffer (AverageBuffer&& other) noexcept;
    AverageBuffer& operator= (AverageBuffer&& other) noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AverageBuffer)

    void addBuffer (const juce::AudioBuffer<float>& buffer);
    AudioBuffer<float> getAverage() const;
    AudioBuffer<float> getStandardDeviation() const;

    void reset();
    int getNumTrials() const;
    int getNumChannels() const;
    int getNumSamples() const;

private:
    juce::AudioBuffer<float> m_sumBuffer;
    juce::AudioBuffer<float> m_sumSquaresBuffer;
    int m_numTrials = 0;
    int m_numChannels;
    int m_numSamples;
};

} // namespace TriggeredAverage
