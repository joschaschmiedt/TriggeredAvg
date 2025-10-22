#include "TriggeredAvgCanvas.h"
#include "GridDisplay.h"
#include "TimeAxis.h"
#include "TriggeredAvgNode.h"

#include <mfidl.h>
using namespace TriggeredAverage;

OptionsBar::OptionsBar (TriggeredAvgCanvas* canvas_, GridDisplay* display_, TimeAxis* timescale_)
    : display (display_),
      canvas (canvas_),
      timescale (timescale_)
{
    clearButton = std::make_unique<UtilityButton> ("CLEAR");

    clearButton->setFont (FontOptions (12.0f));
    clearButton->addListener (this);
    clearButton->setClickingTogglesState (false);
    addAndMakeVisible (clearButton.get());

    saveButton = std::make_unique<UtilityButton> ("SAVE");
    saveButton->setFont (FontOptions (12.0f));
    saveButton->addListener (this);
    saveButton->setClickingTogglesState (false);
    addAndMakeVisible (saveButton.get());

    plotTypeSelector = std::make_unique<ComboBox> ("Plot Type Selector");

    plotTypeSelector->addItemList (DisplayModeStrings, 1);

    plotTypeSelector->setSelectedId (1, dontSendNotification);
    plotTypeSelector->addListener (this);
    addAndMakeVisible (plotTypeSelector.get());

    columnNumberSelector = std::make_unique<ComboBox> ("Column Number Selector");
    for (int i = 1; i < 7; i++)
        columnNumberSelector->addItem (String (i), i);
    columnNumberSelector->setSelectedId (1, dontSendNotification);
    columnNumberSelector->addListener (this);
    addAndMakeVisible (columnNumberSelector.get());

    rowHeightSelector = std::make_unique<ComboBox> ("Row Height Selector");
    for (int i = 2; i < 6; i++)
        rowHeightSelector->addItem (String (i * 50) + " px", i * 50);
    rowHeightSelector->setSelectedId (150, dontSendNotification);
    rowHeightSelector->addListener (this);
    addAndMakeVisible (rowHeightSelector.get());

    overlayButton = std::make_unique<UtilityButton> ("OFF");
    overlayButton->setFont (FontOptions (12.0f));
    overlayButton->addListener (this);
    overlayButton->setClickingTogglesState (true);
    addAndMakeVisible (overlayButton.get());
}

void OptionsBar::buttonClicked (Button* button)
{
    if (button == clearButton.get())
    {
        display->clearPanels();
    }

    else if (button == overlayButton.get())
    {
        display->setConditionOverlay (button->getToggleState());

        if (overlayButton->getToggleState())
            overlayButton->setLabel ("ON");
        else
            overlayButton->setLabel ("OFF");

        canvas->resized();
    }
    else if (button == saveButton.get())
    {
        DynamicObject output = display->getInfo();

        FileChooser chooser ("Save histogram statistics to file...", File(), "*.json");

        if (chooser.browseForFileToSave (true))
        {
            File file = chooser.getResult();

            if (file.exists())
                file.deleteFile();

            FileOutputStream f (file);

            output.writeAsJSON (f,
                                JSON::FormatOptions {}
                                    .withIndentLevel (5)
                                    .withSpacing (JSON::Spacing::multiLine)
                                    .withMaxDecimalPlaces (4));
        }
    }
}

void OptionsBar::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == plotTypeSelector.get())
    {
        auto id = comboBox->getSelectedId();
        display->setPlotType (static_cast<DisplayMode> (comboBox->getSelectedId()));
    }
    else if (comboBox == columnNumberSelector.get())
    {
        const int numColumns = comboBox->getSelectedId();

        display->setNumColumns (numColumns);

        if (numColumns == 1)
            timescale->setVisible (true);
        else
            timescale->setVisible (false);

        canvas->resized();
    }
    else if (comboBox == rowHeightSelector.get())
    {
        display->setRowHeight (comboBox->getSelectedId());

        canvas->resized();
    }
}

void OptionsBar::resized()
{
    const int verticalOffset = 7;

    clearButton->setBounds (getWidth() - 100, verticalOffset, 70, 25);
    saveButton->setBounds (getWidth() - 180, verticalOffset, 70, 25);

    plotTypeSelector->setBounds (440, verticalOffset, 150, 25);

    rowHeightSelector->setBounds (60, verticalOffset, 80, 25);

    columnNumberSelector->setBounds (200, verticalOffset, 50, 25);

    overlayButton->setBounds (340, verticalOffset, 35, 25);
}

void OptionsBar::paint (Graphics& g)
{
    g.setColour (findColour (ThemeColours::defaultText));
    g.setFont (FontOptions ("Inter", "Regular", 15.0f));

    const int verticalOffset = 4;

    g.drawText ("Row", 0, verticalOffset, 53, 15, Justification::centredRight, false);
    g.drawText ("Height", 0, verticalOffset + 15, 53, 15, Justification::centredRight, false);
    g.drawText ("Num", 150, verticalOffset, 43, 15, Justification::centredRight, false);
    g.drawText ("Cols", 150, verticalOffset + 15, 43, 15, Justification::centredRight, false);
    g.drawText ("Overlay", 240, verticalOffset, 93, 15, Justification::centredRight, false);
    g.drawText ("Conditions", 240, verticalOffset + 15, 93, 15, Justification::centredRight, false);
    g.drawText ("Plot", 390, verticalOffset, 43, 15, Justification::centredRight, false);
    g.drawText ("Type", 390, verticalOffset + 15, 43, 15, Justification::centredRight, false);
}

