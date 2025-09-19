// SPDX-License-Identifier: GPL-3.0-or-later

#include "TimeAxis.h"
using namespace TriggeredAverage;

void TriggeredAverage::TimeAxis::paint (Graphics& g)
{
    const float fHeight = static_cast<float> (getHeight());
    const float fWidth = static_cast<float> (getWidth());
    const float fHistogramWidthPx = (fWidth - 30);

    float zeroLoc = static_cast<float> (preTriggerMs) / static_cast<float> (preTriggerMs + postTriggerMs) * fHistogramWidthPx;

    g.setColour (Colours::white);
    g.drawLine (zeroLoc, 0, zeroLoc, fHeight, 2.0);

    float windowSizeMs = preTriggerMs + postTriggerMs;
    float stepSize;

    if (windowSizeMs == 20.0f)
        stepSize = 1.0f;
    else if (windowSizeMs > 20.0f && windowSizeMs <= 50.0f)
        stepSize = 5.0f;
    else if (windowSizeMs > 50.0f && windowSizeMs <= 100.0f)
        stepSize = 10.0f;
    else if (windowSizeMs > 100.0f && windowSizeMs <= 250.0f)
        stepSize = 25.0f;
    else if (windowSizeMs > 250.0f && windowSizeMs <= 500.0f)
        stepSize = 50.0f;
    else if (windowSizeMs > 500.0f && windowSizeMs <= 1000.0f)
        stepSize = 100.0f;
    else if (windowSizeMs >= 1000.0f && windowSizeMs < 2000.0f)
        stepSize = 250.0f;
    else
        stepSize = 500.0f;

    const float tickDistance = (stepSize / windowSizeMs) * fHistogramWidthPx;

    float tick = stepSize;
    float tickLoc = zeroLoc + tickDistance;

    while (tick < postTriggerMs)
    {
        g.drawLine (tickLoc, fHeight, tickLoc, fHeight - 8, 2.0);
        g.drawText (String (tick), static_cast<int> (tickLoc - 50.0f), static_cast<int> (fHeight - 25.0f), 100, 15, Justification::centred);
        tick += stepSize;
        tickLoc += tickDistance;
    }

    tick = -stepSize;
    tickLoc = zeroLoc - tickDistance;

    while (tick > -preTriggerMs)
    {
        g.drawLine (tickLoc, fHeight, tickLoc, fHeight - 8, 2.0);
        g.drawText (String (tick), static_cast<int>(tickLoc - 54.0f), static_cast<int>(fHeight - 25.0f), 100, 15, Justification::centred);
        tick -= stepSize;
        tickLoc -= tickDistance;
    }
}

void TimeAxis::setWindowSizeMs (float pre, float post)
{
    if (pre < 0.0f)
        pre = (-1.0f) * pre;
    preTriggerMs = pre;
    postTriggerMs = post;

    repaint();
}
