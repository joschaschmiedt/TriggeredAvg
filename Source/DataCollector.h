#pragma once
#include "MultiChannelRingBuffer.h"

#include <JuceHeader.h>
#include <ProcessorHeaders.h>

namespace TriggeredAverage
{
class TriggeredAvgNode;
class TriggerSource;
class MultiChannelRingBuffer;

struct CaptureRequest
{
    TriggerSource* triggerSource;
    SampleNumber triggerSample;
    int preSamples;
    int postSamples;
    Array<int> channelIndices;

    CaptureRequest() = delete;
    CaptureRequest (TriggerSource* triggerSource_,
                    SampleNumber triggerSample_,
                    int pre,
                    int post,
                    const Array<int>& channels)
        : triggerSource (triggerSource_),
          triggerSample (triggerSample_),
          preSamples (pre),
          postSamples (post),
          channelIndices (channels)
    {
    }
};

class DataCollector : public Thread
{
public:
    DataCollector (TriggeredAvgNode* viewer, MultiChannelRingBuffer* buffer);
    ~DataCollector() override;
    void run() override;

    void registerCaptureRequest (const CaptureRequest&);
    void registerMessageTrigger (const String& message, int64 sampleNumber);

private:
    TriggeredAvgNode* viewer;
    MultiChannelRingBuffer* ringBuffer;

    CriticalSection triggerQueueLock;
    std::deque<CaptureRequest> ttlTriggerQueue;
    WaitableEvent newTriggerEvent;
    
    std::unordered_map<TriggerSource*, AudioBuffer<float>> m_averageBuffer;

    void processCaptureRequest (const CaptureRequest& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataCollector)
};
} // namespace TriggeredAverage
