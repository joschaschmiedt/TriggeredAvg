/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2025 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "TriggeredAvgNode.h"
#include "TriggeredAvgCanvas.h"
#include "TriggeredAvgEditor.h"

using namespace TriggeredAverage;
using enum TriggeredAverage::TriggerType;

//==============================================================================
// LFPTriggerDetector implementation
//==============================================================================
TriggerDetector::TriggerDetector (TriggeredAvgNode* viewer_, MultiChannelRingBuffer* buffer_)
    : Thread ("Trigger Detector"),
      viewer (viewer_),
      ringBuffer (buffer_),
      newTriggerEvent (false)
{
    //setPriority(Thread::Priority::high);
}

TriggerDetector::~TriggerDetector() { stopThread (1000); }

void TriggerDetector::registerTTLTrigger (int line, bool state, int64 sampleNumber, uint16 streamId)
{
    const ScopedLock lock (triggerQueueLock);
    triggerQueue.emplace_back (line, state, sampleNumber, streamId);
    newTriggerEvent.signal();
}

void TriggerDetector::registerMessageTrigger (const String& message, int64 sampleNumber)
{
    const ScopedLock lock (triggerQueueLock);
    triggerQueue.emplace_back (message, sampleNumber);
    newTriggerEvent.signal();
}

void TriggerDetector::run()
{
    while (! threadShouldExit())
    {
        newTriggerEvent.wait (100);

        while (! threadShouldExit())
        {
            TriggerEvent event (0, false, 0, 0); // Default constructor
            bool hasEvent = false;

            {
                const ScopedLock lock (triggerQueueLock);
                if (! triggerQueue.empty())
                {
                    event = triggerQueue.front();
                    triggerQueue.pop_front();
                    hasEvent = true;
                }
            }

            if (! hasEvent)
                break;

            processTriggerEvent (event);
        }
    }
}

void TriggerDetector::processTriggerEvent (const TriggerEvent& event)
{
    if (! viewer)
        return;

    auto triggerSources = viewer->getTriggerSources();

    for (auto source : triggerSources)
    {
        bool shouldTrigger = false;

        if (event.type == TriggerEvent::TTL)
        {
            bool lineMatches = (source->line == -1) || (event.line == source->line);
            if (lineMatches && event.state && source->canTrigger
                && (source->type == TTL_TRIGGER || source->type == TTL_AND_MSG_TRIGGER))
            {
                shouldTrigger = true;
            }
        }
        else if (event.type == TriggerEvent::MESSAGE)
        {
            if (event.message.equalsIgnoreCase (source->name) && source->canTrigger
                && (source->type == MSG_TRIGGER || source->type == TTL_AND_MSG_TRIGGER))
            {
                shouldTrigger = true;
            }
        }

        if (shouldTrigger)
        {
            //viewer->requestTriggeredCapture (source, event.sampleNumber);

            if (source->type == TTL_AND_MSG_TRIGGER)
                source->canTrigger = false;
        }
    }
}

CaptureManager::CaptureManager (MultiChannelRingBuffer* buffer_)
    : Thread ("Trigger Extractor "),
      ringBuffer (buffer_),
      newRequestEvent (false)
{
    //setPriority(Thread::Priority::normal);
}

CaptureManager::~CaptureManager() { stopThread (1000); }

void CaptureManager::requestCapture (TriggerSource* source,
                                     int64 triggerSample,
                                     int preSamples,
                                     int postSamples,
                                     Array<int> channelIndices)
{
    const ScopedLock lock (requestQueueLock);
    captureRequests.emplace_back (source, triggerSample, preSamples, postSamples, channelIndices);
    newRequestEvent.signal();
}

