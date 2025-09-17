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

#include "TriggeredLFPViewer.h"

#include <stdio.h>

#include "TriggeredLFPCanvas.h"
#include "TriggeredLFPEditor.h"

// LFPDataBuffer implementation
LFPDataBuffer::LFPDataBuffer(int maxTrials_) : maxTrials(maxTrials_), preSamples(0), postSamples(0), numChannels(0)
{
}

void LFPDataBuffer::setParameters(int preSamples_, int postSamples_, int numChannels_)
{
    preSamples = preSamples_;
    postSamples = postSamples_;
    numChannels = numChannels_;
    clear(); // Clear existing data when parameters change
}

void LFPDataBuffer::addTrial(const Array<float*>& channelData, int64 triggerSample)
{
    if (numChannels == 0 || preSamples + postSamples == 0)
        return;

    Array<Array<float>> newTrial;
    
    for (int ch = 0; ch < numChannels; ch++)
    {
        Array<float> channelTrialData;
        
        // Copy data from the continuous buffer
        for (int sample = 0; sample < preSamples + postSamples; sample++)
        {
            if (channelData[ch] != nullptr)
                channelTrialData.add(channelData[ch][sample]);
            else
                channelTrialData.add(0.0f);
        }
        
        newTrial.add(channelTrialData);
    }
    
    trials.push_back(newTrial);
    
    // Keep only the most recent trials
    while (trials.size() > maxTrials)
    {
        trials.pop_front();
    }
}

void LFPDataBuffer::clear()
{
    trials.clear();
}

const Array<float>& LFPDataBuffer::getTrialData(int trialIndex, int channelIndex) const
{
    static Array<float> emptyArray;
    
    if (trialIndex >= 0 && trialIndex < trials.size() && 
        channelIndex >= 0 && channelIndex < numChannels)
    {
        return trials[trialIndex][channelIndex];
    }
    
    return emptyArray;
}

Array<float> LFPDataBuffer::getAveragedData(int channelIndex) const
{
    Array<float> averaged;
    
    if (trials.empty() || channelIndex < 0 || channelIndex >= numChannels)
        return averaged;
    
    int totalSamples = preSamples + postSamples;
    averaged.resize(totalSamples);
    averaged.fill(0.0f);
    
    // Sum all trials
    for (const auto& trial : trials)
    {
        for (int sample = 0; sample < totalSamples; sample++)
        {
            averaged.getReference(sample) += trial[channelIndex][sample];
        }
    }
    
    // Average
    float numTrials = (float)trials.size();
    for (int sample = 0; sample < totalSamples; sample++)
    {
        averaged.getReference(sample) /= numTrials;
    }
    
    return averaged;
}

const Array<float>& LFPDataBuffer::getLatestTrialData(int channelIndex) const
{
    static Array<float> emptyArray;
    
    if (!trials.empty() && channelIndex >= 0 && channelIndex < numChannels)
    {
        return trials.back()[channelIndex];
    }
    
    return emptyArray;
}

// TriggeredLFPViewer implementation
TriggeredLFPViewer::TriggeredLFPViewer()
    : GenericProcessor("Triggered LFP Viewer"),
      canvas(nullptr),
      bufferSize(0),
      writeIndex(0),
      bufferFull(false),
      allChannelsSelected(true)
{
    addIntParameter(Parameter::PROCESSOR_SCOPE,
                    "pre_ms",
                    "Pre MS",
                    "Size of the pre-trigger window in ms",
                    500,
                    10,
                    2000);

    addIntParameter(Parameter::PROCESSOR_SCOPE,
                    "post_ms",
                    "Post MS",
                    "Size of the post-trigger window in ms",
                    2000,
                    10,
                    5000);

    addIntParameter(Parameter::PROCESSOR_SCOPE,
                    "max_trials",
                    "Max Trials",
                    "Maximum number of trials to store per condition",
                    10,
                    1,
                    100);

    addIntParameter(Parameter::PROCESSOR_SCOPE,
                    "trigger_line",
                    "Trigger Line",
                    "The input TTL line of the current trigger source",
                    0,
                    -1,
                    255);

    addIntParameter(Parameter::PROCESSOR_SCOPE,
                    "trigger_type",
                    "Trigger Type",
                    "The type of the current trigger source",
                    1,
                    1,
                    3);
}

