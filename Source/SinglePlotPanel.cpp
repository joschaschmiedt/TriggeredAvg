#include "SinglePlotPanel.h"
#include "GridDisplay.h"
#include "TriggeredAvgCanvas.h"

using namespace TriggeredAverage;
const static Colour panelBackground { 30, 30, 40 };

SinglePlotPanel::SinglePlotPanel (GridDisplay* display_,
                                  const ContinuousChannel* channel,
                                  const TriggerSource* source_)
    : streamId (channel->getStreamId()),
      contChannel (channel),
      latestEventSampleNumber (0),
      baseColour (source_->colour),
      source (source_),
      display (display_),
      waitingForWindowToClose (false),
      sample_rate (channel->getSampleRate())
{
    pre_ms = 0;
    post_ms = 0;
    bin_size_ms = 10;

    infoLabel = std::make_unique<Label> ("info label");
    infoLabel->setJustificationType (Justification::topLeft);
    infoLabel->setText (channel->getName(), dontSendNotification);
    infoLabel->setColour (Label::textColourId, Colours::white);
    addAndMakeVisible (infoLabel.get());

    const auto font12pt = Font { withDefaultMetrics (FontOptions { 12.0f }) };
    const auto font16pt = Font { withDefaultMetrics (FontOptions { 16.0f }) };

    channelLabel = std::make_unique<Label> ("channel label");
    channelLabel->setFont (font12pt);
    channelLabel->setJustificationType (Justification::topLeft);
    channelLabel->setColour (Label::textColourId, Colours::white);
    String channelString = "";
    for (auto ch : channel->getName())
        channelString += ch + ", ";

    channelString = channelString.substring (0, channelString.length() - 2);
    channelLabel->setText (channelString, dontSendNotification);
    addAndMakeVisible (channelLabel.get());

    conditionLabel = std::make_unique<Label> ("condition label");
    conditionLabel->setFont (font16pt);
    conditionLabel->setJustificationType (Justification::topLeft);
    conditionLabel->setText (source->name, dontSendNotification);
    conditionLabel->setColour (Label::textColourId, baseColour);
    addAndMakeVisible (conditionLabel.get());

    hoverLabel = std::make_unique<Label> ("hover label");
    hoverLabel->setJustificationType (Justification::topLeft);
    hoverLabel->setFont (font12pt);
    hoverLabel->setColour (Label::textColourId, Colours::white);
    addAndMakeVisible (hoverLabel.get());

    trialCounter = std::make_unique<Label> ("trial counter");
    trialCounter->setFont (font12pt);
    trialCounter->setJustificationType (Justification::centredTop);
    auto trialCounterString = String ("Trials: ") + String (numTrials);
    trialCounter->setText (trialCounterString, dontSendNotification);
    trialCounter->setColour (Label::textColourId, baseColour);
    addAndMakeVisible (trialCounter.get());

    maxCounts.add (1);
    uniqueSortedIds.add (0);
    counts.add (Array<int>());
    maxSortedId = 0;

    clear();
}

void SinglePlotPanel::resized()
{
    int labelOffset;
    const int width = getWidth();

    if (width < 320)
        labelOffset = 5;
    else
        labelOffset = width - 150;

    if (labelOffset == 5)
        panelWidthPx = width - labelOffset;
    else
        panelWidthPx = labelOffset - 10;

    panelHeightPx = (getHeight() - 10);

    infoLabel->setBounds (labelOffset, 10, 150, 30);

    if (getHeight() < 100)
    {
        conditionLabel->setBounds (labelOffset, 26, 150, 30);
        channelLabel->setVisible (false);
        hoverLabel->setVisible (false);
    }
    else
    {
        conditionLabel->setBounds (labelOffset, 49, 150, 15);
        channelLabel->setVisible (! overlayMode);
        channelLabel->setBounds (labelOffset, 26, 150, 30);

        hoverLabel->setVisible (! overlayMode);
        hoverLabel->setBounds (labelOffset, 66, 150, 45);
    }

    if (labelOffset == 5)
    {
        conditionLabel->setVisible (false);
        channelLabel->setVisible (false);
        hoverLabel->setBounds (width - 120, 10, 150, 45);
    }
    else
    {
        conditionLabel->setVisible (true);
        channelLabel->setVisible (! overlayMode);

        if (overlayMode)
        {
            conditionLabel->setBounds (labelOffset, 49 + 18 * overlayIndex, 150, 15);
        }
    }
}

void SinglePlotPanel::clear()
{
    maxCounts.fill (1);

    numTrials = 0;
}