void CaptureManager::run()
{
    while (! threadShouldExit())
    {
        newRequestEvent.wait (100);

        while (! threadShouldExit())
        {
            CaptureRequest request (nullptr, 0, 0, 0, Array<int>());
            bool hasRequest = false;

            {
                const ScopedLock lock (requestQueueLock);
                if (! captureRequests.empty())
                {
                    request = captureRequests.front();
                    captureRequests.pop_front();
                    hasRequest = true;
                }
            }

            if (! hasRequest)
                break;

            processCaptureRequest (request);
        }
    }
}

bool CaptureManager::getCompletedCapture (TriggerSource*& source,
                                          AudioBuffer<float>& data,
                                          int64& triggerSample)
{
    const ScopedLock lock (completedQueueLock);

    if (completedCaptures.empty())
        return false;

    auto capture = std::move (completedCaptures.front());
    completedCaptures.pop_front();

    source = capture->source;
    triggerSample = capture->triggerSample;

    // Use move semantics to transfer the data efficiently
    data = std::move (capture->data);

    return true;
}

bool CaptureManager::getCompletedTrialData (std::unique_ptr<ContTrialData>& trial)
{
    const ScopedLock lock (completedQueueLock);

    if (completedCaptures.empty())
        return false;

    auto capture = std::move (completedCaptures.front());
    completedCaptures.pop_front();

    // Create the trial data directly from the completed capture
    trial = std::make_unique<ContTrialData> (
        capture->data.getNumChannels(), capture->data.getNumSamples(), capture->triggerSample);

    // Copy the data efficiently
    auto& trialData = const_cast<AudioBuffer<float>&> (trial->getData());
    trialData.makeCopyOf (capture->data);

    return true;
}

void CaptureManager::processCaptureRequest (const CaptureRequest& request)
{
    if (! ringBuffer || ! request.source)
        return;

    int totalSamples = request.preSamples + request.postSamples;

    // Create the completed capture with the correct source
    auto capture = std::make_unique<CompletedCapture> (
        request.source, request.triggerSample, request.channelIndices.size(), totalSamples);

    // Read data directly into the capture buffer
    if (ringBuffer->readTriggeredData (request.triggerSample,
                                       request.preSamples,
                                       request.postSamples,
                                       request.channelIndices,
                                       capture->data))
    {
        const ScopedLock lock (completedQueueLock);
        completedCaptures.push_back (std::move (capture));
    }
}

//==============================================================================
// LFPTrialData implementation
//==============================================================================
ContTrialData::ContTrialData (int numChannels, int numSamples, int64 triggerSample_)
    : data (numChannels, numSamples),
      triggerSample (triggerSample_)
{
}

void ContTrialData::getDownsampledData (AudioBuffer<float>& output, int targetSamples) const
{
    int sourceLength = data.getNumSamples();

    if (sourceLength <= targetSamples)
    {
        output.makeCopyOf (data);
        return;
    }

    output.setSize (data.getNumChannels(), targetSamples);

    float ratio = (float) sourceLength / targetSamples;

    for (int ch = 0; ch < data.getNumChannels(); ++ch)
    {
        for (int i = 0; i < targetSamples; ++i)
        {
            int sourceIndex = (int) (i * ratio);
            if (sourceIndex < sourceLength)
                output.setSample (ch, i, data.getSample (ch, sourceIndex));
        }
    }
}

//==============================================================================
// LFPTrialBuffer implementation
//==============================================================================
ContTrialBuffer::ContTrialBuffer (int maxTrials_) : maxTrials (maxTrials_) {}

ContTrialBuffer::~ContTrialBuffer() {}

void ContTrialBuffer::addTrial (std::unique_ptr<ContTrialData> trial)
{
    const ScopedLock lock (trialsLock);

    trials.push_back (std::move (trial));

    while (trials.size() > maxTrials)
    {
        trials.pop_front();
    }
}

const ContTrialData* ContTrialBuffer::getTrial (int index) const
{
    const ScopedLock lock (trialsLock);

    if (index >= 0 && index < trials.size())
        return trials[index].get();

    return nullptr;
}

