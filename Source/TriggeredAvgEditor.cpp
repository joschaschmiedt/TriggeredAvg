// SPDX-License-Identifier: GPL-3.0-or-later

#include "TriggeredAvgEditor.h"

#include "DataCollector.h"
#include "PopupConfigurationWindow.h"
#include "TriggeredAvgActions.h"
#include "TriggeredAvgCanvas.h"
#include "TriggeredAvgNode.h"
using namespace TriggeredAverage;

TriggeredAvgEditor::TriggeredAvgEditor (GenericProcessor* parentNode)
    : VisualizerEditor (parentNode, "TRIG AVG", 210),
      canvas (nullptr),
      currentConfigWindow (nullptr)

{
    addBoundedValueParameterEditor (Parameter::PROCESSOR_SCOPE, ParameterNames::pre_ms, 20, 30);
    addBoundedValueParameterEditor (Parameter::PROCESSOR_SCOPE, ParameterNames::post_ms, 20, 78);

    for (auto& p : { "pre_ms", "post_ms" })
    {
        auto* ed = getParameterEditor (p);
        ed->setLayout (ParameterEditor::Layout::nameOnTop);
        ed->setBounds (ed->getX(), ed->getY(), 80, 36);
    }

    configureButton = std::make_unique<UtilityButton> ("CONFIGURE");
    configureButton->setFont (FontOptions (14.0f));
    configureButton->addListener (this);
    configureButton->setBounds (115, 85, 80, 30);
    addAndMakeVisible (configureButton.get());
}

Visualizer* TriggeredAvgEditor::createNewCanvas()
{
    auto* p = dynamic_cast<TriggeredAvgNode*> (getProcessor());
    assert (p);

    canvas = new TriggeredAvgCanvas (p);
    p->setCanvas (canvas);

    updateSettings();

    return canvas;
}

void TriggeredAvgEditor::updateSettings()
{
    if (canvas == nullptr)
        return;

    canvas->prepareToUpdate();

    TriggeredAvgNode* proc = dynamic_cast<TriggeredAvgNode*> (getProcessor());
    assert (proc);
    DataStore* store = (proc->getDataStore());
    assert (store);

    store->Clear();
    const int nChannels = proc->getTotalContinuousChannels();
    const int nSamples = proc->getNumberOfSamples();

    for (auto source : proc->getTriggerSources())
    {
        store->ResetAndSetSize (source, nChannels, nSamples);
        for (int i = 0; i < proc->getTotalContinuousChannels(); i++)
        {
            const ContinuousChannel* channel = proc->getContinuousChannel (i);

            canvas->addContChannel (
                channel, source, i, store->getRefToAverageBufferForTriggerSource (source));
        }
    }
    //for (int i = 0; i < proc->getTotalContinuousChannels(); i++)
    //{
    //    const ContinuousChannel* channel = proc->getContinuousChannel (i);

    //    for (auto source : proc->getTriggerSources())
    //    {
    //        //if (channel->isValid())
    //        {
    //            auto* buf = proc->getDataStore()->getRefToAverageBufferForTriggerSource (source);
    //            canvas->addContChannel (channel, source);
    //            //LOGD("Editor adding ", channel->getName(), " for ", source->name);
    //        }
    //    }
    //}

    canvas->setWindowSizeMs (proc->getPreWindowSizeMs(), proc->getPostWindowSizeMs());

    canvas->resized();
}

void TriggeredAvgEditor::updateColours (TriggerSource* source)
{
    if (canvas == nullptr)
        return;

    canvas->updateColourForSource (source);
}

void TriggeredAvgEditor::updateConditionName (TriggerSource* source)
{
    if (canvas == nullptr)
        return;

    canvas->updateConditionName (source);
}

void TriggeredAvgEditor::buttonClicked (Button* button)
{
    if (button == configureButton.get())
    {
        TriggeredAvgNode* proc = (TriggeredAvgNode*) getProcessor();

        Array<TriggerSource*> triggerLines = proc->getTriggerSources();
        LOGD (triggerLines.size(), " trigger sources found.");

        currentConfigWindow =
            new Popup::PopupConfigurationWindow (this, triggerLines, acquisitionIsActive);

        CoreServices::getPopupManager()->showPopup (
            std::unique_ptr<PopupComponent> (currentConfigWindow), button);

        return;
    }
}

void TriggeredAvgEditor::addTriggerSources (Popup::PopupConfigurationWindow* window,
                                            Array<int> lines,
                                            TriggerType type)
{
    TriggeredAvgNode* proc = (TriggeredAvgNode*) getProcessor();

    AddTriggerConditions* action = new AddTriggerConditions (proc, lines, type);

    CoreServices::getUndoManager()->beginNewTransaction ("Disabled during acquisition");
    CoreServices::getUndoManager()->perform ((UndoableAction*) action);

    if (window != nullptr)
        window->update (proc->getTriggerSources());
}

void TriggeredAvgEditor::removeTriggerSources (
    Popup::PopupConfigurationWindow* window,
    juce::Array<TriggerSource*, juce::DummyCriticalSection, 0> triggerSourcesToRemove)
{
    TriggeredAvgNode* proc = (TriggeredAvgNode*) getProcessor();

    RemoveTriggerConditions* action = new RemoveTriggerConditions (proc, triggerSourcesToRemove);

    CoreServices::getUndoManager()->beginNewTransaction ("Disabled during acquisition");
    CoreServices::getUndoManager()->perform ((UndoableAction*) action);

    if (window != nullptr)
        window->update (proc->getTriggerSources());
}
