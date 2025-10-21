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

using StreamId = std::uint16_t;
class TriggeredAvgNode;
class DataCollector;
class MultiChannelRingBuffer;
class TriggerSource;
class DataStore;
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

class TriggeredAvgNode : public GenericProcessor, public juce::AsyncUpdater
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
    int getNumberOfPreSamples() const
    {
        const float sampleRate = getDataStreams()[m_dataStreamIndex]->getSampleRate();
        const int preSamples = static_cast<int> (sampleRate * (getPreWindowSizeMs() / 1000.0f));
        return preSamples;
    }
    int getNumberOfPostSamplesIncludingTrigger() const
    {
        const float sampleRate = getDataStreams()[m_dataStreamIndex]->getSampleRate();
        const int postSamples = static_cast<int> (sampleRate * (getPostWindowSizeMs() / 1000.0f));
        return postSamples;
    }
    int getNumberOfSamples() const
    {
        const float sampleRate = getDataStreams()[m_dataStreamIndex]->getSampleRate();
        const int preSamples = static_cast<int> (sampleRate * (getPreWindowSizeMs() / 1000.0f));
        const int postSamples = static_cast<int> (sampleRate * (getPostWindowSizeMs() / 1000.0f));
        const int totalSamples = preSamples + postSamples;
        return totalSamples;
    }
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

    DataStore* getDataStore() { return m_dataStore.get(); }

    void setCanvas (TriggeredAvgCanvas* canvas) { m_canvas = canvas; }

    String ensureUniqueTriggerSourceName (String name);
    int getNextConditionIndex() const { return m_nextConditionIndex; }

    /** Saves trigger source parameters */
    void saveCustomParametersToXml (XmlElement* xml) override;

    /** Saves trigger source parameters */
    void loadCustomParametersFromXml (XmlElement* xml) override;

    // TODO: should we use this?
    StreamId m_dataStreamIndex = 0;

private:
    void handleBroadcastMessage (const String& message, const int64 sysTimeMs) override;
    String handleConfigMessage (const String& message) override;

    /** Helper method for parsing dynamic objects */
    bool getIntField (DynamicObject::Ptr payload,
                      String name,
                      int& value,
                      int lowerBound,
                      int upperBound);

    void handleTTLEvent (TTLEventPtr event) override;
    void handleAsyncUpdate() override;
    ;

    void initializeThreads();
    void shutdownThreads();

    std::unique_ptr<DataStore> m_dataStore;
    std::unique_ptr<MultiChannelRingBuffer> m_ringBuffer;
    std::unique_ptr<DataCollector> m_dataCollector;
    TriggeredAvgCanvas* m_canvas;

    OwnedArray<TriggerSource> m_triggerSources;
    int m_nextConditionIndex = 1;
    TriggerSource* m_currentTriggerSource = nullptr;

    // Buffer parameters
    int m_ringBufferSize;
    std::atomic<bool> m_threadsInitialized;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggeredAvgNode)
};

} // namespace TriggeredAverage