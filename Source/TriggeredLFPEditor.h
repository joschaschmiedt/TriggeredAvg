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

#ifndef __TriggeredLFPEditor_H_2C4C2D67__
#define __TriggeredLFPEditor_H_2C4C2D67__

#include <EditorHeaders.h>

class TriggeredLFPViewer;
class LFPTriggerSource;
class Visualizer;

/**
    Channel selector component for LFP viewer
*/
class ChannelSelector : public Component,
                       public ComboBox::Listener,
                       public Button::Listener
{
public:
    ChannelSelector(TriggeredLFPViewer* processor);
    ~ChannelSelector() {}

    void paint(Graphics& g) override;
    void resized() override;
    
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(Button* button) override;
    
    void updateChannelList();
    void setSelectedChannels(Array<int> channels);

private:
    TriggeredLFPViewer* processor;
    
    std::unique_ptr<ComboBox> channelSelector;
    std::unique_ptr<UtilityButton> selectAllButton;
    std::unique_ptr<UtilityButton> selectNoneButton;
    
    Array<int> availableChannels;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelSelector);
};

/**
    Display settings component
*/
class DisplaySettings : public Component,
                       public ComboBox::Listener,
                       public Slider::Listener
{
public:
    DisplaySettings(TriggeredLFPViewer* processor);
    ~DisplaySettings() {}

    void paint(Graphics& g) override;
    void resized() override;
    
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;
    void sliderValueChanged(Slider* slider) override;

private:
    TriggeredLFPViewer* processor;
    
    std::unique_ptr<ComboBox> gridRowsSelector;
    std::unique_ptr<ComboBox> gridColsSelector;
    std::unique_ptr<ComboBox> displayModeSelector;
    std::unique_ptr<Slider> amplitudeScaleSlider;
    
    std::unique_ptr<Label> gridRowsLabel;
    std::unique_ptr<Label> gridColsLabel;
    std::unique_ptr<Label> displayModeLabel;
    std::unique_ptr<Label> amplitudeScaleLabel;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisplaySettings);
};

/**
    Trigger source configuration table
*/
class TriggerSourceTable : public Component,
                          public TableListBoxModel
{
public:
    TriggerSourceTable(TriggeredLFPViewer* processor);
    ~TriggerSourceTable() {}

    void paint(Graphics& g) override;
    void resized() override;

    // TableListBoxModel methods
    int getNumRows() override;
    void paintRowBackground(Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;
    void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool rowIsSelected) override;
    Component* refreshComponentForCell(int rowNumber, int columnId, bool isRowSelected, Component* existingComponentToUpdate) override;

    void updateContent();
    void addTriggerSource();
    void removeTriggerSource(int row);

private:
    TriggeredLFPViewer* processor;
    std::unique_ptr<TableListBox> table;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggerSourceTable);
};

/**

    User interface for the TriggeredLFPViewer processor.

*/

class TriggeredLFPEditor : public GenericEditor,
                          public ComboBox::Listener,
                          public Button::Listener
{

public:
    /** Constructor */
    TriggeredLFPEditor(GenericProcessor* parentNode);

    /** Destructor */
    ~TriggeredLFPEditor() {}

    /** Called when the tab becomes visible again */
    void collapsedStateChanged() override;

    /** Updates settings after a parameter value change */
    void updateSettings() override;

    /** Opens the canvas window */
    Visualizer* createNewCanvas();
    
    /** ComboBox listener callback */
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

    /** Button listener callback */
    void buttonClicked(Button* button) override;

    /** Component callback for resizing */
    void resized() override;

private:
    /** Pointer to the actual processor */
    TriggeredLFPViewer* processor;

    // Parameter controls
    std::unique_ptr<ComboBox> preWindowSelector;
    std::unique_ptr<ComboBox> postWindowSelector;
    std::unique_ptr<ComboBox> maxTrialsSelector;
    
    std::unique_ptr<Label> preWindowLabel;
    std::unique_ptr<Label> postWindowLabel;
    std::unique_ptr<Label> maxTrialsLabel;

    // Interface components
    std::unique_ptr<ChannelSelector> channelSelector;
    std::unique_ptr<DisplaySettings> displaySettings;
    std::unique_ptr<TriggerSourceTable> triggerSourceTable;
    
    // Action buttons
    std::unique_ptr<UtilityButton> addTriggerButton;
    std::unique_ptr<UtilityButton> clearDataButton;
    std::unique_ptr<UtilityButton> openCanvasButton;

    void drawParameterControls(Graphics& g);
    void updateParameterControls();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggeredLFPEditor);
};

#endif // __TriggeredLFPEditor_H_2C4C2D67__