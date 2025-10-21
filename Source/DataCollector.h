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
    MultiChannelAverageBuffer* getRefToAverageBufferForTriggerSource (TriggerSource* source)
    {
        // TODO:
        // allocate on first time?
        //if (! m_averageBuffers.contains (request.triggerSource))
        //{
        //    m_averageBuffers.emplace (
        //        request.triggerSource,
        //        MultiChannelAverageBuffer (m_collectBuffer.getNumChannels(), m_collectBuffer.getNumSamples()));
        //}

        //// erase and resize if number of channels does not match
        //if (m_averageBuffers[request.triggerSource].getNumChannels()
        //        != m_collectBuffer.getNumChannels()
        //    || m_averageBuffers[request.triggerSource].getNumSamples()
        //           != m_collectBuffer.getNumSamples())
        //{
        //    m_averageBuffers.erase (request.triggerSource);
        //    m_averageBuffers.emplace (
        //        request.triggerSource,
        //        MultiChannelAverageBuffer (m_collectBuffer.getNumChannels(), m_collectBuffer.getNumSamples()));
        //}

        if (m_averageBuffers.contains (source))
            return &m_averageBuffers.at (source);
        return nullptr;
    }
    std::scoped_lock<std::recursive_mutex> GetLock()
    {
        return std::scoped_lock<std::recursive_mutex> (m_mutex);
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
    //void resetAverageBuffers()
    //{
    //    const ScopedLock lock (triggerQueueLock);
    //    //m_averageBuffers.clear();
    //}

private:
    TriggeredAvgNode* m_processor;
    MultiChannelRingBuffer* ringBuffer;

    CriticalSection triggerQueueLock;
    std::deque<CaptureRequest> captureRequestQueue;
    WaitableEvent newTriggerEvent;

    AudioBuffer<float> m_collectBuffer;
    //std::unordered_map<TriggerSource*, MultiChannelAverageBuffer> m_averageBuffers;
    DataStore* m_datastore;

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

    void addBuffer (const juce::AudioBuffer<float>& buffer);
    AudioBuffer<float> getAverage() const;
    AudioBuffer<float> getStandardDeviation() const;

    void resetTrials();
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
