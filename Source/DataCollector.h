#pragma once
#include "MultiChannelRingBuffer.h"

#include <JuceHeader.h>
#include <ProcessorHeaders.h>

namespace TriggeredAverage
{
class MultiChannelAverageBuffer;
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

class DataStore
{
public:
    void ResetAndResizeAverageBufferForTriggerSource (TriggerSource* source, int nChannels, int nSamples);

    MultiChannelAverageBuffer* getRefToAverageBufferForTriggerSource (TriggerSource* source)
    {
        if (m_averageBuffers.contains (source))
            return &m_averageBuffers.at (source);
        return nullptr;
    }
    std::scoped_lock<std::recursive_mutex> GetLock()
    {
        return std::scoped_lock<std::recursive_mutex> (m_mutex);
    }

    void Clear()
    {
        auto lock = GetLock();
        m_averageBuffers.clear();
    }
    // TODO: Add method for getteing a ref with a lock

private:
    std::recursive_mutex m_mutex;
    std::unordered_map<TriggerSource*, MultiChannelAverageBuffer> m_averageBuffers;
};

class DataCollector : public Thread
{
public:
    DataCollector (TriggeredAvgNode*, MultiChannelRingBuffer*, DataStore*);
    ~DataCollector() override;
    void run() override;
    void registerTriggerSource (const TriggerSource*);
    void registerCaptureRequest (const CaptureRequest&);

private:
    // dependencies
    TriggeredAvgNode* m_processor;
    MultiChannelRingBuffer* ringBuffer;
    DataStore* m_datastore;

    // data
    std::deque<CaptureRequest> captureRequestQueue;
    AudioBuffer<float> m_collectBuffer;

    // synchronization
    CriticalSection triggerQueueLock;
    WaitableEvent newTriggerEvent;

    RingBufferReadResult processCaptureRequest (const CaptureRequest&);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataCollector)
};

class MultiChannelAverageBuffer
{
public:
    MultiChannelAverageBuffer() = default;
    MultiChannelAverageBuffer (int numChannels, int numSamples);
    MultiChannelAverageBuffer (MultiChannelAverageBuffer&& other) noexcept;
    MultiChannelAverageBuffer& operator= (MultiChannelAverageBuffer&& other) noexcept;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultiChannelAverageBuffer)

    void addDataToAverageFromBuffer (const juce::AudioBuffer<float>& buffer);
    AudioBuffer<float> getAverage() const;
    AudioBuffer<float> getStandardDeviation() const;

    void resetTrials();
    int getNumTrials() const;
    int getNumChannels() const;
    int getNumSamples() const;
    // resets and resizes the buffers
    void setSize (int nChannels, int nSamples)
    {
        m_numChannels = nChannels;
        m_numSamples = nSamples;
        m_sumBuffer.setSize (nChannels, nSamples);
        m_sumSquaresBuffer.setSize (nChannels, nSamples);
        resetTrials();
    }

private:
    juce::AudioBuffer<float> m_sumBuffer;
    juce::AudioBuffer<float> m_sumSquaresBuffer;
    int m_numTrials = 0;
    int m_numChannels;
    int m_numSamples;
};

} // namespace TriggeredAverage
