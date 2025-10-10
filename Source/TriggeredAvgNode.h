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

#include <ProcessorHeaders.h>
#include <atomic>
#include <memory>

namespace TriggeredAverage
{

class TriggeredAvgNode;
class DataCollector;
class MultiChannelRingBuffer;
class TriggerSource;
enum class TriggerType : std::int_fast8_t;
class TriggeredAvgCanvas;

namespace ParameterNames
{
    constexpr auto pre_ms = "pre_ms";
    constexpr auto post_ms = "post_ms";
    constexpr auto max_trials = "max_trials";
    constexpr auto trigger_line = "trigger_line";
    constexpr auto trigger_type = "trigger_type";

} // namespace ParameterNames

class TriggeredAvgNode : public GenericProcessor, public ::Timer
{
public:
    TriggeredAvgNode();
    ~TriggeredAvgNode() override;

    // overrides
    AudioProcessorEditor* createEditor() override;
    void parameterValueChanged (Parameter* param) override;
    void process (AudioBuffer<float>& buffer) override;
    void prepareToPlay (double sampleRate, int maximumExpectedSamplesPerBlock) override;

    // parameters
    float getPreWindowSizeMs() const;
    float getPostWindowSizeMs() const;
    int getMaxTrials() const { return (int) getParameter (ParameterNames::max_trials)->getValue(); }

    // trigger sources
    Array<TriggerSource*> getTriggerSources();
    TriggerSource* addTriggerSource (int line, TriggerType type, int index = -1);
    void removeTriggerSources (Array<TriggerSource*> sources);
    void removeTriggerSource (int indexToRemove);

    void setTriggerSourceName (TriggerSource* source, String name, bool updateEditor = true);
    void setTriggerSourceLine (TriggerSource* source, int line, bool updateEditor = true);

    /** Sets trigger source colour */
    void setTriggerSourceColour (TriggerSource* source, Colour colour, bool updateEditor = true);

    /** Sets trigger source type */
    void setTriggerSourceTriggerType (TriggerSource* source,
                                      TriggerType type,
                                      bool updateEditor = true);

    String ensureUniqueTriggerSourceName (String name);
    int getNextConditionIndex() const { return nextConditionIndex; }

    /** Saves trigger source parameters */
    void saveCustomParametersToXml (XmlElement* xml) override;

    /** Saves trigger source parameters */
    void loadCustomParametersFromXml (XmlElement* xml) override;
    TriggeredAvgCanvas* canvas;

    // TODO: should we use this?
    int m_dataStreamIndex = 0;

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
    std::unique_ptr<DataCollector> dataCollector;

    OwnedArray<TriggerSource> triggerSources;
    int nextConditionIndex = 1;
    TriggerSource* currentTriggerSource = nullptr;

    // Buffer parameters
    int ringBufferSize;
    std::atomic<bool> threadsInitialized;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggeredAvgNode)
};

} // namespace TriggeredAverage