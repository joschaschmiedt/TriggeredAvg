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
    void run() override;

    void registerTTLTrigger (TTLEventPtr);
    void registerMessageTrigger (const String& message, int64 sampleNumber);


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
