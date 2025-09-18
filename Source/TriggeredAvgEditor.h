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
#include <VisualizerEditorHeaders.h>

class Visualizer;

namespace TriggeredAverage
{
class TriggeredAvgNode;
class TriggerSource;

/**

    User interface for the TriggeredLFPViewer processor.

*/

class TriggeredAvgEditor : public VisualizerEditor,
                           public ComboBox::Listener,
                           public Button::Listener
{
public:
    /** Constructor */
    TriggeredAvgEditor (GenericProcessor* parentNode);

    /** Destructor */
    ~TriggeredAvgEditor() {}

    /** Called when the tab becomes visible again */
    void collapsedStateChanged() override;

    /** Updates settings after a parameter value change */
    void updateSettings() override;

    /** Opens the canvas window */
    Visualizer* createNewCanvas() override;

    /** ComboBox listener callback */
    void comboBoxChanged (ComboBox* comboBoxThatHasChanged) override;

    /** Button listener callback */
    void buttonClicked (Button* button) override;

    /** Component callback for resizing */
    void resized() override;

private:
    /** Pointer to the actual processor */
    TriggeredAvgNode* processor;

    // Action button for clearing data (keep minimal controls in editor)
    std::unique_ptr<UtilityButton> clearDataButton;

    void updateParameterControls();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TriggeredAvgEditor);
};

} // namespace TriggeredAverage
#endif // __TriggeredLFPEditor_H_2C4C2D67__