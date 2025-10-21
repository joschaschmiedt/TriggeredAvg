#include "GridDisplay.h"

#include "DataCollector.h"
#include "SinglePlotPanel.h"
#include "TriggerSource.h"
#include "TriggeredAvgNode.h"

TriggeredAverage::GridDisplay::GridDisplay() = default;

void TriggeredAverage::GridDisplay::refresh()
{
    for (auto panel : panels)
    {
        panel->repaint();
    }
}

void TriggeredAverage::GridDisplay::resized()
{
    totalHeight = 0;
    const int numPlots = panels.size();
    const int leftEdge = 10;
    const int rightEdge = getWidth() - borderSize;
    const int histogramWidth = (rightEdge - leftEdge - borderSize * (numColumns - 1)) / numColumns;

    int index = -1;
    int overlayIndex = 0;
    int row = 0;
    int col = 0;
    bool drawBackground = true;

    ContinuousChannel* latestChannel = nullptr;

    for (auto panel : panels)
    {
        if (overlayConditions)
        {
            if (panel->contChannel != latestChannel)
            {
                latestChannel = const_cast<ContinuousChannel*> (panel->contChannel);
                drawBackground = true;
                index++;
                overlayIndex = 0;
            }
        }
        else
        {
            index++;
        }

        row = index / numColumns;
        col = index % numColumns;

        panel->drawBackground (drawBackground);
        panel->setBounds (leftEdge + col * (histogramWidth + borderSize),
                          row * (panelHeightPx + borderSize),
                          histogramWidth,
                          panelHeightPx);

        panel->setOverlayMode (overlayConditions);
        panel->setOverlayIndex (overlayIndex);

        if (overlayConditions)
        {
            drawBackground = false;
            overlayIndex++;
        }
    }

    totalHeight = (row + 1) * (panelHeightPx + borderSize);
}

void TriggeredAverage::GridDisplay::addContChannel (const ContinuousChannel* channel,
                                                    const TriggerSource* source,
                                                    const MultiChannelAverageBuffer* avgBuffer)
{
    auto* h = new SinglePlotPanel (this, channel, source, avgBuffer);
    h->setPlotType (plotType);

    //LOGD("Display adding ", channel->getName(), " for ", source->name);

    panels.add (h);
    triggerSourceToPanelMap[source].add (h);
    contChannelToPanelMap[channel].add (h);

    int numRows = panels.size() / numColumns + 1;

    totalHeight = (numRows + 1) * (panelHeightPx + 10);

    addAndMakeVisible (h);
}

void TriggeredAverage::GridDisplay::updateColourForSource (const TriggerSource* source)
{
    Array<SinglePlotPanel*> plotPanels = triggerSourceToPanelMap[source];

    for (auto panel : plotPanels)
    {
        panel->setSourceColour (source->colour);
    }
}

void TriggeredAverage::GridDisplay::updateConditionName (const TriggerSource* source)
{
    Array<SinglePlotPanel*> plotPanels = triggerSourceToPanelMap[source];

    for (auto panel : plotPanels)
    {
        panel->setSourceName (source->name);
    }
}

void TriggeredAverage::GridDisplay::setNumColumns (int numColumns_)
{
    numColumns = numColumns_;
    resized();
}

void TriggeredAverage::GridDisplay::setRowHeight (int height)
{
    panelHeightPx = height;
    resized();
}

void TriggeredAverage::GridDisplay::setConditionOverlay (bool overlay_)
{
    overlayConditions = overlay_;
    resized();
}

void TriggeredAverage::GridDisplay::prepareToUpdate()
{
    panels.clear();
    triggerSourceToPanelMap.clear();
    contChannelToPanelMap.clear();
    setBounds (0, 0, getWidth(), 0);
}

void TriggeredAverage::GridDisplay::setWindowSizeMs (float pre_ms_, float post_ms_)
{
    post_ms = post_ms_;

    for (auto hist : panels)
    {
        hist->setWindowSizeMs (pre_ms_, post_ms);
    }
}

void TriggeredAverage::GridDisplay::setPlotType (TriggeredAverage::DisplayMode plotType_)
{
    plotType = plotType_;

    for (auto panel : panels)
    {
        panel->setPlotType (plotType);
    }
}

void TriggeredAverage::GridDisplay::pushEvent (const TriggerSource* source,
                                               uint16 streamId,
                                               int64 sample_number)
{
    for (auto hist : triggerSourceToPanelMap[source])
    {
        if (hist->streamId == streamId)
        {
            //hist->addEvent (sample_number);
        }
    }
}

int TriggeredAverage::GridDisplay::getDesiredHeight() const { return totalHeight; }

void TriggeredAverage::GridDisplay::clearPanels()
{
    for (auto hist : panels)
    {
        hist->clear();
    }
}

DynamicObject TriggeredAverage::GridDisplay::getInfo()
{
    DynamicObject output;
    Array<var> panelInfo;

    for (auto panel : panels)
    {
        auto info = panel->getInfo().clone();
        panelInfo.add (info.get());
    }

    output.setProperty (Identifier ("panels"), panelInfo);

    return output;
}