#pragma once
#include <JuceHeader.h>
#include <cstdint>
namespace TriggeredAverage
{
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
class TriggeredAvgNode;
class TriggerSource
{
public:
    TriggerSource (TriggeredAvgNode* processor_,
                   const juce::String& name_,
                   int line_,
                   TriggerType type_);

    static juce::Colour getColourForLine (int line);

    juce::String name;
    int line;
    TriggerType type;
    bool canTrigger;
    juce::Colour colour;
    TriggeredAvgNode* processor;
};

// Container class for managing multiple TriggerSource objects
class TriggerSources
{
public:
    juce::Array<TriggerSource*> getAll();
    TriggerSource* addTriggerSource (int line, TriggerType type, int index = -1);
    TriggerSource* getLastAddedTriggerSource() const { return m_currentTriggerSource; }
    void removeTriggerSources (Array<TriggerSource*> sources);
    void removeTriggerSource (int indexToRemove);

    void setTriggerSourceName (TriggerSource* source, String name, bool updateEditor = true);
    static void setTriggerSourceLine (TriggerSource* source, int line, bool updateEditor = true);

    /** Sets trigger source colour */
    static void
        setTriggerSourceColour (TriggerSource* source, Colour colour, bool updateEditor = true);

    /** Sets trigger source type */
    static void setTriggerSourceTriggerType (TriggerSource* source,
                                             TriggerType type,
                                             bool updateEditor = true);
    String ensureUniqueTriggerSourceName (String name);
    int getNextConditionIndex() const { return m_nextConditionIndex; }
    void clear() { m_triggerSources.clear(); }
    size_t size() const { return m_triggerSources.size(); }

private:
    OwnedArray<TriggerSource> m_triggerSources;
    int m_nextConditionIndex = 1;
    TriggerSource* m_currentTriggerSource = nullptr;
};

} // namespace TriggeredAverage
