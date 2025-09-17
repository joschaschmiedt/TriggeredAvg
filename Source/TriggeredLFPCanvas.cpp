/*
    ------------------------------------------------------------------

    This file is part of the Open Ephys GUI
    Copyright (C) 2025 Open Ephys

    ------------------------------------------------------------------

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "TriggeredLFPCanvas.h"
#include "TriggeredLFPViewer.h"

// ChannelSelector implementation
ChannelSelector::ChannelSelector(TriggeredLFPViewer* processor_) : processor(processor_)
{
    channelSelector = std::make_unique<ComboBox>("Channel Selector");
    channelSelector->addListener(this);
    addAndMakeVisible(channelSelector.get());

    selectAllButton = std::make_unique<UtilityButton>("All");
    selectAllButton->addListener(this);
    addAndMakeVisible(selectAllButton.get());

    selectNoneButton = std::make_unique<UtilityButton>("None");
    selectNoneButton->addListener(this);
    addAndMakeVisible(selectNoneButton.get());
}

void ChannelSelector::paint(Graphics& g)
{
    g.setColour(Colours::darkgrey);
    g.fillRect(getLocalBounds());
    
    g.setColour(Colours::white);
    g.setFont(12.0f);
    g.drawText("Channels:", 5, 5, 80, 20, Justification::left);
}

void ChannelSelector::resized()
{
    int yPos = 25;
    selectAllButton->setBounds(5, yPos, 40, 20);
    selectNoneButton->setBounds(50, yPos, 40, 20);
    channelSelector->setBounds(5, yPos + 25, getWidth() - 10, 20);
}

void ChannelSelector::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == channelSelector.get() && processor != nullptr)
    {
        int selectedId = channelSelector->getSelectedId();
        
        if (selectedId == 1) // "All Channels" selected
        {
            Array<int> allChannels;
            for (int i = 0; i < availableChannels.size(); i++)
                allChannels.add(availableChannels[i]);
            processor->setSelectedChannels(allChannels);
        }
        else if (selectedId > 1) // Individual channel selected
        {
            Array<int> singleChannel;
            singleChannel.add(selectedId - 2); // Convert back to 0-based index
            processor->setSelectedChannels(singleChannel);
        }
    }
}

void ChannelSelector::buttonClicked(Button* button)
{
    if (processor == nullptr)
        return;
        
    if (button == selectAllButton.get())
    {
        // Select all channels
        Array<int> allChannels;
        for (int i = 0; i < availableChannels.size(); i++)
            allChannels.add(availableChannels[i]);
        processor->setSelectedChannels(allChannels);
        channelSelector->setSelectedId(1); // Set to "All Channels"
    }
    else if (button == selectNoneButton.get())
    {
        // Select no channels
        Array<int> noChannels;
        processor->setSelectedChannels(noChannels);
        channelSelector->setSelectedId(0); // Deselect all
    }
}

void ChannelSelector::updateChannelList()
{
    channelSelector->clear();
    availableChannels.clear();
    
    if (processor == nullptr)
        return;
    
    // Get the number of available input channels
    int numChannels = processor->getNumInputs();
    
    if (numChannels == 0)
    {
        channelSelector->addItem("No channels available", 1);
        channelSelector->setEnabled(false);
        return;
    }
    
    channelSelector->setEnabled(true);
    
    // Add "All Channels" option
    channelSelector->addItem("All Channels", 1);
    
    // Add individual channels
    for (int i = 0; i < numChannels; i++)
    {
        String channelName = "Channel " + String(i + 1);
        channelSelector->addItem(channelName, i + 2); // Start from ID 2
        availableChannels.add(i);
    }
    
    // Set default selection to "All Channels"
    channelSelector->setSelectedId(1);
    
    // Update processor with all channels selected by default
    Array<int> allChannels;
    for (int i = 0; i < numChannels; i++)
        allChannels.add(i);
    processor->setSelectedChannels(allChannels);
}

void ChannelSelector::setSelectedChannels(Array<int> channels)
{
    if (channels.isEmpty())
    {
        channelSelector->setSelectedId(0); // No selection
        return;
    }
    
    // Check if all available channels are selected
    bool allSelected = true;
    for (int i = 0; i < availableChannels.size(); i++)
    {
        if (!channels.contains(availableChannels[i]))
        {
            allSelected = false;
            break;
        }
    }
    
    if (allSelected && channels.size() == availableChannels.size())
    {
        channelSelector->setSelectedId(1); // "All Channels"
    }
    else if (channels.size() == 1)
    {
        // Single channel selected
        int channelIndex = channels[0];
        channelSelector->setSelectedId(channelIndex + 2); // Convert to ComboBox ID
    }
    else
    {
        // Multiple channels selected, show "All Channels" for simplicity
        channelSelector->setSelectedId(1);
    }
}

// TriggerSourceTable implementation
TriggerSourceTable::TriggerSourceTable(TriggeredLFPViewer* processor_) : processor(processor_)
{
    table = std::make_unique<TableListBox>("Trigger Sources", this);
    table->getHeader().addColumn("Name", 1, 100);
    table->getHeader().addColumn("Line", 2, 50);
    table->getHeader().addColumn("Type", 3, 80);
    table->setMultipleSelectionEnabled(false);
    addAndMakeVisible(table.get());
}

void TriggerSourceTable::paint(Graphics& g)
{
    g.setColour(Colours::darkgrey);
    g.fillRect(getLocalBounds());
}

void TriggerSourceTable::resized()
{
    table->setBounds(getLocalBounds());
}

int TriggerSourceTable::getNumRows()
{
    return processor->getTriggerSources().size();
}

void TriggerSourceTable::paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected)
{
    if (rowIsSelected)
        g.fillAll(Colours::lightblue);
    else
        g.fillAll(Colours::white);
}

void TriggerSourceTable::paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected)
{
    g.setColour(Colours::black);
    g.setFont(12.0f);
    
    auto sources = processor->getTriggerSources();
    if (rowNumber < sources.size())
    {
        auto source = sources[rowNumber];
        String text;
        
        switch (columnId)
        {
            case 1: text = source->name; break;
            case 2: text = String(source->line); break;
            case 3: text = source->type == TTL_TRIGGER ? "TTL" : 
                          (source->type == MSG_TRIGGER ? "MSG" : "TTL+MSG"); break;
        }
        
        g.drawText(text, 5, 0, width - 10, height, Justification::left);
    }
}

Component* TriggerSourceTable::refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate)
{
    return nullptr;
}

void TriggerSourceTable::updateContent()
{
    table->updateContent();
}

void TriggerSourceTable::addTriggerSource()
{
    processor->addTriggerSource(0, TTL_TRIGGER);
    updateContent();
}

void TriggerSourceTable::removeTriggerSource(int row)
{
    processor->removeTriggerSource(row);
    updateContent();
}

// LFPOptionsBar implementation
LFPOptionsBar::LFPOptionsBar(TriggeredLFPCanvas* canvas_, TriggeredLFPDisplay* display_)
    : canvas(canvas_), display(display_)
{
    clearButton = std::make_unique<UtilityButton>("CLEAR");
    clearButton->addListener(this);
    addAndMakeVisible(clearButton.get());

    saveButton = std::make_unique<UtilityButton>("SAVE");
    saveButton->addListener(this);
    addAndMakeVisible(saveButton.get());

    autoScaleButton = std::make_unique<UtilityButton>("AUTO");
    autoScaleButton->addListener(this);
    addAndMakeVisible(autoScaleButton.get());

    // Parameter controls moved from editor
    preWindowLabel = std::make_unique<Label>("Pre Window Label", "Pre (ms):");
    addAndMakeVisible(preWindowLabel.get());

    preWindowSelector = std::make_unique<ComboBox>("Pre Window");
    preWindowSelector->addItem("100", 100);
    preWindowSelector->addItem("250", 250);
    preWindowSelector->addItem("500", 500);
    preWindowSelector->addItem("1000", 1000);
    preWindowSelector->setSelectedId(500);
    preWindowSelector->addListener(this);
    addAndMakeVisible(preWindowSelector.get());

    postWindowLabel = std::make_unique<Label>("Post Window Label", "Post (ms):");
    addAndMakeVisible(postWindowLabel.get());

    postWindowSelector = std::make_unique<ComboBox>("Post Window");
    postWindowSelector->addItem("500", 500);
    postWindowSelector->addItem("1000", 1000);
    postWindowSelector->addItem("2000", 2000);
    postWindowSelector->addItem("5000", 5000);
    postWindowSelector->setSelectedId(2000);
    postWindowSelector->addListener(this);
    addAndMakeVisible(postWindowSelector.get());

    maxTrialsLabel = std::make_unique<Label>("Max Trials Label", "Max Trials:");
    addAndMakeVisible(maxTrialsLabel.get());

    maxTrialsSelector = std::make_unique<ComboBox>("Max Trials");
    maxTrialsSelector->addItem("5", 5);
    maxTrialsSelector->addItem("10", 10);
    maxTrialsSelector->addItem("20", 20);
    maxTrialsSelector->addItem("50", 50);
    maxTrialsSelector->setSelectedId(10);
    maxTrialsSelector->addListener(this);
    addAndMakeVisible(maxTrialsSelector.get());

    // Action button moved from editor
    addTriggerButton = std::make_unique<UtilityButton>("Add Trigger");
    addTriggerButton->addListener(this);
    addAndMakeVisible(addTriggerButton.get());

    displayModeSelector = std::make_unique<ComboBox>("Display Mode");
    displayModeSelector->addItem("Individual", INDIVIDUAL_TRACES);
    displayModeSelector->addItem("Average", AVERAGED_TRACES);
    displayModeSelector->addItem("Overlay", OVERLAY_MODE);
    displayModeSelector->addItem("Both", BOTH_MODES);
    displayModeSelector->setSelectedId(INDIVIDUAL_TRACES);
    displayModeSelector->addListener(this);
    addAndMakeVisible(displayModeSelector.get());

    gridRowsSelector = std::make_unique<ComboBox>("Grid Rows");
    for (int i = 1; i <= 6; i++)
        gridRowsSelector->addItem(String(i), i);
    gridRowsSelector->setSelectedId(2);
    gridRowsSelector->addListener(this);
    addAndMakeVisible(gridRowsSelector.get());

    gridColsSelector = std::make_unique<ComboBox>("Grid Cols");
    for (int i = 1; i <= 4; i++)
        gridColsSelector->addItem(String(i), i);
    gridColsSelector->setSelectedId(2);
    gridColsSelector->addListener(this);
    addAndMakeVisible(gridColsSelector.get());

    amplitudeScaleSlider = std::make_unique<Slider>("Amplitude Scale");
    amplitudeScaleSlider->setRange(0.1, 10.0, 0.1);
    amplitudeScaleSlider->setValue(1.0);
    amplitudeScaleSlider->addListener(this);
    addAndMakeVisible(amplitudeScaleSlider.get());

    timeScaleSlider = std::make_unique<Slider>("Time Scale");
    timeScaleSlider->setRange(0.5, 5.0, 0.1);
    timeScaleSlider->setValue(1.0);
    timeScaleSlider->addListener(this);
    addAndMakeVisible(timeScaleSlider.get());

    // Labels
    displayModeLabel = std::make_unique<Label>("Display Mode Label", "Mode:");
    addAndMakeVisible(displayModeLabel.get());

    gridSizeLabel = std::make_unique<Label>("Grid Size Label", "Grid:");
    addAndMakeVisible(gridSizeLabel.get());

    amplitudeLabel = std::make_unique<Label>("Amplitude Label", "Amp:");
    addAndMakeVisible(amplitudeLabel.get());

    timeLabel = std::make_unique<Label>("Time Label", "Time:");
    addAndMakeVisible(timeLabel.get());
}

void LFPOptionsBar::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
}

void LFPOptionsBar::resized()
{
    int xPos = 10;
    int spacing = 5;
    int buttonWidth = 60;
    int comboWidth = 80;
    int sliderWidth = 100;
    int yPos1 = 5;  // First row
    int yPos2 = 30; // Second row

    // First row - buttons and basic display controls
    clearButton->setBounds(xPos, yPos1, buttonWidth, 20);
    xPos += buttonWidth + spacing;

    saveButton->setBounds(xPos, yPos1, buttonWidth, 20);
    xPos += buttonWidth + spacing;

    autoScaleButton->setBounds(xPos, yPos1, buttonWidth, 20);
    xPos += buttonWidth + spacing * 2;

    displayModeLabel->setBounds(xPos, yPos1, 40, 20);
    xPos += 45;
    displayModeSelector->setBounds(xPos, yPos1, comboWidth, 20);
    xPos += comboWidth + spacing;

    gridSizeLabel->setBounds(xPos, yPos1, 35, 20);
    xPos += 40;
    gridRowsSelector->setBounds(xPos, yPos1, 40, 20);
    xPos += 45;
    gridColsSelector->setBounds(xPos, yPos1, 40, 20);
    xPos += 45 + spacing;

    amplitudeLabel->setBounds(xPos, yPos1, 35, 20);
    xPos += 40;
    amplitudeScaleSlider->setBounds(xPos, yPos1, sliderWidth, 20);
    xPos += sliderWidth + spacing;

    timeLabel->setBounds(xPos, yPos1, 35, 20);
    xPos += 40;
    timeScaleSlider->setBounds(xPos, yPos1, sliderWidth, 20);

    // Second row - parameter controls moved from editor
    xPos = 10;
    preWindowLabel->setBounds(xPos, yPos2, 60, 20);
    xPos += 65;
    preWindowSelector->setBounds(xPos, yPos2, comboWidth, 20);
    xPos += comboWidth + spacing;

    postWindowLabel->setBounds(xPos, yPos2, 60, 20);
    xPos += 65;
    postWindowSelector->setBounds(xPos, yPos2, comboWidth, 20);
    xPos += comboWidth + spacing;

    maxTrialsLabel->setBounds(xPos, yPos2, 70, 20);
    xPos += 75;
    maxTrialsSelector->setBounds(xPos, yPos2, comboWidth, 20);
    xPos += comboWidth + spacing * 2;

    addTriggerButton->setBounds(xPos, yPos2, buttonWidth + 20, 20);
}

void LFPOptionsBar::buttonClicked(Button* button)
{
    if (button == clearButton.get())
    {
        display->clear();
    }
    else if (button == saveButton.get())
    {
        display->saveToFile();
    }
    else if (button == autoScaleButton.get())
    {
        display->setAutoScale(true);
    }
    else if (button == addTriggerButton.get())
    {
        // Get processor through canvas
        auto processor = canvas->getProcessor();
        processor->addTriggerSource(0, TTL_TRIGGER);
        canvas->getTriggerSourceTable()->updateContent();
    }
}

void LFPOptionsBar::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == displayModeSelector.get())
    {
        display->setDisplayMode(static_cast<DisplayMode>(displayModeSelector->getSelectedId()));
    }
    else if (comboBoxThatHasChanged == gridRowsSelector.get() || 
             comboBoxThatHasChanged == gridColsSelector.get())
    {
        display->setGridSize(gridRowsSelector->getSelectedId(), gridColsSelector->getSelectedId());
    }
    else if (comboBoxThatHasChanged == preWindowSelector.get())
    {
        auto processor = canvas->getProcessor();
        processor->getParameter("pre_ms")->setNextValue(preWindowSelector->getSelectedId());
    }
    else if (comboBoxThatHasChanged == postWindowSelector.get())
    {
        auto processor = canvas->getProcessor();
        processor->getParameter("post_ms")->setNextValue(postWindowSelector->getSelectedId());
    }
    else if (comboBoxThatHasChanged == maxTrialsSelector.get())
    {
        auto processor = canvas->getProcessor();
        processor->getParameter("max_trials")->setNextValue(maxTrialsSelector->getSelectedId());
    }
}

void LFPOptionsBar::sliderValueChanged(Slider* slider)
{
    if (slider == amplitudeScaleSlider.get())
    {
        display->setAmplitudeScale((float)slider->getValue());
    }
    else if (slider == timeScaleSlider.get())
    {
        display->setTimeScale((float)slider->getValue());
    }
}

// LFPPlot implementation
LFPPlot::LFPPlot(TriggeredLFPDisplay* display_, int plotIndex_)
    : display(display_), plotIndex(plotIndex_), triggerSource(nullptr), channelIndex(-1),
      displayMode(INDIVIDUAL_TRACES), amplitudeScale(1.0f), timeScale(1.0f), 
      showGrid(true), yOffset(0.0f), yRange(1.0f), autoScale(true)
{
}

void LFPPlot::paint(Graphics& g)
{
    g.fillAll(Colours::white);
    g.setColour(Colours::black);
    g.drawRect(getLocalBounds(), 1);

    if (triggerSource == nullptr || channelIndex < 0)
    {
        g.setColour(Colours::grey);
        g.setFont(12.0f);
        g.drawText("No data", getLocalBounds(), Justification::centred);
        return;
    }

    if (showGrid)
        drawGrid(g);

    drawAxes(g);

    // Draw traces based on display mode
    if (displayMode == INDIVIDUAL_TRACES || displayMode == BOTH_MODES)
    {
        for (int i = 0; i < individualTraces.size(); ++i)
        {
            drawTrace(g, individualTraces[i], triggerSource->colour, 0.3f);
        }
    }

    if (displayMode == AVERAGED_TRACES || displayMode == BOTH_MODES)
    {
        if (averagedTrace.size() > 0)
        {
            drawTrace(g, averagedTrace, triggerSource->colour, 1.0f);
        }
    }

    drawLabels(g);
}

void LFPPlot::resized()
{
    calculateDisplayRange();
}

void LFPPlot::mouseDown(const MouseEvent& event)
{
    // Handle mouse interactions
}

void LFPPlot::mouseDrag(const MouseEvent& event)
{
    // Handle dragging for zoom/pan
}

void LFPPlot::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
    // Handle wheel zoom
}

void LFPPlot::setTriggerSource(LFPTriggerSource* source)
{
    triggerSource = source;
    updateData();
}

void LFPPlot::setChannel(int channelIndex_)
{
    channelIndex = channelIndex_;
    updateData();
}

void LFPPlot::setDisplayMode(DisplayMode mode)
{
    displayMode = mode;
    repaint();
}

void LFPPlot::setAmplitudeScale(float scale)
{
    amplitudeScale = scale;
    repaint();
}

void LFPPlot::setTimeScale(float scale)
{
    timeScale = scale;
    repaint();
}

void LFPPlot::setShowGrid(bool show)
{
    showGrid = show;
    repaint();
}

void LFPPlot::updateData()
{
    if (triggerSource == nullptr || channelIndex < 0)
        return;

    auto dataBuffer = display->getProcessor()->getDataBuffer(triggerSource);
    if (dataBuffer == nullptr)
        return;

    // Update individual traces
    individualTraces.clear();
    for (int trial = 0; trial < dataBuffer->getNumTrials(); ++trial)
    {
        individualTraces.add(dataBuffer->getTrialData(trial, channelIndex));
    }

    // Update averaged trace
    averagedTrace = dataBuffer->getAveragedData(channelIndex);

    // Update time axis
    timeAxis.clear();
    int totalSamples = dataBuffer->getTotalSamples();
    
    if (display->getProcessor()->getDataStreams().size() > 0)
    {
        float sampleRate = display->getProcessor()->getDataStreams()[0]->getSampleRate();
        float preTimeMs = dataBuffer->getPreSamples() / sampleRate * 1000.0f;
        
        for (int i = 0; i < totalSamples; ++i)
        {
            float timeMs = -preTimeMs + (i / sampleRate * 1000.0f);
            timeAxis.add(timeMs);
        }
    }

    repaint();
}

void LFPPlot::clear()
{
    individualTraces.clear();
    averagedTrace.clear();
    timeAxis.clear();
    repaint();
}

void LFPPlot::drawTrace(Graphics& g, const Array<float>& trace, Colour colour, float alpha)
{
    if (trace.size() < 2 || timeAxis.size() != trace.size())
        return;

    Path tracePath;
    bool firstPoint = true;

    for (int i = 0; i < trace.size(); ++i)
    {
        float x = jmap(timeAxis[i], timeAxis[0], timeAxis.getLast(), 0.0f, (float)getWidth());
        float y = jmap(trace[i] * amplitudeScale, -yRange, yRange, (float)getHeight(), 0.0f);

        if (firstPoint)
        {
            tracePath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            tracePath.lineTo(x, y);
        }
    }

    g.setColour(colour.withAlpha(alpha));
    g.strokePath(tracePath, PathStrokeType(1.0f));
}

void LFPPlot::drawGrid(Graphics& g)
{
    g.setColour(Colours::lightgrey);
    
    // Vertical grid lines
    for (int i = 1; i < 10; ++i)
    {
        float x = i * getWidth() / 10.0f;
        g.drawLine(x, 0, x, getHeight());
    }

    // Horizontal grid lines
    for (int i = 1; i < 5; ++i)
    {
        float y = i * getHeight() / 5.0f;
        g.drawLine(0, y, getWidth(), y);
    }
}

void LFPPlot::drawAxes(Graphics& g)
{
    g.setColour(Colours::black);
    
    // Draw axes
    g.drawLine(0, getHeight() / 2, getWidth(), getHeight() / 2); // X-axis
    g.drawLine(getWidth() / 4, 0, getWidth() / 4, getHeight()); // Y-axis (at trigger time)
}

void LFPPlot::drawLabels(Graphics& g)
{
    g.setColour(Colours::black);
    g.setFont(10.0f);

    if (triggerSource != nullptr)
    {
        String label = triggerSource->name;
        if (channelIndex >= 0)
            label += " Ch" + String(channelIndex);
        
        g.drawText(label, 5, 5, getWidth() - 10, 15, Justification::left);
    }
}

void LFPPlot::calculateDisplayRange()
{
    // Calculate appropriate Y range for display
    float maxVal = 0.1f;
    
    for (const auto& trace : individualTraces)
    {
        for (float val : trace)
        {
            maxVal = jmax(maxVal, std::abs(val));
        }
    }
    
    if (averagedTrace.size() > 0)
    {
        for (float val : averagedTrace)
        {
            maxVal = jmax(maxVal, std::abs(val));
        }
    }
    
    yRange = maxVal * 1.2f; // Add 20% margin
}

// TriggeredLFPDisplay implementation
TriggeredLFPDisplay::TriggeredLFPDisplay(TriggeredLFPCanvas* canvas_, TriggeredLFPViewer* processor_)
    : canvas(canvas_), processor(processor_), gridRows(2), gridCols(2), maxPlots(4),
      preWindowMs(500), postWindowMs(2000), displayMode(INDIVIDUAL_TRACES),
      amplitudeScale(1.0f), timeScale(1.0f), autoScale(true), showGrid(true)
{
    setupPlotGrid();
}

void TriggeredLFPDisplay::paint(Graphics& g)
{
    g.fillAll(Colours::black);
}

void TriggeredLFPDisplay::resized()
{
    redistributePlots();
}

void TriggeredLFPDisplay::setWindowSizeMs(int preSizeMs, int postSizeMs)
{
    preWindowMs = preSizeMs;
    postWindowMs = postSizeMs;
    updateAllPlots();
}

void TriggeredLFPDisplay::setGridSize(int rows, int cols)
{
    gridRows = rows;
    gridCols = cols;
    maxPlots = rows * cols;
    setupPlotGrid();
    redistributePlots();
}

void TriggeredLFPDisplay::setDisplayMode(DisplayMode mode)
{
    displayMode = mode;
    for (auto plot : plots)
    {
        plot->setDisplayMode(mode);
    }
}

void TriggeredLFPDisplay::setAmplitudeScale(float scale)
{
    amplitudeScale = scale;
    for (auto plot : plots)
    {
        plot->setAmplitudeScale(scale);
    }
}

void TriggeredLFPDisplay::setTimeScale(float scale)
{
    timeScale = scale;
    for (auto plot : plots)
    {
        plot->setTimeScale(scale);
    }
}

void TriggeredLFPDisplay::setAutoScale(bool autoScale_)
{
    autoScale = autoScale_;
    // Implement auto-scaling logic
}

void TriggeredLFPDisplay::setShowGrid(bool show)
{
    showGrid = show;
    for (auto plot : plots)
    {
        plot->setShowGrid(show);
    }
}

void TriggeredLFPDisplay::clear()
{
    for (auto plot : plots)
    {
        plot->clear();
    }
}

void TriggeredLFPDisplay::updateTriggerSourceData(LFPTriggerSource* source)
{
    for (auto plot : plots)
    {
        if (plot->getTriggerSource() == source)
        {
            plot->updateData();
        }
    }
}

void TriggeredLFPDisplay::updateAllPlots()
{
    for (auto plot : plots)
    {
        plot->updateData();
    }
}

void TriggeredLFPDisplay::saveToFile()
{
    // Implement save functionality
}

DynamicObject TriggeredLFPDisplay::getInfo()
{
    DynamicObject info;
    // Return display information
    return info;
}

void TriggeredLFPDisplay::setupPlotGrid()
{
    plots.clear();
    
    for (int i = 0; i < maxPlots; ++i)
    {
        auto plot = new LFPPlot(this, i);
        plots.add(plot);
        addAndMakeVisible(plot);
    }
}

void TriggeredLFPDisplay::redistributePlots()
{
    if (plots.isEmpty())
        return;

    int plotWidth = getWidth() / gridCols;
    int plotHeight = getHeight() / gridRows;

    for (int i = 0; i < plots.size(); ++i)
    {
        int row = i / gridCols;
        int col = i % gridCols;
        
        int x = col * plotWidth;
        int y = row * plotHeight;
        
        plots[i]->setBounds(x, y, plotWidth, plotHeight);
        plots[i]->setVisible(i < maxPlots);
    }
}

LFPPlot* TriggeredLFPDisplay::getPlotForSourceAndChannel(LFPTriggerSource* source, int channel)
{
    // Find or assign a plot for this source/channel combination
    for (auto plot : plots)
    {
        if (plot->getTriggerSource() == source && plot->getChannel() == channel)
        {
            return plot;
        }
    }
    
    // Find an empty plot
    for (auto plot : plots)
    {
        if (plot->getTriggerSource() == nullptr)
        {
            plot->setTriggerSource(source);
            plot->setChannel(channel);
            return plot;
        }
    }
    
    return nullptr;
}

// TriggeredLFPCanvas implementation
TriggeredLFPCanvas::TriggeredLFPCanvas(TriggeredLFPViewer* processor_)
    : Visualizer(processor_), processor(processor_), acquisitionIsActive(false)
{
    processor->canvas = this;

    display = std::make_unique<TriggeredLFPDisplay>(this, processor);
    addAndMakeVisible(display.get());

    optionsBar = std::make_unique<LFPOptionsBar>(this, display.get());
    addAndMakeVisible(optionsBar.get());

    channelSelector = std::make_unique<ChannelSelector>(processor);
    addAndMakeVisible(channelSelector.get());

    triggerSourceTable = std::make_unique<TriggerSourceTable>(processor);
    addAndMakeVisible(triggerSourceTable.get());

    Timer::startTimer(50); // 20 Hz refresh rate
}

void TriggeredLFPCanvas::paint(Graphics& g)
{
    g.fillAll(Colours::darkgrey);
}

void TriggeredLFPCanvas::resized()
{
    int optionsHeight = 60;  // Increased height for two rows of controls
    int sidebarWidth = 250;  // Width for channel selector and trigger table
    
    optionsBar->setBounds(0, 0, getWidth(), optionsHeight);
    
    // Left sidebar for channel selector and trigger source table
    int remainingHeight = getHeight() - optionsHeight;
    int halfHeight = remainingHeight / 2;
    
    channelSelector->setBounds(0, optionsHeight, sidebarWidth, halfHeight);
    triggerSourceTable->setBounds(0, optionsHeight + halfHeight, sidebarWidth, halfHeight);
    
    // Main display area takes the remaining space
    display->setBounds(sidebarWidth, optionsHeight, getWidth() - sidebarWidth, remainingHeight);
}

void TriggeredLFPCanvas::refreshState()
{
    channelSelector->updateChannelList();
    display->updateAllPlots();
}

void TriggeredLFPCanvas::beginAnimation()
{
    acquisitionIsActive = true;
    channelSelector->updateChannelList();
}

void TriggeredLFPCanvas::endAnimation()
{
    acquisitionIsActive = false;
}

void TriggeredLFPCanvas::refresh()
{
    channelSelector->updateChannelList();
    display->updateAllPlots();
}

void TriggeredLFPCanvas::setWindowSizeMs(int preSizeMs, int postSizeMs)
{
    display->setWindowSizeMs(preSizeMs, postSizeMs);
}

void TriggeredLFPCanvas::newDataReceived(LFPTriggerSource* source)
{
    if (acquisitionIsActive)
    {
        display->updateTriggerSourceData(source);
    }
}

void TriggeredLFPCanvas::timerCallback()
{
    if (acquisitionIsActive)
    {
        // Periodic updates during acquisition
        display->updateAllPlots();
    }
}