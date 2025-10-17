#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggerSource.h"
#include "TriggeredAvgNode.h"
#include <ProcessorHeaders.h>

using namespace TriggeredAverage;

DataCollector::DataCollector (TriggeredAvgNode* viewer_, MultiChannelRingBuffer* buffer_)
    : Thread ("TriggeredAvg: Data Collector"),
      viewer (viewer_),
      ringBuffer (buffer_),
      newTriggerEvent (false)
{
    //setPriority(Thread::Priority::high);
}

DataCollector::~DataCollector() { stopThread (1000); }

void DataCollector::registerCaptureRequest (const CaptureRequest& request)
{
    const ScopedLock lock (triggerQueueLock);
    captureRequestQueue.emplace_back (request);
    newTriggerEvent.signal();
}

void DataCollector::run()
{
    constexpr double retryIntervalMs = 50.0;
    while (! threadShouldExit())
    {
        if (bool wasTriggered = newTriggerEvent.wait (100))
        {
            const ScopedLock lock (triggerQueueLock);
            while (! captureRequestQueue.empty())
            {
                switch (processCaptureRequest (captureRequestQueue.front()))
                {
                    case RingBufferReadResult::Success:
                    case RingBufferReadResult::DataInRingBufferTooOld:
                    case RingBufferReadResult::InvalidParameters:
                        captureRequestQueue.pop_front();
                        break;
                    case RingBufferReadResult::NotEnoughNewData:
                        // wait and try again later
                        wait (retryIntervalMs);
                        break;
                    case RingBufferReadResult::UnknownError:
                        assert (false);
                        break;
                }
            }
        }
    }
}

void DataCollector::registerTriggerSource (const TriggerSource*)
{

}

RingBufferReadResult DataCollector::processCaptureRequest (const CaptureRequest& request)
{
    if (! viewer)
        return RingBufferReadResult::UnknownError;

    auto result = ringBuffer->readAroundSample (
        request.triggerSample, request.preSamples, request.postSamples, m_collectBuffer);
    if (result == RingBufferReadResult::Success)
    {
        // allocate on first time?
        if(!m_averageBuffer.contains (request.triggerSource))
        {
            m_averageBuffer.emplace (
                request.triggerSource,
                AverageBuffer (m_collectBuffer.getNumChannels(), m_collectBuffer.getNumSamples()));
        }

        // erase and resize if number of channels does not match
        if (m_averageBuffer[request.triggerSource].getNumChannels() != m_collectBuffer.getNumChannels() ||
            m_averageBuffer[request.triggerSource].getNumSamples() != m_collectBuffer.getNumSamples())
        {
            m_averageBuffer.erase (request.triggerSource);
            m_averageBuffer.emplace (
                request.triggerSource,
                AverageBuffer (m_collectBuffer.getNumChannels(), m_collectBuffer.getNumSamples()));
        }

        m_averageBuffer.at(request.triggerSource).addBuffer (m_collectBuffer);
    }
    return result;
}
AverageBuffer::AverageBuffer (int numChannels, int numSamples)
    : m_numChannels (numChannels),
      m_numSamples (numSamples)
{
    m_sumBuffer.setSize (numChannels, numSamples);
    m_sumSquaresBuffer.setSize (numChannels, numSamples);
    reset();
}
AverageBuffer::AverageBuffer (AverageBuffer&& other) noexcept
    : m_numChannels (other.m_numChannels),
      m_numSamples (other.m_numSamples)
{
    m_sumBuffer = std::move (other.m_sumBuffer);
    m_sumSquaresBuffer = std::move (other.m_sumSquaresBuffer);
    m_numTrials = other.m_numTrials;
}
AverageBuffer& AverageBuffer::operator= (AverageBuffer&& other) noexcept
{
    if (this != &other)
    {
        m_sumBuffer = std::move (other.m_sumBuffer);
        m_sumSquaresBuffer = std::move (other.m_sumSquaresBuffer);
        m_numTrials = other.m_numTrials;
        m_numChannels = other.m_numChannels;
        m_numSamples = other.m_numSamples;
    }
    return *this;
}
void AverageBuffer::addBuffer (const juce::AudioBuffer<float>& buffer)
{
    jassert (buffer.getNumChannels() == m_numChannels);
    jassert (buffer.getNumSamples() == m_numSamples);

    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        auto* sumData = m_sumBuffer.getWritePointer (ch);
        auto* sumSquaresData = m_sumSquaresBuffer.getWritePointer (ch);
        auto* inputData = buffer.getReadPointer (ch);

        for (int i = 0; i < m_numSamples; ++i)
        {
            float sample = inputData[i];
            sumData[i] += sample;
            sumSquaresData[i] += sample * sample;
        }
    }

    ++m_numTrials;
}
AudioBuffer<float> AverageBuffer::getAverage() const
{
    AudioBuffer<float> outputBuffer;
    ;
    if (m_numTrials == 0)
    {
        outputBuffer.clear();
        return outputBuffer;
    }

    outputBuffer.setSize (m_numChannels, m_numSamples, false, false, true);

    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        auto* sumData = m_sumBuffer.getReadPointer (ch);
        auto* outputData = outputBuffer.getWritePointer (ch);

        for (int i = 0; i < m_numSamples; ++i)
        {
            outputData[i] = sumData[i] / static_cast<float> (m_numTrials);
        }
    }
    return outputBuffer;
}
AudioBuffer<float> AverageBuffer::getStandardDeviation() const
{
    juce::AudioBuffer<float> outputBuffer;
    if (m_numTrials == 0)
    {
        outputBuffer.clear();
        return outputBuffer;
    }

    outputBuffer.setSize (m_numChannels, m_numSamples, false, false, true);

    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        auto* sumData = m_sumBuffer.getReadPointer (ch);
        auto* sumSquaresData = m_sumSquaresBuffer.getReadPointer (ch);
        auto* outputData = outputBuffer.getWritePointer (ch);

        for (int i = 0; i < m_numSamples; ++i)
        {
            const float mean = sumData[i] / static_cast<float> (m_numTrials);
            const float meanSquares = sumSquaresData[i] / static_cast<float> (m_numTrials);
            const float variance = meanSquares - (mean * mean);
            outputData[i] = std::sqrt (
                std::max (0.0f, variance)); // Clamp to avoid negative due to float precision
        }
    }
    return outputBuffer;
}

void AverageBuffer::reset()
{
    m_sumBuffer.clear();
    m_sumSquaresBuffer.clear();
    m_numTrials = 0;
}
int AverageBuffer::getNumTrials() const { return m_numTrials; }
int AverageBuffer::getNumChannels() const
{
    assert (m_sumBuffer.getNumChannels() == m_sumSquaresBuffer.getNumChannels());
    return m_sumBuffer.getNumChannels();
}
int AverageBuffer::getNumSamples() const
{
    assert (m_sumBuffer.getNumChannels() == m_sumSquaresBuffer.getNumChannels());
    return m_sumBuffer.getNumSamples();
}
