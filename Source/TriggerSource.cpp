#include "TriggerSource.h"
#include <JuceHeader.h>

TriggeredAverage::TriggerSource::TriggerSource (TriggeredAvgNode* processor_,
                                                const juce::String& name_,
                                                int line_,
                                                TriggerType type_)
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

juce::Colour TriggeredAverage::TriggerSource::getColourForLine (int line)
{
    Array<Colour> eventColours = { Colour (224, 185, 36),  Colour (243, 119, 33),
                                   Colour (237, 37, 36),   Colour (217, 46, 171),
                                   Colour (101, 31, 255),  Colour (48, 117, 255),
                                   Colour (116, 227, 156), Colour (82, 173, 0) };

    return eventColours[line % 8];
}