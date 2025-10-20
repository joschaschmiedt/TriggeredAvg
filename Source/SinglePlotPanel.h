#pragma once

#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
enum class DisplayMode : std::uint8_t;
class GridDisplay;
class TriggerSource;

class SinglePlotPanel : public Component, public ComboBox::Listener
{
public:
    SinglePlotPanel (TriggeredAverage::GridDisplay*,
                     const ContinuousChannel*,
                     const TriggerSource*);

    void paint (Graphics& g) override;
    void resized() override;

    void clear();
    void setWindowSizeMs (float pre_ms, float post_ms);
    void setPlotType (TriggeredAverage::DisplayMode plotType);
    void setSourceColour (Colour colour);

    void setSourceName (const String& name) const;
    void drawBackground (bool);
    void setOverlayMode (bool);
    void setOverlayIndex (int index);
    void mouseMove (const MouseEvent& event);
    void mouseExit (const MouseEvent& event);
    void comboBoxChanged (ComboBox* comboBox) override;
    void update();

    uint16 streamId;
    const ContinuousChannel* contChannel;
    DynamicObject getInfo() const;

private:
    std::unique_ptr<Label> infoLabel;
    std::unique_ptr<Label> channelLabel;
    std::unique_ptr<Label> conditionLabel;
    std::unique_ptr<Label> hoverLabel;
    std::unique_ptr<Label> trialCounter;

    bool plotAllTraces = true;
    bool plotAverage = true;
    int maxSortedId = 0;

    Colour baseColour;


    const TriggerSource* source;
    TriggeredAverage::GridDisplay* display;

    float pre_ms;
    float post_ms;
    int bin_size_ms;
    int panelWidthPx;
    int panelHeightPx;
    bool shouldDrawBackground = true;
    int overlayIndex = 0;
    bool overlayMode = false;
    bool waitingForWindowToClose;
    size_t numTrials = 0;
    const double sample_rate;
};
} // namespace TriggeredAverage
