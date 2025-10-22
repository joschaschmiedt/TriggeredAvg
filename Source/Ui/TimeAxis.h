// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <VisualizerWindowHeaders.h>

namespace TriggeredAverage
{
class TimeAxis : public Component
{
public:
    void paint (Graphics& g) override;
    void setWindowSizeMs (float pre_ms, float post_ms);

private:
    float preTriggerMs = 250.0f; // should not be negative
    float postTriggerMs = 500;
};
} // namespace TriggeredAverage
