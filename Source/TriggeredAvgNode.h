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
#pragma once

#include "MultiChannelRingBuffer.h"

#include <ProcessorHeaders.h>
#include <atomic>
#include <deque>
#include <map>
#include <memory>
#include <vector>

namespace TriggeredAverage
{

class ContTrialData;
class TriggeredAvgNode;

enum class TriggerType : std::int_fast8_t
{
    TTL_TRIGGER = 1,
    MSG_TRIGGER = 2,
    TTL_AND_MSG_TRIGGER = 3
};

constexpr auto TriggerTypeToString (TriggerType type)
{
    switch (type)
    {
        case TriggerType::TTL_TRIGGER:
            return "TTL Trigger";
        case TriggerType::MSG_TRIGGER:
            return "Message Trigger";
        case TriggerType::TTL_AND_MSG_TRIGGER:
            return "TTL and Message Trigger";
        default:
            return "Unknown Trigger Type";
    }
}

/** 
    Represents one trigger source for LFP data
*/
class TriggerSource
{
public:
    TriggerSource (TriggeredAvgNode* processor_, String name_, int line_, TriggerType type_)
        : name (name_),
          line (line_),
          type (type_),
          processor (processor_)
    {
        if (type == TriggerType::TTL_TRIGGER)
            canTrigger = true;
        else
            canTrigger = false;

        colour = getColourForLine (line);
    }

    static Colour getColourForLine (int line)
    {
        Array<Colour> eventColours = { Colour (224, 185, 36),  Colour (243, 119, 33),
                                       Colour (237, 37, 36),   Colour (217, 46, 171),
                                       Colour (101, 31, 255),  Colour (48, 117, 255),
                                       Colour (116, 227, 156), Colour (82, 173, 0) };

        return eventColours[line % 8];
    }

    String name;
    int line;
    TriggerType type;
    TriggeredAvgNode* processor;
    bool canTrigger;
    Colour colour;
};


/**
    Thread for real-time trigger detection
*/
class TriggerDetector : public Thread
{
public:
    TriggerDetector (TriggeredAvgNode* viewer, MultiChannelRingBuffer* buffer);
    ~TriggerDetector() override;

    /** Register TTL trigger event */
    void registerTTLTrigger (int line, bool state, int64 sampleNumber, uint16 streamId);

    /** Register message trigger */
    void registerMessageTrigger (const String& message, int64 sampleNumber);

    /** Thread run function */
    void run() override;

private:
    struct TriggerEvent
    {
        enum Type
        {
            TTL,
            MESSAGE
        };
        Type type;
        int line;
        bool state;
        String message;
        int64 sampleNumber;
        uint16 streamId;

        TriggerEvent (int l, bool s, int64 sn, uint16 si)
            : type (TTL),
              line (l),
              state (s),
              sampleNumber (sn),
              streamId (si)
        {
        }
        TriggerEvent (const String& msg, int64 sn)
            : type (MESSAGE),
              message (msg),
              sampleNumber (sn)
        {
        }
    };

    TriggeredAvgNode* viewer;
    MultiChannelRingBuffer* ringBuffer;

    CriticalSection triggerQueueLock;
    std::deque<TriggerEvent> triggerQueue;
    WaitableEvent newTriggerEvent;

    /** Check if trigger event matches any trigger sources */
    void processTriggerEvent (const TriggerEvent& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggerDetector);
};

/**
    Thread for managing triggered data capture
*/
class CaptureManager : public Thread
{
public:
    CaptureManager (MultiChannelRingBuffer* buffer);
    ~CaptureManager();

    /** Request triggered data capture */
    void requestCapture (TriggerSource* source,
                         int64 triggerSample,
                         int preSamples,
                         int postSamples,
                         Array<int> channelIndices);

    /** Thread run function */
    void run() override;

    /** Get completed captures for UI */
    bool getCompletedCapture (TriggerSource*& source,
                              AudioBuffer<float>& data,
                              int64& triggerSample);
    bool getCompletedTrialData (std::unique_ptr<ContTrialData>& trial);

private:
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

    struct CompletedCapture
    {
        TriggerSource* source;
        AudioBuffer<float> data;
        int64 triggerSample;

        CompletedCapture (TriggerSource* s, int64 ts, int numChannels, int numSamples)
            : source (s),
              data (numChannels, numSamples),
              triggerSample (ts)
        {
        }
    };

    MultiChannelRingBuffer* ringBuffer;

    CriticalSection requestQueueLock;
    std::deque<CaptureRequest> captureRequests;
    WaitableEvent newRequestEvent;

    CriticalSection completedQueueLock;
    std::deque<std::unique_ptr<CompletedCapture>> completedCaptures;

    /** Process a single capture request */
    void processCaptureRequest (const CaptureRequest& request);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CaptureManager);
};