AudioProcessorEditor* TriggeredLFPViewer::createEditor()
{
    editor = std::make_unique<TriggeredLFPEditor>(this);
    return editor.get();
}

void TriggeredLFPViewer::parameterValueChanged(Parameter* param)
{
    if (param->getName().equalsIgnoreCase("pre_ms"))
    {
        if (canvas != nullptr)
            canvas->setWindowSizeMs((int)param->getValue(),
                                   (int)getParameter("post_ms")->getValue());
    }
    else if (param->getName().equalsIgnoreCase("post_ms"))
    {
        if (canvas != nullptr)
            canvas->setWindowSizeMs((int)getParameter("pre_ms")->getValue(),
                                   (int)param->getValue());
    }
    else if (param->getName().equalsIgnoreCase("max_trials"))
    {
        // Update all data buffers with new max trials
        for (auto& pair : dataBuffers)
        {
            // Note: Would need to recreate buffers with new max trials
            // For now, just clear them
            pair.second->clear();
        }
    }
    else if (param->getName().equalsIgnoreCase("trigger_line"))
    {
        if (currentTriggerSource != nullptr)
        {
            currentTriggerSource->line = (int)param->getValue();
        }
    }
    else if (param->getName().equalsIgnoreCase("trigger_type"))
    {
        if (currentTriggerSource != nullptr)
        {
            currentTriggerSource->type = (TriggerType)(int)param->getValue();

            if (currentTriggerSource->type == TTL_TRIGGER)
                currentTriggerSource->canTrigger = true;
            else
                currentTriggerSource->canTrigger = false;
        }
    }
}

void TriggeredLFPViewer::process(AudioBuffer<float>& buffer)
{
    // Update continuous buffer for triggered data collection
    updateContinuousBuffer(buffer);
    
    // Check for events (TTLs and messages)
    checkForEvents(true);
}

void TriggeredLFPViewer::updateContinuousBuffer(const AudioBuffer<float>& buffer)
{
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();
    
    // Initialize buffer if needed
    if (continuousBuffer.size() != numChannels || bufferSize == 0)
    {
        // Calculate buffer size based on maximum window size
        if (getDataStreams().size() > 0)
        {
            float sampleRate = getDataStreams()[0]->getSampleRate();
            if (sampleRate > 0)
            {
                int maxWindowMs = jmax(getPreWindowSizeMs(), getPostWindowSizeMs());
                bufferSize = (int)(sampleRate * (maxWindowMs + 1000) / 1000); // Add 1 second buffer
                
                continuousBuffer.clear();
                for (int ch = 0; ch < numChannels; ch++)
                {
                    Array<float> channelBuffer;
                    channelBuffer.resize(bufferSize);
                    channelBuffer.fill(0.0f);
                    continuousBuffer.add(channelBuffer);
                }
                
                writeIndex = 0;
                bufferFull = false;
            }
        }
    }
    
    // Copy new samples to circular buffer
    for (int ch = 0; ch < numChannels && ch < continuousBuffer.size(); ch++)
    {
        for (int sample = 0; sample < numSamples; sample++)
        {
            continuousBuffer.getReference(ch).set(writeIndex + sample, buffer.getSample(ch, sample));
        }
    }
    
    writeIndex += numSamples;
    if (writeIndex >= bufferSize)
    {
        writeIndex = 0;
        bufferFull = true;
    }
}

int TriggeredLFPViewer::getPreWindowSizeMs()
{
    return (int)getParameter("pre_ms")->getValue();
}

int TriggeredLFPViewer::getPostWindowSizeMs()
{
    return (int)getParameter("post_ms")->getValue();
}

int TriggeredLFPViewer::getMaxTrials()
{
    return (int)getParameter("max_trials")->getValue();
}

Array<LFPTriggerSource*> TriggeredLFPViewer::getTriggerSources()
{
    Array<LFPTriggerSource*> sources;

    for (auto source : triggerSources)
    {
        sources.add(source);
    }

    return sources;
}