void ContTrialBuffer::getAveragedData (AudioBuffer<float>& output) const
{
    const ScopedLock lock (trialsLock);

    if (trials.empty())
        return;

    const auto& firstTrial = trials[0]->getData();
    output.setSize (firstTrial.getNumChannels(), firstTrial.getNumSamples());
    output.clear();

    // Sum all trials
    for (const auto& trial : trials)
    {
        const auto& trialData = trial->getData();
        for (int ch = 0; ch < output.getNumChannels(); ++ch)
        {
            output.addFrom (ch, 0, trialData, ch, 0, trialData.getNumSamples());
        }
    }

    // Average
    float numTrials = (float) trials.size();
    output.applyGain (1.0f / numTrials);
}

void ContTrialBuffer::clear()
{
    const ScopedLock lock (trialsLock);
    trials.clear();
}

//==============================================================================
// TriggeredLFPViewer implementation
//==============================================================================
TriggeredAvgNode::TriggeredAvgNode()
    : GenericProcessor ("Triggered Avg"),
      canvas (nullptr),
      allChannelsSelected (true),
      ringBufferSize (getSampleRate (0) * 10),
      threadsInitialized (false) // 10 seconds buffer
{
    addFloatParameter (Parameter::PROCESSOR_SCOPE,
                       "pre_ms",
                       "Pre MS",
                       "Size of the pre-trigger window in ms",
                       "ms",
                       500.0f,
                       10.0f,
                       5000.0f,
                       10.0f);

    addFloatParameter (Parameter::PROCESSOR_SCOPE,
                       "post_ms",
                       "Post MS",
                       "Size of the post-trigger window in ms",
                       "ms",
                       2000.0f,
                       10.0f,
                       5000.0f,
                       10.0f);

    addIntParameter (Parameter::PROCESSOR_SCOPE,
                     "max_trials",
                     "Max Trials",
                     "Maximum number of trials to store per condition",
                     10,
                     1,
                     100);

    addIntParameter (Parameter::PROCESSOR_SCOPE,
                     "trigger_line",
                     "Trigger Line",
                     "The input TTL line of the current trigger source",
                     0,
                     -1,
                     255);

    addIntParameter (Parameter::PROCESSOR_SCOPE,
                     "trigger_type",
                     "Trigger Type",
                     "The type of the current trigger source",
                     1,
                     1,
                     3);

    // Create a default trigger source for any line
    addTriggerSource (-1, TTL_TRIGGER);
}

TriggeredAvgNode::~TriggeredAvgNode() { shutdownThreads(); }

AudioProcessorEditor* TriggeredAvgNode::createEditor()
{
    editor = std::make_unique<TriggeredAvgEditor> (this);
    return editor.get();
}

void TriggeredAvgNode::parameterValueChanged (Parameter* param)
{
    // Update trial buffers when max trials changes
    if (param->getName().equalsIgnoreCase ("max_trials"))
    {
        for (auto& pair : trialBuffers)
        {
            pair.second->clear(); // Clear existing trials
        }
    }
    else if (param->getName().equalsIgnoreCase ("trigger_line"))
    {
        if (currentTriggerSource != nullptr)
        {
            currentTriggerSource->line = (int) param->getValue();
        }
    }
    else if (param->getName().equalsIgnoreCase ("trigger_type"))
    {
        if (currentTriggerSource != nullptr)
        {
            currentTriggerSource->type = (TriggerType) (int) param->getValue();

            if (currentTriggerSource->type == TTL_TRIGGER)
                currentTriggerSource->canTrigger = true;
            else
                currentTriggerSource->canTrigger = false;
        }
    }
}

void TriggeredAvgNode::process (AudioBuffer<float>& buffer)
{
    int64 firstSampleNumber = getFirstSampleNumberForBlock (getDataStreams()[0]->getStreamId());
    ringBuffer->addData (buffer, firstSampleNumber);

    checkForEvents (true);
}

