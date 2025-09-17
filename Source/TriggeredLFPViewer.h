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

#ifndef __TriggeredLFPViewer_H_3F920F95__
#define __TriggeredLFPViewer_H_3F920F95__

#include <ProcessorHeaders.h>

#include <map>
#include <vector>
#include <deque>

class TriggeredLFPViewer;

enum TriggerType
{
    TTL_TRIGGER = 1,
    MSG_TRIGGER = 2,
    TTL_AND_MSG_TRIGGER = 3
};

/** 
    Represents one trigger source for LFP data
*/
class LFPTriggerSource
{
public:
    LFPTriggerSource(TriggeredLFPViewer* processor_, String name_, int line_, TriggerType type_) 
        : processor(processor_), name(name_), line(line_), type(type_)
    {
        if (type == TTL_TRIGGER)
            canTrigger = true;
        else
            canTrigger = false;

        colour = getColourForLine(line);
    }

    static Colour getColourForLine(int line)
    {
        Array<Colour> eventColours = {
            Colour(224, 185, 36),
            Colour(243, 119, 33),
            Colour(237, 37, 36),
            Colour(217, 46, 171),
            Colour(101, 31, 255),
            Colour(48, 117, 255),
            Colour(116, 227, 156),
            Colour(82, 173, 0)
        };

        return eventColours[line % 8];
    }

    String name;
    int line;
    TriggerType type;
    TriggeredLFPViewer* processor;
    bool canTrigger;
    Colour colour;
};

/**
    Stores continuous data for triggered LFP analysis
*/
class LFPDataBuffer
{
public:
    LFPDataBuffer(int maxTrials = 10);
    ~LFPDataBuffer() {}

    void setParameters(int preSamples, int postSamples, int numChannels);
    void addTrial(const Array<float*>& channelData, int64 triggerSample);
    void clear();
    
    int getNumTrials() const { return trials.size(); }
    int getPreSamples() const { return preSamples; }
    int getPostSamples() const { return postSamples; }
    int getTotalSamples() const { return preSamples + postSamples; }
    int getNumChannels() const { return numChannels; }
    
    // Get data for a specific trial and channel
    const Array<float>& getTrialData(int trialIndex, int channelIndex) const;
    
    // Get averaged data across trials for a channel
    Array<float> getAveragedData(int channelIndex) const;
    
    // Get the most recent trial data
    const Array<float>& getLatestTrialData(int channelIndex) const;

private:
    int maxTrials;
    int preSamples;
    int postSamples;
    int numChannels;
    
    // Store trials as [trial][channel][sample]
    std::deque<Array<Array<float>>> trials;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFPDataBuffer);
};

class TriggeredLFPCanvas;

/**
    
    Collects continuous data around TTL/message triggers for oscilloscope-like display.
    Supports multiple channels, conditions, and real-time averaging.

*/

class TriggeredLFPViewer : public GenericProcessor, public Timer
{
public:
    /** Constructor */
    TriggeredLFPViewer();

    /** Destructor */
    ~TriggeredLFPViewer() {}

    /** Creates the TriggeredLFPEditor. */
    AudioProcessorEditor* createEditor() override;

    /** Used to alter parameters of data acquisition. */
    void parameterValueChanged(Parameter* param) override;

    /** Calls checkForEvents and manages continuous data collection */
    void process(AudioBuffer<float>& buffer) override;

    /** Returns the pre-trigger window size in ms */
    int getPreWindowSizeMs();

    /** Returns the post-trigger window size in ms */
    int getPostWindowSizeMs();

    /** Returns the maximum number of trials to store */
    int getMaxTrials();

    /** Pointer to the display canvas */
    TriggeredLFPCanvas* canvas;

    /** Returns an array of current trigger sources */
    Array<LFPTriggerSource*> getTriggerSources();

    /** Adds a new trigger source */
    LFPTriggerSource* addTriggerSource(int line, TriggerType type, int index = -1);

    /** Removes trigger sources */
    void removeTriggerSources(Array<LFPTriggerSource*> sources);

    /** Removes a trigger source by index*/
    void removeTriggerSource(int indexToRemove);

    /** Checks whether the source name is unique*/
    String ensureUniqueName(String name);

    /** Sets trigger source name*/
    void setTriggerSourceName(LFPTriggerSource* source, String name, bool updateEditor = true);

    /** Sets trigger source line */
    void setTriggerSourceLine(LFPTriggerSource* source, int line, bool updateEditor = true);

    /** Sets trigger source colour */
    void setTriggerSourceColour(LFPTriggerSource* source, Colour colour, bool updateEditor = true);

    /** Sets trigger source type */
    void setTriggerSourceTriggerType(LFPTriggerSource* source, TriggerType type, bool updateEditor = true);

    /** Get selected channels for display */
    Array<int> getSelectedChannels();

    /** Set selected channels for display */
    void setSelectedChannels(Array<int> channels);

    /** Clear all data for a specific trigger source */
    void clearTriggerSourceData(LFPTriggerSource* source);

    /** Clear all data */
    void clearAllData();

    /** Get data buffer for a trigger source */
    LFPDataBuffer* getDataBuffer(LFPTriggerSource* source);

    int getNextConditionIndex() { return nextConditionIndex; }

    /** Saves trigger source parameters */
    void saveCustomParametersToXml(XmlElement* xml) override;

    /** Loads trigger source parameters */
    void loadCustomParametersFromXml(XmlElement* xml) override;

private:
    /** Responds to incoming broadcast messages */
    void handleBroadcastMessage(const String& message, const int64 sysTimeMs) override;

    /** Responds to incoming configuration messages */
    String handleConfigMessage(const String& message) override;

    /** Helper method for parsing dynamic objects */
    bool getIntField(DynamicObject::Ptr payload,
                     String name,
                     int& value,
                     int lowerBound,
                     int upperBound);

    /** Pushes incoming events to trigger data collection */
    void handleTTLEvent(TTLEventPtr event) override;

    /** Updates editor after receiving config message */
    void timerCallback() override;

    /** Collects continuous data around a trigger */
    void collectTriggeredData(LFPTriggerSource* source, uint16 streamId, int64 sampleNumber);

    /** Maintains circular buffer of continuous data */
    void updateContinuousBuffer(const AudioBuffer<float>& buffer);

    OwnedArray<LFPTriggerSource> triggerSources;
    std::map<LFPTriggerSource*, std::unique_ptr<LFPDataBuffer>> dataBuffers;

    int nextConditionIndex = 1;
    LFPTriggerSource* currentTriggerSource = nullptr;

    // Continuous data circular buffer for collecting triggered windows
    Array<Array<float>> continuousBuffer; // [channel][sample]
    int bufferSize;
    int writeIndex;
    bool bufferFull;

    // Channel selection
    Array<int> selectedChannels;
    bool allChannelsSelected;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggeredLFPViewer);
};

#endif // __TriggeredLFPViewer_H_3F920F95__