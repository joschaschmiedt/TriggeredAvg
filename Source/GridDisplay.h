#pragma once
#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
enum class DisplayMode : std::uint8_t;
class SinglePlotPanel;
class TriggerSource;

// GUI Component that holds the grid of triggered average panels
class GridDisplay : public Component
{
public:
    GridDisplay();

    /** Renders the Visualizer on each animation callback cycle
        Called instead of Juce's "repaint()" to avoid redrawing underlying components
        if not necessary.*/
    void refresh();

    void resized() override;
    void setWindowSizeMs (float pre_ms, float post_ms);
    void setPlotType (TriggeredAverage::DisplayMode plotType);
    void pushEvent (const TriggerSource* source, uint16 streamId, int64 sample_number);

    void addContChannel (const ContinuousChannel* channel, const TriggerSource* source);

    void updateColourForSource (const TriggerSource* source);
    void updateConditionName (const TriggerSource* source);
    void setNumColumns (int numColumns);
    void setRowHeight (int rowHeightPixels);

    void setConditionOverlay (bool);
    void prepareToUpdate();
    int getDesiredHeight() const;
    void clearPanels();

    DynamicObject getInfo();

private:
    OwnedArray<SinglePlotPanel> panels;

    std::map<const TriggerSource*, Array<SinglePlotPanel*>> triggerSourceToPanelMap;
    std::map<const ContinuousChannel*, Array<SinglePlotPanel*>> contChannelToPanelMap;

    int totalHeight = 0;
    int panelHeightPx = 150;
    int borderSize = 10;
    int numColumns = 1;

    bool overlayConditions = false;

    float post_ms;
    DisplayMode plotType;
};

} // namespace TriggeredAverage