void OptionsBar::saveCustomParametersToXml (XmlElement* xml) const
{
    xml->setAttribute ("plot_type", plotTypeSelector->getSelectedId());
    xml->setAttribute ("num_cols", columnNumberSelector->getSelectedId());
    xml->setAttribute ("row_height", rowHeightSelector->getSelectedId());
    xml->setAttribute ("overlay", overlayButton->getToggleState());
}

void OptionsBar::loadCustomParametersFromXml (XmlElement* xml)
{
    columnNumberSelector->setSelectedId (xml->getIntAttribute ("num_cols", 1), sendNotification);
    rowHeightSelector->setSelectedId (xml->getIntAttribute ("row_height", 150), sendNotification);
    overlayButton->setToggleState (xml->getBoolAttribute ("overlay", false), sendNotification);
    plotTypeSelector->setSelectedId (xml->getIntAttribute ("plot_type", 1), sendNotification);
}

TriggeredAvgCanvas::TriggeredAvgCanvas (TriggeredAvgNode* processor_)
    : Visualizer (processor_),
      m_dataStore (processor_->getDataStore())
{
    m_timeAxis = std::make_unique<TimeAxis>();
    addAndMakeVisible (m_timeAxis.get());

    m_mainViewport = std::make_unique<Viewport>();
    m_mainViewport->setScrollBarsShown (true, true);

    m_grid = std::make_unique<GridDisplay>();
    m_mainViewport->setViewedComponent (m_grid.get(), false);
    m_mainViewport->setScrollBarThickness (15);
    addAndMakeVisible (m_mainViewport.get());
    m_grid->setBounds (0, 50, 500, 100);

    m_optionsBarHolder = std::make_unique<Viewport>();
    m_optionsBarHolder->setScrollBarsShown (false, true);
    m_optionsBarHolder->setScrollBarThickness (10);

    m_optionsBar = std::make_unique<OptionsBar> (this, m_grid.get(), m_timeAxis.get());
    m_optionsBarHolder->setViewedComponent (m_optionsBar.get(), false);
    addAndMakeVisible (m_optionsBarHolder.get());
}

void TriggeredAvgCanvas::refreshState() { resized(); }

void TriggeredAvgCanvas::resized()
{
    const int scrollBarThickness = m_mainViewport->getScrollBarThickness();
    const int timescaleHeight = 40;
    const int optionsBarHeight = 44;

    if (m_timeAxis->isVisible())
    {
        m_timeAxis->setBounds (10, 0, getWidth() - scrollBarThickness - 150, timescaleHeight);
        m_mainViewport->setBounds (
            0, timescaleHeight, getWidth(), getHeight() - timescaleHeight - optionsBarHeight);
    }
    else
    {
        m_mainViewport->setBounds (0, 10, getWidth(), getHeight() - 10 - optionsBarHeight);
    }

    m_grid->setBounds (0, 0, getWidth() - scrollBarThickness, m_grid->getDesiredHeight());
    m_grid->resized();

    m_optionsBarHolder->setBounds (0, getHeight() - optionsBarHeight, getWidth(), optionsBarHeight);

    int optionsWidth = getWidth() < 775 ? 775 : getWidth();
    m_optionsBar->setBounds (0, 0, optionsWidth, m_optionsBarHolder->getHeight());
}

void TriggeredAvgCanvas::paint (Graphics& g)
{
    g.fillAll (Colour (0, 18, 43));

    g.setColour (findColour (ThemeColours::componentBackground));
    g.fillRect (m_optionsBarHolder->getBounds());
}

void TriggeredAvgCanvas::setWindowSizeMs (float pre_ms_, float post_ms_)
{
    pre_ms = pre_ms_;
    post_ms = post_ms_;

    m_grid->setWindowSizeMs (pre_ms, post_ms);
    m_timeAxis->setWindowSizeMs (pre_ms, post_ms);

    repaint();
}

void TriggeredAvgCanvas::addContChannel (const ContinuousChannel* channel,
                                         const TriggerSource* source,
                                         int channelIndexInAverageBuffer,
                                         const MultiChannelAverageBuffer* avgBuffer)
{
    m_grid->addContChannel (channel, source, channelIndexInAverageBuffer, avgBuffer);
}

void TriggeredAvgCanvas::updateColourForSource (const TriggerSource* source)
{
    m_grid->updateColourForSource (source);
}

void TriggeredAvgCanvas::updateConditionName (const TriggerSource* source)
{
    m_grid->updateConditionName (source);
}

void TriggeredAvgCanvas::prepareToUpdate() { m_grid->prepareToUpdate(); }

void TriggeredAvgCanvas::saveCustomParametersToXml (XmlElement* xml)
{
    m_optionsBar->saveCustomParametersToXml (xml);
}

void TriggeredAvgCanvas::loadCustomParametersFromXml (XmlElement* xml)
{
    m_optionsBar->loadCustomParametersFromXml (xml);
}
