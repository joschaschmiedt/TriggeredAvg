/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI Plugin Triggered Average
    Copyright (C) 2022 Open Ephys
    Copyright (C) 2025 Joscha Schmiedt

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
#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggerSource.h"
#include "TriggeredAvgCanvas.h"
#include "TriggeredAvgEditor.h"

using namespace TriggeredAverage;

TriggeredAvgNode::TriggeredAvgNode()
    : GenericProcessor ("Triggered Avg"),
      canvas (nullptr),
      // TODO: check if 10 seconds buffer is sufficient (lock-free?) and how to handle more than one stream
      ringBufferSize (
          static_cast<int> (GenericProcessor::getSampleRate (m_dataStreamIndex) * 10.0f)),
      threadsInitialized (false) // 10 seconds buffer
{
    addFloatParameter (Parameter::PROCESSOR_SCOPE,
                       ParameterNames::pre_ms,
                       "Pre MS",
                       "Size of the pre-trigger window in ms",
                       "ms",
                       500.0f,
                       10.0f,
                       5000.0f,
                       10.0f);

    addFloatParameter (Parameter::PROCESSOR_SCOPE,
                       ParameterNames::post_ms,
                       "Post MS",
                       "Size of the post-trigger window in ms",
                       "ms",
                       2000.0f,
                       10.0f,
                       5000.0f,
                       10.0f);

    addIntParameter (Parameter::PROCESSOR_SCOPE,
                     ParameterNames::max_trials,
                     "Max Trials",
                     "Maximum number of trials to store per condition",
                     10,
                     1,
                     100);

    addIntParameter (Parameter::PROCESSOR_SCOPE,
                     ParameterNames::trigger_line,
                     "Trigger Line",
                     "The input TTL line of the current trigger source",
                     0,
                     -1,
                     255);

    addIntParameter (Parameter::PROCESSOR_SCOPE,
                     ParameterNames::trigger_type,
                     "Trigger Type",
                     "The type of the current trigger source",
                     1,
                     1,
                     3);

    // Create a default trigger source for any line
    addTriggerSource (-1, TriggerType::TTL_TRIGGER);
}

TriggeredAvgNode::~TriggeredAvgNode() { shutdownThreads(); }

AudioProcessorEditor* TriggeredAvgNode::createEditor()
{
    editor = std::make_unique<TriggeredAvgEditor> (this);
    return editor.get();
}

void TriggeredAvgNode::parameterValueChanged (Parameter* param)
{
    using namespace ParameterNames;
    // Update trial buffers when max trials changes
    if (param->getName().equalsIgnoreCase (max_trials))
    {
        // TODO:
    }
    else if (param->getName().equalsIgnoreCase (trigger_line))
    {
        if (currentTriggerSource != nullptr)
        {
            currentTriggerSource->line = (int) param->getValue();
        }
    }
    else if (param->getName().equalsIgnoreCase (trigger_type))
    {
        if (currentTriggerSource != nullptr)
        {
            currentTriggerSource->type = (TriggerType) (int) param->getValue();

            if (currentTriggerSource->type == TriggerType::TTL_TRIGGER)
                currentTriggerSource->canTrigger = true;
            else
                currentTriggerSource->canTrigger = false;
        }
    }
    else if (param->getName().equalsIgnoreCase (ParameterNames::pre_ms))
    {
        // TODO:
    }
    else if (param->getName().equalsIgnoreCase (ParameterNames::post_ms))
    {
        // TODO:
    }
}

void TriggeredAvgNode::process (AudioBuffer<float>& buffer)
{
    SampleNumber firstSampleNumber =
        getFirstSampleNumberForBlock (getDataStreams()[m_dataStreamIndex]->getStreamId());
    if (! ringBuffer)
        return;

    ringBuffer->addData (buffer, firstSampleNumber);
    checkForEvents (false);
}

void TriggeredAvgNode::prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock)
{
    GenericProcessor::prepareToPlay (sampleRate, maximumExpectedSamplesPerBlock);
    initializeThreads();
}

float TriggeredAvgNode::getPreWindowSizeMs() const
{
    return getParameter (ParameterNames::pre_ms)->getValue();
}

float TriggeredAvgNode::getPostWindowSizeMs() const
{
    return getParameter (ParameterNames::post_ms)->getValue();
}

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
    name = ensureUniqueTriggerSourceName (name);

    TriggerSource* source = new TriggerSource (this, name, line, type);

    if (index == -1)
        triggerSources.add (source);
    else
        triggerSources.insert (index, source);

    currentTriggerSource = source;
    getParameter ("trigger_type")->setNextValue ((int) type, false);

    return source;
}

void TriggeredAvgNode::removeTriggerSources (Array<TriggerSource*> sources)
{
    for (auto source : sources)
    {
        triggerSources.removeObject (source);
    }
}

void TriggeredAvgNode::removeTriggerSource (int indexToRemove)
{
    if (indexToRemove >= 0 && indexToRemove < triggerSources.size())
    {
        TriggerSource* source = triggerSources[indexToRemove];
        triggerSources.remove (indexToRemove);
    }
}

String TriggeredAvgNode::ensureUniqueTriggerSourceName (String name)
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
    source->name = ensureUniqueTriggerSourceName (name);
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

    if (source->type == TriggerType::TTL_TRIGGER)
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
            int savedType =
                sourceXml->getIntAttribute ("type", static_cast<int> (TriggerType::TTL_TRIGGER));
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
    if (dataCollector && threadsInitialized.load())
    {
        for (auto source : triggerSources)
        {
            if (message.equalsIgnoreCase (source->name))
            {
                if (source->type == TriggerType::TTL_AND_MSG_TRIGGER)
                {
                    source->canTrigger = true;
                }
                else if (source->type == TriggerType::MSG_TRIGGER)
                {
                    for (auto stream : getDataStreams())
                    {
                        const uint16 streamId = stream->getStreamId();
                        // TODO: Do something
                        assert (false);
                    }
                }
            }
        }
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
    if (dataCollector && threadsInitialized.load())
    {
        for (auto source : triggerSources)
        {
            if (event->getLine() == source->line && event->getState() && source->canTrigger)
            {
                //dataCollector->pushEvent (source, event->getStreamId(), event->getSampleNumber());
                int preSamples =
                    (int) (getSampleRate (m_dataStreamIndex) * (getPreWindowSizeMs() / 1000.0f));
                int postSamples =
                    (int) (getSampleRate (m_dataStreamIndex) * (getPostWindowSizeMs() / 1000.0f));
                dataCollector->registerCaptureRequest (
                    CaptureRequest { event->getSampleNumber(), preSamples, postSamples });

                if (source->type == TriggerType::TTL_AND_MSG_TRIGGER)
                    source->canTrigger = false;
            }
        }
    }
}

void TriggeredAvgNode::timerCallback()
{
    // TODO: Whats the purpose of this timer?
    //if (! threadsInitialized.load())
    //return;

    //TriggerSource* source;
    //AudioBuffer<float> data;
    //int64 triggerSample;
}

void TriggeredAvgNode::initializeThreads()
{
    if (threadsInitialized.load())
        shutdownThreads();

    if (getNumInputs() > 0 && ringBufferSize > 0)
    {
        ringBuffer = std::make_unique<MultiChannelRingBuffer> (getNumInputs(), ringBufferSize);
        dataCollector = std::make_unique<DataCollector> (this, ringBuffer.get());

        dataCollector->startThread (Thread::Priority::high);

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

        dataCollector.reset();
        ringBuffer.reset();

        threadsInitialized.store (false);
    }
}
