#pragma once
#include <JuceHeader.h>
#include <ProcessorHeaders.h>

namespace TriggeredAverage
{

class TriggeredAvgNode;
class TriggerSource;
class MultiChannelRingBuffer;

class DataCollector : public Thread
{
public:
    DataCollector (TriggeredAvgNode* viewer, MultiChannelRingBuffer* buffer);
    ~DataCollector() override;

    void pushEvent (const TriggerSource* source, uint16 streamId, int64 sample_number);
    void registerTTLTrigger (TTLEventPtr);
    void registerMessageTrigger (const String& message, int64 sampleNumber);

    /** Thread run function */
    void run() override;

private:
    TriggeredAvgNode* viewer;
    MultiChannelRingBuffer* ringBuffer;

    CriticalSection triggerQueueLock;
    std::deque<TTLEventPtr> ttlTriggerQueue;
    WaitableEvent newTriggerEvent;

    void processTriggerEvent (TTLEventPtr event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataCollector)
};
} // namespace TriggeredAverage
