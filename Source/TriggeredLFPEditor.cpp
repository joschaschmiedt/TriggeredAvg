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

// TriggeredLFPEditor implementation
TriggeredLFPEditor::TriggeredLFPEditor(GenericProcessor* parentNode) 
    : VisualizerEditor(parentNode, "LFP", 200)  // Reduced width since we moved controls
{
    processor = static_cast<TriggeredLFPViewer*>(parentNode);

    // Keep only essential action button in the editor
    clearDataButton = std::make_unique<UtilityButton>("Clear Data");
    clearDataButton->addListener(this);
    addAndMakeVisible(clearDataButton.get());
}

void TriggeredLFPEditor::collapsedStateChanged()
{
    // Update canvas if needed
}

void TriggeredLFPEditor::updateSettings()
{
    updateParameterControls();
}

void TriggeredLFPEditor::comboBoxChanged(ComboBox* comboBoxThatHasChanged)
{
    // No combo boxes in the editor anymore
}

void TriggeredLFPEditor::buttonClicked(Button* button)
{
    if (button == clearDataButton.get())
    {
        processor->clearAllData();
    }
}

Visualizer* TriggeredLFPEditor::createNewCanvas()
{
    TriggeredLFPViewer* processor = static_cast<TriggeredLFPViewer*>(getProcessor());
    
    auto canvas = new TriggeredLFPCanvas(processor);
    processor->canvas = canvas;
    
    return canvas;
}

void TriggeredLFPEditor::updateParameterControls()
{
    // Update parameter control values if needed
}

void TriggeredLFPEditor::resized()
{
    int yPos = 30;
    int leftMargin = 10;

    // Only the clear data button remains
    clearDataButton->setBounds(leftMargin, yPos, 100, 20);
}