LFPTriggerSource* TriggeredLFPViewer::addTriggerSource(int line, TriggerType type, int index)
{
    String name = "Condition " + String(nextConditionIndex++);
    name = ensureUniqueName(name);

    LFPTriggerSource* source = new LFPTriggerSource(this, name, line, type);

    if (index == -1)
        triggerSources.add(source);
    else
        triggerSources.insert(index, source);

    // Create data buffer for this source
    dataBuffers[source] = std::make_unique<LFPDataBuffer>(getMaxTrials());

    currentTriggerSource = source;
    getParameter("trigger_type")->setNextValue((int)type, false);

    return source;
}

void TriggeredLFPViewer::removeTriggerSources(Array<LFPTriggerSource*> sources)
{
    for (auto source : sources)
    {
        dataBuffers.erase(source);
        triggerSources.removeObject(source);
    }
}

void TriggeredLFPViewer::removeTriggerSource(int indexToRemove)
{
    if (indexToRemove >= 0 && indexToRemove < triggerSources.size())
    {
        LFPTriggerSource* source = triggerSources[indexToRemove];
        dataBuffers.erase(source);
        triggerSources.remove(indexToRemove);
    }
}

String TriggeredLFPViewer::ensureUniqueName(String name)
{
    Array<String> existingNames;

    for (auto source : triggerSources)
        existingNames.add(source->name);

    if (!existingNames.contains(name))
        return name;

    int suffix = 2;
    while (existingNames.contains(name + " " + String(suffix)))
        suffix++;

    return name + " " + String(suffix);
}

void TriggeredLFPViewer::setTriggerSourceName(LFPTriggerSource* source, String name, bool updateEditor)
{
    source->name = ensureUniqueName(name);
}

void TriggeredLFPViewer::setTriggerSourceLine(LFPTriggerSource* source, int line, bool updateEditor)
{
    source->line = line;
    source->colour = LFPTriggerSource::getColourForLine(line);
}

void TriggeredLFPViewer::setTriggerSourceColour(LFPTriggerSource* source, Colour colour, bool updateEditor)
{
    source->colour = colour;
}

void TriggeredLFPViewer::setTriggerSourceTriggerType(LFPTriggerSource* source, TriggerType type, bool updateEditor)
{
    source->type = type;

    if (source->type == TTL_TRIGGER)
        source->canTrigger = true;
    else
        source->canTrigger = false;
}

Array<int> TriggeredLFPViewer::getSelectedChannels()
{
    if (allChannelsSelected)
    {
        Array<int> allChannels;
        for (int i = 0; i < getNumInputs(); i++)
            allChannels.add(i);
        return allChannels;
    }
    return selectedChannels;
}

void TriggeredLFPViewer::setSelectedChannels(Array<int> channels)
{
    selectedChannels = channels;
    allChannelsSelected = false;
}

void TriggeredLFPViewer::clearTriggerSourceData(LFPTriggerSource* source)
{
    if (dataBuffers.find(source) != dataBuffers.end())
    {
        dataBuffers[source]->clear();
    }
}

void TriggeredLFPViewer::clearAllData()
{
    for (auto& pair : dataBuffers)
    {
        pair.second->clear();
    }
}

LFPDataBuffer* TriggeredLFPViewer::getDataBuffer(LFPTriggerSource* source)
{
    if (dataBuffers.find(source) != dataBuffers.end())
    {
        return dataBuffers[source].get();
    }
    return nullptr;
}

void TriggeredLFPViewer::handleBroadcastMessage(const String& message, const int64 sysTimeMs)
{
    LOGD("Triggered LFP Viewer received ", message);

    for (auto source : triggerSources)
    {
        if (message.equalsIgnoreCase(source->name))
        {
            if (source->type == TTL_AND_MSG_TRIGGER)
            {
                source->canTrigger = true;
            }
            else if (source->type == MSG_TRIGGER)
            {
                if (canvas != nullptr)
                {
                    for (auto stream : getDataStreams())
                    {
                        const uint16 streamId = stream->getStreamId();
                        collectTriggeredData(source, streamId, getFirstSampleNumberForBlock(streamId));
                    }
                }
            }
        }
    }
}

String TriggeredLFPViewer::handleConfigMessage(const String& message)
{
    return "";
}

bool TriggeredLFPViewer::getIntField(DynamicObject::Ptr payload,
                                    String name,
                                    int& value,
                                    int lowerBound,
                                    int upperBound)
{
    if (payload->hasProperty(name))
    {
        value = payload->getProperty(name);
        if (value >= lowerBound && value <= upperBound)
            return true;
    }
    return false;
}

