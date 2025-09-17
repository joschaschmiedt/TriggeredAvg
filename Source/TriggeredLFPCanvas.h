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

#ifndef TriggeredLFPCANVAS_H_
#define TriggeredLFPCANVAS_H_

#include <VisualizerWindowHeaders.h>

#include "TriggeredLFPViewer.h"

class LFPTriggerSource;
class TriggeredLFPCanvas;
class TriggeredLFPDisplay;

enum DisplayMode
{
    INDIVIDUAL_TRACES = 1,
    AVERAGED_TRACES = 2,
    OVERLAY_MODE = 3,
    BOTH_MODES = 4
};

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
    Options bar for controlling display parameters and trigger settings
*/
class LFPOptionsBar : public Component,
                     public Button::Listener,
                     public ComboBox::Listener,
                     public Slider::Listener
{
public:
    /** Constructor */
    LFPOptionsBar(TriggeredLFPCanvas* canvas, TriggeredLFPDisplay* display);

    /** Destructor */
    ~LFPOptionsBar() {}

    /** Component callback for rendering the background */
    void paint(Graphics& g) override;

    /** Component callback for resizing */
    void resized() override;

    /** Button listener callback */
    void buttonClicked(Button* button) override;

    /** ComboBox listener callback */
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

    /** Slider listener callback */
    void sliderValueChanged(Slider* slider) override;

private:
    TriggeredLFPCanvas* canvas;
    TriggeredLFPDisplay* display;

    // Original display controls
    std::unique_ptr<UtilityButton> clearButton;
    std::unique_ptr<UtilityButton> saveButton;
    std::unique_ptr<UtilityButton> autoScaleButton;
    
    std::unique_ptr<ComboBox> displayModeSelector;
    std::unique_ptr<ComboBox> gridRowsSelector;
    std::unique_ptr<ComboBox> gridColsSelector;
    
    std::unique_ptr<Slider> amplitudeScaleSlider;
    std::unique_ptr<Slider> timeScaleSlider;
    
    std::unique_ptr<Label> displayModeLabel;
    std::unique_ptr<Label> gridSizeLabel;
    std::unique_ptr<Label> amplitudeLabel;
    std::unique_ptr<Label> timeLabel;

    // Parameter controls moved from editor
    std::unique_ptr<ComboBox> preWindowSelector;
    std::unique_ptr<ComboBox> postWindowSelector;
    std::unique_ptr<ComboBox> maxTrialsSelector;
    
    std::unique_ptr<Label> preWindowLabel;
    std::unique_ptr<Label> postWindowLabel;
    std::unique_ptr<Label> maxTrialsLabel;

    // Action buttons moved from editor
    std::unique_ptr<UtilityButton> addTriggerButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFPOptionsBar);
};

/**
    Individual plot component for displaying LFP traces
*/
class LFPPlot : public Component
{
public:
    LFPPlot(TriggeredLFPDisplay* display, int plotIndex);
    ~LFPPlot() {}

    void paint(Graphics& g) override;
    void resized() override;
    void mouseDown(const MouseEvent& event) override;
    void mouseDrag(const MouseEvent& event) override;
    void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

    void setTriggerSource(LFPTriggerSource* source);
    void setChannel(int channelIndex);
    void setDisplayMode(DisplayMode mode);
    void setAmplitudeScale(float scale);
    void setTimeScale(float scale);
    void setShowGrid(bool show);
    
    void updateData();
    void clear();

    LFPTriggerSource* getTriggerSource() const { return triggerSource; }
    int getChannel() const { return channelIndex; }

private:
    TriggeredLFPDisplay* display;
    int plotIndex;
    
    LFPTriggerSource* triggerSource;
    int channelIndex;
    DisplayMode displayMode;
    float amplitudeScale;
    float timeScale;
    bool showGrid;
    
    // Data for display
    Array<Array<float>> individualTraces;
    Array<float> averagedTrace;
    Array<float> timeAxis;
    
    // Display parameters
    float yOffset;
    float yRange;
    bool autoScale;
    