/**
    Stores completed triggered data for display
*/
class ContTrialData
{
public:
    ContTrialData (int numChannels, int numSamples, int64 triggerSample);
    ~ContTrialData() {}

    const AudioBuffer<float>& getData() const { return data; }
    int64 getTriggerSample() const { return triggerSample; }

    /** Get downsampled data for display */
    void getDownsampledData (AudioBuffer<float>& output, int targetSamples) const;

private:
    AudioBuffer<float> data;
    int64 triggerSample;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContTrialData);
};

/**
    Container for storing multiple trials per trigger source
*/
class ContTrialBuffer
{
public:
    ContTrialBuffer (int maxTrials = 10);
    ~ContTrialBuffer();

    /** Add new trial data */
    void addTrial (std::unique_ptr<ContTrialData> trial);

    /** Get trial by index */
    const ContTrialData* getTrial (int index) const;

    /** Get number of stored trials */
    size_t getNumTrials() const { return trials.size(); }

    /** Get averaged data across all trials */
    void getAveragedData (AudioBuffer<float>& output) const;

    /** Clear all trials */
    void clear();

private:
    std::deque<std::unique_ptr<ContTrialData>> trials;
    int maxTrials;

    CriticalSection trialsLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ContTrialBuffer);
};

class TriggeredAvgCanvas;

/**
    
    Collects continuous data around TTL/message triggers for oscilloscope-like display.
    Uses a streamlined multi-threaded architecture with direct ring buffer writes 
    from the real-time audio thread.

*/
class TriggeredAvgNode : public GenericProcessor, public ::Timer
{
public:
    /** Constructor */
    TriggeredAvgNode();

    ~TriggeredAvgNode() override;

    AudioProcessorEditor* createEditor() override;

    /** Used to alter parameters of data acquisition. */
    void parameterValueChanged (Parameter* param) override;

    /** Calls checkForEvents */
    void process (AudioBuffer<float>& buffer) override;

    float getPreWindowSizeMs() const;
    float getPostWindowSizeMs() const;
    int getMaxTrials() const { return (int) getParameter ("max_trials")->getValue(); }

    /** Returns an array of current trigger sources */
    Array<TriggerSource*> getTriggerSources();

    /** Adds a new trigger source */
    TriggerSource* addTriggerSource (int line, TriggerType type, int index = -1);

    /** Removes trigger sources */
    void removeTriggerSources (Array<TriggerSource*> sources);

    /** Removes a trigger source by index*/
    void removeTriggerSource (int indexToRemove);

    /** Checks whether the source name is unique*/
    String ensureUniqueName (String name);

    /** Sets trigger source name*/
    void setTriggerSourceName (TriggerSource* source, String name, bool updateEditor = true);

    /** Sets trigger source line */
    void setTriggerSourceLine (TriggerSource* source, int line, bool updateEditor = true);

    /** Sets trigger source colour */
    void setTriggerSourceColour (TriggerSource* source, Colour colour, bool updateEditor = true);

    /** Sets trigger source type */
    void setTriggerSourceTriggerType (TriggerSource* source,
                                      TriggerType type,
                                      bool updateEditor = true);

    int getNextConditionIndex() { return nextConditionIndex; }

    /** Saves trigger source parameters */
    void saveCustomParametersToXml (XmlElement* xml) override;

    /** Saves trigger source parameters */
    void loadCustomParametersFromXml (XmlElement* xml) override;
    TriggeredAvgCanvas* canvas;

private:
    /** Responds to incoming broadcast messages */
    void handleBroadcastMessage (const String& message, const int64 sysTimeMs) override;

    /** Responds to incoming configuration messages */
    String handleConfigMessage (const String& message) override;

    /** Helper method for parsing dynamic objects */
    bool getIntField (DynamicObject::Ptr payload,
                      String name,
                      int& value,
                      int lowerBound,
                      int upperBound);

    /** Pushes incoming events to trigger detector */
    void handleTTLEvent (TTLEventPtr event) override;

    /** Updates UI with completed captures */
    void timerCallback() override;

    /** Initialize threading components */
    void initializeThreads();

    /** Shutdown threading components */
    void shutdownThreads();

    std::unique_ptr<MultiChannelRingBuffer> ringBuffer;
    std::unique_ptr<TriggerDetector> triggerDetector;
    std::unique_ptr<CaptureManager> captureManager;

    // Trigger sources and trial data
    OwnedArray<TriggerSource> triggerSources;
    std::map<TriggerSource*, std::unique_ptr<ContTrialBuffer>> trialBuffers;

    int nextConditionIndex = 1;
    TriggerSource* currentTriggerSource = nullptr;

    // Channel selection
    Array<int> selectedChannels;
    bool allChannelsSelected;

    // Buffer parameters
    int ringBufferSize;
    std::atomic<bool> threadsInitialized;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggeredAvgNode)
};

} // namespace TriggeredAverage