void TriggeredLFPViewer::handleTTLEvent(TTLEventPtr event)
{
    for (auto source : triggerSources)
    {
        if (event->getLine() == source->line && event->getState() && source->canTrigger)
        {
            collectTriggeredData(source, event->getStreamId(), event->getSampleNumber());

            if (source->type == TTL_AND_MSG_TRIGGER)
                source->canTrigger = false;
        }
    }
}

void TriggeredLFPViewer::collectTriggeredData(LFPTriggerSource* source, uint16 streamId, int64 sampleNumber)
{
    if (dataBuffers.find(source) == dataBuffers.end())
        return;

    if (getDataStreams().size() == 0 || continuousBuffer.isEmpty())
        return;

    float sampleRate = getDataStreams()[0]->getSampleRate();
    if (sampleRate <= 0)
        return;

    int preSamples = (int)(sampleRate * getPreWindowSizeMs() / 1000);
    int postSamples = (int)(sampleRate * getPostWindowSizeMs() / 1000);
    int totalSamples = preSamples + postSamples;

    // Update buffer parameters
    dataBuffers[source]->setParameters(preSamples, postSamples, continuousBuffer.size());

    // Extract data from circular buffer
    Array<float*> channelDataPtrs;
    Array<Array<float>> extractedData;

    for (int ch = 0; ch < continuousBuffer.size(); ch++)
    {
        Array<float> channelData;
        channelData.resize(totalSamples);

        // Calculate start position in circular buffer
        int startPos = writeIndex - preSamples;
        if (startPos < 0)
            startPos += bufferSize;

        for (int sample = 0; sample < totalSamples; sample++)
        {
            int bufferPos = (startPos + sample) % bufferSize;
            channelData.set(sample, continuousBuffer[ch][bufferPos]);
        }

        extractedData.add(channelData);
        channelDataPtrs.add(extractedData.getReference(ch).getRawDataPointer());
    }

    // Add trial to data buffer
    dataBuffers[source]->addTrial(channelDataPtrs, sampleNumber);

    // Notify canvas of new data
    if (canvas != nullptr)
    {
        canvas->newDataReceived(source);
    }
}

void TriggeredLFPViewer::timerCallback()
{
    // Update editor if needed
}

void TriggeredLFPViewer::saveCustomParametersToXml(XmlElement* xml)
{
    for (auto source : triggerSources)
    {
        XmlElement* sourceXml = xml->createNewChildElement("TRIGGERSOURCE");
        sourceXml->setAttribute("name", source->name);
        sourceXml->setAttribute("line", source->line);
        sourceXml->setAttribute("type", source->type);
        sourceXml->setAttribute("colour", source->colour.toString());
    }

    // Save channel selection
    XmlElement* channelsXml = xml->createNewChildElement("CHANNELS");
    channelsXml->setAttribute("allSelected", allChannelsSelected);
    if (!allChannelsSelected)
    {
        for (int ch : selectedChannels)
        {
            XmlElement* channelXml = channelsXml->createNewChildElement("CHANNEL");
            channelXml->setAttribute("index", ch);
        }
    }
}

void TriggeredLFPViewer::loadCustomParametersFromXml(XmlElement* xml)
{
    triggerSources.clear();
    dataBuffers.clear();

    forEachXmlChildElement(*xml, sourceXml)
    {
        if (sourceXml->hasTagName("TRIGGERSOURCE"))
        {
            String name = sourceXml->getStringAttribute("name");
            int line = sourceXml->getIntAttribute("line");
            int type = sourceXml->getIntAttribute("type");
            Colour colour = Colour::fromString(sourceXml->getStringAttribute("colour"));

            LFPTriggerSource* source = addTriggerSource(line, (TriggerType)type);
            source->name = name;
            source->colour = colour;
        }
        else if (sourceXml->hasTagName("CHANNELS"))
        {
            allChannelsSelected = sourceXml->getBoolAttribute("allSelected", true);
            if (!allChannelsSelected)
            {
                selectedChannels.clear();
                forEachXmlChildElement(*sourceXml, channelXml)
                {
                    if (channelXml->hasTagName("CHANNEL"))
                    {
                        selectedChannels.add(channelXml->getIntAttribute("index"));
                    }
                }
            }
        }
    }
}