    void drawTrace(Graphics& g, const Array<float>& trace, Colour colour, float alpha = 1.0f);
    void drawGrid(Graphics& g);
    void drawAxes(Graphics& g);
    void drawLabels(Graphics& g);
    void calculateDisplayRange();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LFPPlot);
};

/**
    Main display component containing the grid of LFP plots
*/
class TriggeredLFPDisplay : public Component
{
public:
    /** Constructor */
    TriggeredLFPDisplay(TriggeredLFPCanvas* canvas, TriggeredLFPViewer* processor);

    /** Destructor */
    ~TriggeredLFPDisplay() {}

    /** Component callback for rendering the background */
    void paint(Graphics& g) override;

    /** Component callback for resizing */
    void resized() override;

    /** Sets the window size for display */
    void setWindowSizeMs(int preSizeMs, int postSizeMs);

    /** Sets the display grid dimensions */
    void setGridSize(int rows, int cols);

    /** Sets the display mode for all plots */
    void setDisplayMode(DisplayMode mode);

    /** Sets the amplitude scale for all plots */
    void setAmplitudeScale(float scale);

    /** Sets the time scale for all plots */
    void setTimeScale(float scale);

    /** Enable/disable auto-scaling */
    void setAutoScale(bool autoScale);

    /** Show/hide grid lines */
    void setShowGrid(bool show);

    /** Clears all data from plots */
    void clear();

    /** Updates data for a specific trigger source */
    void updateTriggerSourceData(LFPTriggerSource* source);

    /** Updates data for all plots */
    void updateAllPlots();

    /** Saves current display to file */
    void saveToFile();

    /** Gets display information for saving */
    DynamicObject getInfo();

    /** Sets up the plot grid based on available trigger sources and channels */
    void setupPlotGrid();

    TriggeredLFPViewer* getProcessor() { return processor; }

private:
    TriggeredLFPCanvas* canvas;
    TriggeredLFPViewer* processor;

    OwnedArray<LFPPlot> plots;
    
    int gridRows;
    int gridCols;
    int maxPlots;
    
    int preWindowMs;
    int postWindowMs;
    
    DisplayMode displayMode;
    float amplitudeScale;
    float timeScale;
    bool autoScale;
    bool showGrid;

    void redistributePlots();
    LFPPlot* getPlotForSourceAndChannel(LFPTriggerSource* source, int channel);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggeredLFPDisplay);
};

/**
    Canvas for the Triggered LFP Viewer
*/
class TriggeredLFPCanvas : public Visualizer,
                          public Timer
{
public:
    /** Constructor */
    TriggeredLFPCanvas(TriggeredLFPViewer* processor);

    /** Destructor */
    ~TriggeredLFPCanvas() {}

    /** Draws the canvas background */
    void paint(Graphics& g) override;

    /** Called when the canvas is resized */
    void resized() override;

    /** Called when the visualizer's tab becomes active */
    void refreshState() override;

    /** Called when the data acquisition is started */
    void beginAnimation() override;

    /** Called when the data acquisition is stopped */
    void endAnimation() override;

    /** Called when parameters are updated */
    void refresh() override;

    /** Sets the window size for triggered data collection */
    void setWindowSizeMs(int preSizeMs, int postSizeMs);

    /** Called when new triggered data is available */
    void newDataReceived(LFPTriggerSource* source);

    /** Gets the display component */
    TriggeredLFPDisplay* getDisplay() { return display.get(); }

    /** Gets the processor */
    TriggeredLFPViewer* getProcessor() { return processor; }

    /** Gets the trigger source table */
    TriggerSourceTable* getTriggerSourceTable() { return triggerSourceTable.get(); }

    /** Timer callback for periodic updates */
    void timerCallback() override;

private:
    TriggeredLFPViewer* processor;

    std::unique_ptr<LFPOptionsBar> optionsBar;
    std::unique_ptr<TriggeredLFPDisplay> display;
    std::unique_ptr<ChannelSelector> channelSelector;
    std::unique_ptr<TriggerSourceTable> triggerSourceTable;

    bool acquisitionIsActive;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TriggeredLFPCanvas);
};

#endif // TriggeredLFPCANVAS_H_