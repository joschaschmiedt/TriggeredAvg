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

#include "TriggeredLFPEditor.h"
#include "TriggeredLFPViewer.h"
#include "TriggeredLFPCanvas.h"

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
    // Handle channel selection
}

void ChannelSelector::buttonClicked(Button* button)
{
    if (button == selectAllButton.get())
    {
        // Select all channels
    }
    else if (button == selectNoneButton.get())
    {
        // Select no channels  
    }
}

void ChannelSelector::updateChannelList()
{
    // Update available channels
}

void ChannelSelector::setSelectedChannels(Array<int> channels)
{
    // Set selected channels
}

// DisplaySettings implementation
DisplaySettings::DisplaySettings(TriggeredLFPViewer* processor_) : processor(processor_)
{
    gridRowsLabel = std::make_unique<Label>("Rows Label", "Rows:");
    addAndMakeVisible(gridRowsLabel.get());
    
    gridRowsSelector = std::make_unique<ComboBox>("Grid Rows");
    for (int i = 1; i <= 6; i++)
        gridRowsSelector->addItem(String(i), i);
    gridRowsSelector->setSelectedId(2);
    gridRowsSelector->addListener(this);
    addAndMakeVisible(gridRowsSelector.get());

    gridColsLabel = std::make_unique<Label>("Cols Label", "Cols:");
    addAndMakeVisible(gridColsLabel.get());
    
    gridColsSelector = std::make_unique<ComboBox>("Grid Cols");
    for (int i = 1; i <= 4; i++)
        gridColsSelector->addItem(String(i), i);
    gridColsSelector->setSelectedId(2);
    gridColsSelector->addListener(this);
    addAndMakeVisible(gridColsSelector.get());

    displayModeLabel = std::make_unique<Label>("Mode Label", "Mode:");
    addAndMakeVisible(displayModeLabel.get());
    
    displayModeSelector = std::make_unique<ComboBox>("Display Mode");
    displayModeSelector->addItem("Individual", 1);
    displayModeSelector->addItem("Average", 2);
    displayModeSelector->addItem("Overlay", 3);
    displayModeSelector->addItem("Both", 4);
    displayModeSelector->setSelectedId(1);
    displayModeSelector->addListener(this);
    addAndMakeVisible(displayModeSelector.get());

    amplitudeScaleLabel = std::make_unique<Label>("Amplitude Label", "Scale:");
    addAndMakeVisible(amplitudeScaleLabel.get());
    
    amplitudeScaleSlider = std::make_unique<Slider>("Amplitude Scale");
    amplitudeScaleSlider->setRange(0.1, 10.0, 0.1);
    amplitudeScaleSlider->setValue(1.0);
    amplitudeScaleSlider->addListener(this);
    addAndMakeVisible(amplitudeScaleSlider.get());
}

void DisplaySettings::paint(Graphics& g)
{
    g.setColour(Colours::darkgrey);
    g.fillRect(getLocalBounds());
}

void DisplaySettings::resized()
{
    int yPos = 10;
    int spacing = 25;
    
    gridRowsLabel->setBounds(5, yPos, 40, 20);
    gridRowsSelector->setBounds(50, yPos, 60, 20);
    
    yPos += spacing;
    gridColsLabel->setBounds(5, yPos, 40, 20);
    gridColsSelector->setBounds(50, yPos, 60, 20);
    
    yPos += spacing;
    displayModeLabel->setBounds(5, yPos, 40, 20);
    displayModeSelector->setBounds(50, yPos, 80, 20);
    
    yPos += spacing;
    amplitudeScaleLabel->setBounds(5, yPos, 40, 20);
    amplitudeScaleSlider->setBounds(50, yPos, 80, 20);
}

void DisplaySettings::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    // Handle combo box changes
}