void SinglePlotPanel::addSpike (int64 sample_number, int sortedId)
{
    //const ScopedLock lock(mutex);

    //newSpikeSampleNumbers.add (sample_number);
    //newSpikeSortedIds.add (sortedId);

    //int sortedIdIndex = uniqueSortedIds.indexOf (sortedId);

    //if (sortedIdIndex < 0)
    //{
    //    sortedIdIndex = uniqueSortedIds.size();
    //    uniqueSortedIds.add (sortedId);
    //    if (sortedId > 0)
    //        unitSelector->addItem ("Unit " + String (sortedId), sortedId + 1);

    //    maxCounts.add (1);
    //    counts.add (Array<int>());
    //    maxSortedId = jmax (sortedId, maxSortedId);
    //}
}

void SinglePlotPanel::addEvent (int64 sample_number)
{
    numTrials++;
    latestEventSampleNumber = sample_number;
}

void SinglePlotPanel::setWindowSizeMs (float pre, float post)
{
    pre_ms = pre;
    post_ms = post;
}

void SinglePlotPanel::setPlotType (TriggeredAverage::DisplayMode plotType)
{
    switch (plotType)
    {
        case DisplayMode::INDIVIDUAL_TRACES:
            plotAverage = false;
            plotAllTraces = true;
            break;
        case DisplayMode::AVERAGE_TRAGE:
            plotAverage = true;
            plotAllTraces = false;
            break;
        case DisplayMode::ALL_AND_AVERAGE:
            plotAverage = true;
            plotAllTraces = true;
            break;
        default:
            plotAverage = true;
            plotAllTraces = false;
            break;
    }

    repaint();
}

void SinglePlotPanel::setSourceColour (Colour colour)
{
    baseColour = colour;
    conditionLabel->setColour (Label::textColourId, baseColour);
    repaint();
}

void SinglePlotPanel::setSourceName (const String& name) const
{
    conditionLabel->setText (name, dontSendNotification);
}

void SinglePlotPanel::drawBackground (bool shouldDraw)
{
    shouldDrawBackground = shouldDraw;

    infoLabel->setVisible (shouldDrawBackground);
}

void SinglePlotPanel::setOverlayMode (bool shouldOverlay)
{
    overlayMode = shouldOverlay;

    maxCounts.fill (1);

}

void SinglePlotPanel::setOverlayIndex (int index)
{
    overlayIndex = index;

    resized();
}

void SinglePlotPanel::update() { numTrials++; }

void SinglePlotPanel::paint (Graphics& g)
{

    if (shouldDrawBackground)
        g.fillAll (panelBackground);

    if (plotAverage)
    {
        // TODO: draw average
        g.drawLine (0.0f, 0.0f, 10.0f, 10.0f, 2.0f);
    }

    if (plotAllTraces)
    {
        // TODO: draw all traces
        g.drawLine (-1.0f, 2.0f, 3.0f, -1.0f, 0.5f);
    }

    float zeroLoc = (pre_ms) / (pre_ms + post_ms) * static_cast<float> (panelWidthPx);

    auto trialCounterString =  String (numTrials);
    trialCounter->setText (trialCounterString, dontSendNotification);
    g.setColour (Colours::white);
    g.drawLine (zeroLoc, 0, zeroLoc, static_cast<float> (getHeight()), 2.0);
}

void SinglePlotPanel::mouseMove (const MouseEvent& event)
{
    if (event.getPosition().x < panelWidthPx)
    {
        String hoverString = "TODO";
        hoverLabel->setText (hoverString, dontSendNotification);

        repaint();
    }
}

void SinglePlotPanel::mouseExit (const MouseEvent& event)
{
    hoverLabel->setText ("", dontSendNotification);
    repaint();
}

void SinglePlotPanel::comboBoxChanged (ComboBox* comboBox)
{
    if (overlayMode)
    {
        //display->setUnitForElectrode (spikeChannel,
        //                              uniqueSortedIds[comboBox->getSelectedItemIndex()]);
    }
    else
    {
        repaint();
    }
}

void SinglePlotPanel::setMaxCount (int unitId, int count)
{
    const int sortedIdIndex = uniqueSortedIds.indexOf (unitId);

    if (maxCounts[sortedIdIndex] < count)
    {
        maxCounts.set (sortedIdIndex, count);
        repaint();
    }
}

DynamicObject SinglePlotPanel::getInfo() const
{
    DynamicObject info;

    info.setProperty (Identifier ("channel"), var (contChannel->getName()));
    info.setProperty (Identifier ("condition"), var (source->name));
    info.setProperty (Identifier ("color"), var (source->colour.toString()));
    info.setProperty (Identifier ("trial_count"), var (int (numTrials)));

    return info;
}
