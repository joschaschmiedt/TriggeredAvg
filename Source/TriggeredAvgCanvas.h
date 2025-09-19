// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "TimeAxis.h"
#include "TriggeredAvgNode.h"
#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
class TriggerSource;
class TriggeredAvgCanvas;
class GridDisplay;

enum class DisplayMode : std::uint8_t
{
    INDIVIDUAL_TRACES = 0,
    AVERAGE_TRAGE = 1,
    ALL_AND_AVERAGE = 2,
    NUMBER_OF_MODES = 3,
};

constexpr auto DisplayModeModeToString (DisplayMode mode) -> const char*
{
    using enum DisplayMode;
    switch (mode)
    {
        case INDIVIDUAL_TRACES:
            return "All traces";
        case AVERAGE_TRAGE:
            return "Average trace";
        case ALL_AND_AVERAGE:
            return "Average + All";
        default:
            return "Unknown";
    }
}

static const auto DisplayModeStrings = StringArray { "All traces", "Average trace", "Average + All" };

class OptionsBar : public Component,
                   public Button::Listener,
                   public ComboBox::Listener
{
public:
    OptionsBar (TriggeredAvgCanvas*, GridDisplay*, TimeAxis*);
    OptionsBar() = delete;
    OptionsBar (const OptionsBar&) = delete;
    OptionsBar& operator= (const OptionsBar&) = delete;
    ~OptionsBar() override = default;

    void buttonClicked (Button* button) override;
    void comboBoxChanged (ComboBox* comboBox) override;
    void resized();
    void paint (Graphics& g);
    void saveCustomParametersToXml (XmlElement* xml);
    void loadCustomParametersFromXml (XmlElement* xml);

private:
    std::unique_ptr<UtilityButton> clearButton;
    std::unique_ptr<UtilityButton> saveButton;

    std::unique_ptr<ComboBox> plotTypeSelector;

    std::unique_ptr<ComboBox> columnNumberSelector;
    std::unique_ptr<ComboBox> rowHeightSelector;
    std::unique_ptr<UtilityButton> overlayButton;

    GridDisplay* display;
    TriggeredAvgCanvas* canvas;
    TimeAxis* timescale;
};

class TriggeredAvgCanvas : public Visualizer
{
public:
    TriggeredAvgCanvas (TriggeredAvgNode* processor);
    ~TriggeredAvgCanvas() override = default;

    /** Renders the Visualizer on each animation callback cycle
        Called instead of Juce's "repaint()" to avoid redrawing underlying components
        if not necessary.*/
    void refresh() override {}
    /** Called when the Visualizer's tab becomes visible after being hidden .*/
    void refreshState() override;

    /** Called when the Visualizer is first created, and optionally when
        the parameters of the underlying processor are changed. */
    void updateSettings() override {}

    /** Called when the component changes size */
    void resized() override;

    /** Renders component background */
    void paint (Graphics& g) override;

    /** Sets the overall window size*/
    void setWindowSizeMs (float pre_ms, float post_ms);


    /** Add an event to the queue */
    void pushEvent (const TriggerSource* source, uint16 streamId, int64 sample_number);

    /** Add a spike to the queue */
    //void pushSpike (const SpikeChannel* channel, int64 sample_number, int sortedId);

    /** Adds a spike channel */
    void addContChannel (const ContinuousChannel* channel, const TriggerSource* source);

    /** Changes source colour */
    void updateColourForSource (const TriggerSource* source);

    /** Changes source name */
    void updateConditionName (const TriggerSource* source);

    /** Prepare for update*/
    void prepareToUpdate();

    /** Save plot type*/
    void saveCustomParametersToXml (XmlElement* xml) override;

    /** Load plot type*/
    void loadCustomParametersFromXml (XmlElement* xml) override;

private:
    float pre_ms;
    float post_ms;

    std::unique_ptr<Viewport> viewport;

    std::unique_ptr<TimeAxis> scale;
    std::unique_ptr<GridDisplay> display;

    std::unique_ptr<Viewport> optionsBarHolder;
    std::unique_ptr<OptionsBar> optionsBar;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggeredAvgCanvas)
};
} // namespace TriggeredAverage
