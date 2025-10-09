#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggeredAvgNode.h"
#include <ProcessorHeaders.h>

using namespace TriggeredAverage;
// using enum TriggeredAverage::TriggerType;

DataCollector::DataCollector (TriggeredAvgNode* viewer_, MultiChannelRingBuffer* buffer_)
    : Thread ("Trigger Detector"),
      viewer (viewer_),
      ringBuffer (buffer_),
      newTriggerEvent (false)
{
    //setPriority(Thread::Priority::high);
}

DataCollector::~DataCollector() { stopThread (1000); }

void DataCollector::pushEvent (const TriggerSource* source, uint16 streamId, int64 sample_number)
{
    //if (source->type == TriggerType::TTL_TRIGGER
    //    || source->type == TriggerType::TTL_AND_MSG_TRIGGER)
    //{
    //    auto ttlEvent =
    //        TTLEvent::createTTLEvent (, sample_number, (uint8) source->line, true);
    //    registerTTLTrigger (ttlEvent);
    //}
    // else if (source->type == TriggerType::MSG_TRIGGER)
    // {
    //     registerMessageTrigger (source->name, sample_number);
    // }
}

void DataCollector::registerTTLTrigger (TTLEventPtr ttlEvent)
{
    const ScopedLock lock (triggerQueueLock);
    ttlTriggerQueue.emplace_back (ttlEvent);
    newTriggerEvent.signal();
}

void DataCollector::registerMessageTrigger (const String& message, int64 sampleNumber)
{
    const ScopedLock lock (triggerQueueLock);
    //ttlTriggerQueue.emplace_back (message, sampleNumber);
    newTriggerEvent.signal();
}

void DataCollector::run()
{
    while (! threadShouldExit())
    {
        bool result = newTriggerEvent.wait (100);

        const ScopedLock lock (triggerQueueLock);
        while (! ttlTriggerQueue.empty())
        {
            processTriggerEvent (ttlTriggerQueue.front());
            ttlTriggerQueue.pop_front();
        }
    }
}

void DataCollector::processTriggerEvent (TTLEventPtr event)
{
    if (! viewer)
        return;

    auto triggerSources = viewer->getTriggerSources();

    for (auto source : triggerSources)
    {
        bool shouldTrigger = false;

        switch (event->getEventType())
        {
            case EventChannel::TTL:
            {
                bool lineMatches = (source->line == -1) || (event->getLine() == source->line);
                if (lineMatches && event->getState() && source->canTrigger
                    && (source->type == TriggerType::TTL_TRIGGER
                        || source->type == TriggerType::TTL_AND_MSG_TRIGGER))
                {
                    shouldTrigger = true;
                }
                break;
            }
            case EventChannel::TEXT:
                break;
            case EventChannel::CUSTOM:
                break;
            case EventChannel::INVALID:
                break;
        }

        if (shouldTrigger)
        {
            // TODO: Trigger processing via
            //viewer->requestTriggeredCapture (source, event.sampleNumber);

            if (source->type == TriggerType::TTL_AND_MSG_TRIGGER)
                source->canTrigger = false;
        }
    }
}

struct CaptureRequest
{
    TriggerSource* source;
    int64 triggerSample;
    int preSamples;
    int postSamples;
    Array<int> channelIndices;

    CaptureRequest (TriggerSource* s, int64 ts, int pre, int post, Array<int> channels)
        : source (s),
          triggerSample (ts),
          preSamples (pre),
          postSamples (post),
          channelIndices (channels)
    {
    }
};

static void processCaptureRequest (const TriggeredAverage::MultiChannelRingBuffer* ringBuffer,
                                   CaptureRequest& request)
{
    if (! ringBuffer || ! request.source)
        return;

    int totalSamples = request.preSamples + request.postSamples;

    // Create the completed capture with the correct source
    //auto capture = std::make_unique<CompletedCapture> (
    //request.source, request.triggerSample, request.channelIndices.size(), totalSamples);
    AudioBuffer<float> buffer;

    // TODO: wait until all the required data is there

    bool result = ringBuffer->readTriggeredData (request.triggerSample,
                                                 request.preSamples,
                                                 request.postSamples,
                                                 request.channelIndices,
                                                 buffer);
}