float TriggeredAvgNode::getPreWindowSizeMs() const { return getParameter ("pre_ms")->getValue(); }

float TriggeredAvgNode::getPostWindowSizeMs() const { return getParameter ("post_ms")->getValue(); }

Array<TriggerSource*> TriggeredAvgNode::getTriggerSources()
{
    Array<TriggerSource*> sources;
    for (auto source : triggerSources)
        sources.add (source);
    return sources;
}

TriggerSource* TriggeredAvgNode::addTriggerSource (int line, TriggerType type, int index)
{
    String name = "Condition " + String (nextConditionIndex++);
    name = ensureUniqueName (name);

    TriggerSource* source = new TriggerSource (this, name, line, type);

    if (index == -1)
        triggerSources.add (source);
    else
        triggerSources.insert (index, source);

    // Create trial buffer for this source
    trialBuffers[source] = std::make_unique<ContTrialBuffer> (getMaxTrials());

    currentTriggerSource = source;
    getParameter ("trigger_type")->setNextValue ((int) type, false);

    return source;
}

void TriggeredAvgNode::removeTriggerSources (Array<TriggerSource*> sources)
{
    for (auto source : sources)
    {
        trialBuffers.erase (source);
        triggerSources.removeObject (source);
    }
}

void TriggeredAvgNode::removeTriggerSource (int indexToRemove)
{
    if (indexToRemove >= 0 && indexToRemove < triggerSources.size())
    {
        TriggerSource* source = triggerSources[indexToRemove];
        trialBuffers.erase (source);
        triggerSources.remove (indexToRemove);
    }
}

String TriggeredAvgNode::ensureUniqueName (String name)
{
    Array<String> existingNames;
    for (auto source : triggerSources)
        existingNames.add (source->name);

    if (! existingNames.contains (name))
        return name;

    int suffix = 2;
    while (existingNames.contains (name + " " + String (suffix)))
        suffix++;

    return name + " " + String (suffix);
}

void TriggeredAvgNode::setTriggerSourceName (TriggerSource* source, String name, bool updateEditor)
{
    source->name = ensureUniqueName (name);
}

void TriggeredAvgNode::setTriggerSourceLine (TriggerSource* source, int line, bool updateEditor)
{
    source->line = line;
    source->colour = TriggerSource::getColourForLine (line);
}

void TriggeredAvgNode::setTriggerSourceColour (TriggerSource* source,
                                               Colour colour,
                                               bool updateEditor)
{
    source->colour = colour;
}

void TriggeredAvgNode::setTriggerSourceTriggerType (TriggerSource* source,
                                                    TriggerType type,
                                                    bool updateEditor)
{
    source->type = type;

    if (source->type == TTL_TRIGGER)
        source->canTrigger = true;
    else
        source->canTrigger = false;
}

void TriggeredAvgNode::saveCustomParametersToXml (XmlElement* xml)
{
    for (auto source : triggerSources)
    {
        XmlElement* sourceXml = xml->createNewChildElement ("TRIGGERSOURCE");
        sourceXml->setAttribute ("name", source->name);
        sourceXml->setAttribute ("line", source->line);
        sourceXml->setAttribute ("type", static_cast<int> (source->type));
        sourceXml->setAttribute ("colour", source->colour.toString());
    }

    // Save channel selection
    XmlElement* channelsXml = xml->createNewChildElement ("CHANNELS");
    channelsXml->setAttribute ("allSelected", allChannelsSelected);
    if (! allChannelsSelected)
    {
        for (int ch : selectedChannels)
        {
            XmlElement* channelXml = channelsXml->createNewChildElement ("CHANNEL");
            channelXml->setAttribute ("index", ch);
        }
    }
}

