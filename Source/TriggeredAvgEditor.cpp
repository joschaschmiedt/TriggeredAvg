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

#include "TriggeredAvgEditor.h"
#include "TriggeredAvgNode.h"
#include "TriggeredAvgCanvas.h"
using namespace TriggeredAverage;

// TriggeredLFPEditor implementation
TriggeredAvgEditor::TriggeredAvgEditor(GenericProcessor* parentNode) 
    : VisualizerEditor(parentNode, "Triggered Avg", 200)  // Reduced width since we moved controls
{
    processor = static_cast<TriggeredAvgNode*>(parentNode);

    // Keep only essential action button in the editor
    clearDataButton = std::make_unique<UtilityButton>("Clear Data");
    clearDataButton->addListener(this);
    addAndMakeVisible(clearDataButton.get());
}

void TriggeredAvgEditor::collapsedStateChanged()
{
    // Update canvas if needed
}

void TriggeredAvgEditor::updateSettings()
{
    updateParameterControls();
}

void TriggeredAvgEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    // No combo boxes in the editor anymore
}

void TriggeredAvgEditor::buttonClicked(Button* button)
{
    if (button == clearDataButton.get())
    {
        processor->clearAllData();
    }
}

Visualizer* TriggeredAvgEditor::createNewCanvas()
{
    TriggeredAvgNode* processor = static_cast<TriggeredAvgNode*>(getProcessor());
    
    auto triggered_avg_canvas = new TriggeredAvgCanvas(processor);
    processor->canvas = triggered_avg_canvas;
    
    return triggered_avg_canvas;
}

void TriggeredAvgEditor::updateParameterControls()
{
    // Update parameter control values if needed
}

void TriggeredAvgEditor::resized()
{
    int yPos = 30;
    int leftMargin = 10;

    // Only the clear data button remains
    clearDataButton->setBounds(leftMargin, yPos, 100, 20);
}