void DisplaySettings::sliderValueChanged(Slider* slider)
{
    // Handle slider changes
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

// TriggeredLFPEditor implementation
TriggeredLFPEditor::TriggeredLFPEditor(GenericProcessor* parentNode) 
    : VisualizerEditor(parentNode, "LFP", 300)
{
    processor = static_cast<TriggeredLFPViewer*>(parentNode);

    // Remove the desiredWidth setting as VisualizerEditor handles sizing

    // Parameter controls
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

    // Interface components
    channelSelector = std::make_unique<ChannelSelector>(processor);
    addAndMakeVisible(channelSelector.get());

    displaySettings = std::make_unique<DisplaySettings>(processor);
    addAndMakeVisible(displaySettings.get());

    triggerSourceTable = std::make_unique<TriggerSourceTable>(processor);
    addAndMakeVisible(triggerSourceTable.get());

    // Action buttons
    addTriggerButton = std::make_unique<UtilityButton>("Add Trigger");
    addTriggerButton->addListener(this);
    addAndMakeVisible(addTriggerButton.get());

    clearDataButton = std::make_unique<UtilityButton>("Clear Data");
    clearDataButton->addListener(this);
    addAndMakeVisible(clearDataButton.get());

    // Remove the openCanvasButton since VisualizerEditor handles canvas opening automatically
}

void TriggeredLFPEditor::collapsedStateChanged()
{
    triggerSourceTable->updateContent();
}

void TriggeredLFPEditor::updateSettings()
{
    updateParameterControls();
}

void TriggeredLFPEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == preWindowSelector.get())
    {
        processor->getParameter("pre_ms")->setNextValue(preWindowSelector->getSelectedId());
    }
    else if (comboBoxThatHasChanged == postWindowSelector.get())
    {
        processor->getParameter("post_ms")->setNextValue(postWindowSelector->getSelectedId());
    }
    else if (comboBoxThatHasChanged == maxTrialsSelector.get())
    {
        processor->getParameter("max_trials")->setNextValue(maxTrialsSelector->getSelectedId());
    }
}

void TriggeredLFPEditor::buttonClicked(Button* button)
{
    if (button == addTriggerButton.get())
    {
        triggerSourceTable->addTriggerSource();
    }
    else if (button == clearDataButton.get())
    {
        processor->clearAllData();
    }
    // Removed openCanvasButton handling - VisualizerEditor handles canvas opening automatically
}

Visualizer* TriggeredLFPEditor::createNewCanvas()
{
    TriggeredLFPViewer* processor = static_cast<TriggeredLFPViewer*>(getProcessor());
    
    auto canvas = new TriggeredLFPCanvas(processor);
    processor->canvas = canvas;
    
    return canvas;
}

void TriggeredLFPEditor::drawParameterControls(Graphics& g)
{
    // Draw background for parameter controls
}

void TriggeredLFPEditor::updateParameterControls()
{
    // Update parameter control values
}

void TriggeredLFPEditor::resized()
{
    int yPos = 30;
    int spacing = 25;
    int leftMargin = 10;
    int rightMargin = 10;
    int availableWidth = getWidth() - leftMargin - rightMargin;

    // Parameter controls
    preWindowLabel->setBounds(leftMargin, yPos, 60, 20);
    preWindowSelector->setBounds(leftMargin + 65, yPos, 80, 20);
    yPos += spacing;

    postWindowLabel->setBounds(leftMargin, yPos, 60, 20);
    postWindowSelector->setBounds(leftMargin + 65, yPos, 80, 20);
    yPos += spacing;

    maxTrialsLabel->setBounds(leftMargin, yPos, 60, 20);
    maxTrialsSelector->setBounds(leftMargin + 65, yPos, 80, 20);
    yPos += spacing + 10;

    // Channel selector
    channelSelector->setBounds(leftMargin, yPos, availableWidth, 70);
    yPos += 75;

    // Display settings
    displaySettings->setBounds(leftMargin, yPos, availableWidth, 120);
    yPos += 125;

    // Trigger source table
    triggerSourceTable->setBounds(leftMargin, yPos, availableWidth, 100);
    yPos += 105;

    // Action buttons
    addTriggerButton->setBounds(leftMargin, yPos, 80, 20);
    clearDataButton->setBounds(leftMargin + 85, yPos, 80, 20);
    // Removed openCanvasButton bounds - VisualizerEditor handles canvas opening automatically
}