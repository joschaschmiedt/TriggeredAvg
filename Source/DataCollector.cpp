#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggerSource.h"
#include "TriggeredAvgNode.h"
#include <ProcessorHeaders.h>

using namespace TriggeredAverage;

DataCollector::DataCollector (TriggeredAvgNode* viewer_, MultiChannelRingBuffer* buffer_)
    : Thread ("Trigger Detector"),
      viewer (viewer_),
      ringBuffer (buffer_),
      newTriggerEvent (false)
{
    //setPriority(Thread::Priority::high);
}

DataCollector::~DataCollector() { stopThread (1000); }

void DataCollector::registerCaptureRequest (const CaptureRequest& request)
{
    const ScopedLock lock (triggerQueueLock);
    ttlTriggerQueue.emplace_back (request);
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
            processCaptureRequest (ttlTriggerQueue.front());
            ttlTriggerQueue.pop_front();
        }
    }
}

void DataCollector::processCaptureRequest (const CaptureRequest& request)
{
    if (! viewer)
        return;

    // TODO: read data and add to average buffer
    for (auto source : viewer->getTriggerSources())
    {
        //bool shouldTrigger = false;

        //switch (event->getEventType())
        //{
        //    case EventChannel::TTL:
        //    {
        //        bool lineMatches = (source->line == -1) || (event->getLine() == source->line);
        //        if (lineMatches && event->getState() && source->canTrigger
        //            && (source->type == TriggerType::TTL_TRIGGER
        //                || source->type == TriggerType::TTL_AND_MSG_TRIGGER))
        //        {
        //            shouldTrigger = true;
        //        }
        //        break;
        //    }
        //    case EventChannel::TEXT:
        //        break;
        //    case EventChannel::CUSTOM:
        //        break;
        //    case EventChannel::INVALID:
        //        break;
        //}

        //if (shouldTrigger)
        //{
        //    // TODO: Trigger processing via
        //    //viewer->requestTriggeredCapture (source, event.sampleNumber);

        //    if (source->type == TriggerType::TTL_AND_MSG_TRIGGER)
        //        source->canTrigger = false;

        AudioBuffer<float> buffer;
        auto result = ringBuffer->readAroundSample (request.triggerSample,
                                                    request.preSamples,
                                                    request.postSamples,
                                                    request.channelIndices,
                                                    buffer);
    }
}