void TriggeredAvgNode::loadCustomParametersFromXml (XmlElement* xml)
{
    triggerSources.clear();
    nextConditionIndex = 1;

    for (auto sourceXml : xml->getChildIterator())
    {
        if (sourceXml->hasTagName ("TRIGGERSOURCE"))
        {
            String savedName = sourceXml->getStringAttribute ("name");
            int savedLine = sourceXml->getIntAttribute ("line", 0);
            int savedType = sourceXml->getIntAttribute ("type", static_cast<int> (TTL_TRIGGER));
            String savedColour = sourceXml->getStringAttribute ("colour", "");

            TriggerSource* source =
                addTriggerSource (savedLine, static_cast<TriggerType> (savedType));

            if (savedName.isNotEmpty())
                source->name = savedName;

            if (savedColour.length() > 0)
                source->colour = Colour::fromString (savedColour);
        }
    }
}

void TriggeredAvgNode::handleBroadcastMessage (const String& message, const int64 sysTimeMs)
{
    if (triggerDetector && threadsInitialized.load())
    {
        // Estimate sample number from system time
        int64 sampleNumber = 0;
        if (getDataStreams().size() > 0)
        {
            float sampleRate = getDataStreams()[0]->getSampleRate();
            sampleNumber = (int64) (sysTimeMs * sampleRate / 1000.0);
        }

        triggerDetector->registerMessageTrigger (message, sampleNumber);
    }
}

String TriggeredAvgNode::handleConfigMessage (const String& message) { return ""; }

bool TriggeredAvgNode::getIntField (DynamicObject::Ptr payload,
                                    String name,
                                    int& value,
                                    int lowerBound,
                                    int upperBound)
{
    if (payload->hasProperty (name))
    {
        value = payload->getProperty (name);
        if (value >= lowerBound && value <= upperBound)
            return true;
    }
    return false;
}

void TriggeredAvgNode::handleTTLEvent (TTLEventPtr event)
{
    if (triggerDetector && threadsInitialized.load())
    {
        triggerDetector->registerTTLTrigger (
            event->getLine(), event->getState(), event->getSampleNumber(), event->getStreamId());
    }
}

void TriggeredAvgNode::timerCallback()
{
    if (! captureManager || ! threadsInitialized.load())
        return;

    TriggerSource* source;
    AudioBuffer<float> data;
    int64 triggerSample;

    while (captureManager->getCompletedCapture (source, data, triggerSample))
    {
        // Create trial data object
        auto trial = std::make_unique<ContTrialData> (
            data.getNumChannels(), data.getNumSamples(), triggerSample);

        // Move the data into the trial (no copying!)
        auto& trialData = const_cast<AudioBuffer<float>&> (trial->getData());
        trialData = std::move (data);

        if (trialBuffers.find (source) != trialBuffers.end())
        {
            trialBuffers[source]->addTrial (std::move (trial));
        }

        if (canvas != nullptr)
        {
            //canvas->newDataReceived (source);
        }
    }
}

void TriggeredAvgNode::initializeThreads()
{
    if (threadsInitialized.load())
        shutdownThreads();

    if (getNumInputs() > 0 && ringBufferSize > 0)
    {
        ringBuffer = std::make_unique<MultiChannelRingBuffer> (getNumInputs(), ringBufferSize);
        triggerDetector = std::make_unique<TriggerDetector> (this, ringBuffer.get());
        captureManager = std::make_unique<CaptureManager> (ringBuffer.get());

        triggerDetector->startThread (Thread::Priority::high);
        captureManager->startThread (Thread::Priority::normal);

        threadsInitialized.store (true);

        // Start timer to poll for completed captures every 50ms
        startTimer (50);
    }
}

void TriggeredAvgNode::shutdownThreads()
{
    if (threadsInitialized.load())
    {
        // Stop the timer first to prevent callbacks with invalid objects
        stopTimer();

        triggerDetector.reset();
        captureManager.reset();
        ringBuffer.reset();

        threadsInitialized.store (false);
    }
}