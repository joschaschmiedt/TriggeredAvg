#pragma once

#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
enum class DisplayMode : std::uint8_t;
class GridDisplay;
class TriggerSource;

class SinglePlotPanel : public Component,
                        public Timer,
                        public ComboBox::Listener
{
public:
    SinglePlotPanel (TriggeredAverage::GridDisplay*, const ContinuousChannel* channel, const TriggerSource* triggerSource);

    void paint (Graphics& g) override;
    void resized() override;

    // TODO: remove
    void addSpike (int64 sample_number, int sortedId);
    void setUnitId (int unitId);

    void addEvent (int64 sample_number);

    void clear();
    void setWindowSizeMs (float pre_ms, float post_ms);
    void setPlotType (TriggeredAverage::DisplayMode plotType);
    void setSourceColour (Colour colour);

    void setSourceName (const String& name) const;
    void setMaxCount (int unitId, int count);
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

    // TODO: remove?
    void timerCallback();
    void recount (bool full = true);

    std::unique_ptr<Label> infoLabel;
    std::unique_ptr<Label> channelLabel;
    std::unique_ptr<Label> conditionLabel;
    std::unique_ptr<Label> hoverLabel;
    std::unique_ptr<ComboBox> unitSelector;


    // TODO: remove
    Array<int64> newSpikeSampleNumbers;
    Array<int> newSpikeSortedIds;
    int64 latestEventSampleNumber;
    Array<int> uniqueSortedIds;
    Array<double> binEdges;


    bool plotAllTraces = true;
    bool plotAverage = true;
    int maxSortedId = 0;
    int maxRasterTrials = 30;

    // TODO: remove
    Array<double> relativeTimes;
    Array<int> relativeTimeTrialIndices;
    Array<int> relativeTimeSortedIds;


    Colour baseColour;

    Array<Array<int>> counts;

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
    Array<int> maxCounts;
    bool waitingForWindowToClose;
    size_t numTrials = 0;
    int currentUnitId = 0;
    const double sample_rate;
};
} // namespace TriggeredAverage
