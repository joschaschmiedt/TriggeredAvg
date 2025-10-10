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
    TriggeredAvgNode* processor;
    bool canTrigger;
    juce::Colour colour;
};
} // namespace TriggeredAverage
