#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggerSource.h"
#include "TriggeredAvgNode.h"
#include <ProcessorHeaders.h>

using namespace TriggeredAverage;

void DataStore::ResetAndSetSize (TriggerSource* source, int nChannels, int nSamples)
{
    std::scoped_lock<std::recursive_mutex> lock (m_mutex);
    if (! source)
    {
        for (auto& [key, value] : m_averageBuffers)
        {
            value.setSize (nChannels, nSamples);
        }
    }
    else
    {
        m_averageBuffers[source].setSize (nChannels, nSamples);
    }
}

DataCollector::DataCollector (TriggeredAvgNode* viewer_,
                              MultiChannelRingBuffer* buffer_,
                              DataStore* datastore_)
    : Thread ("TriggeredAvg: Data Collector"),
      m_processor (viewer_),
      ringBuffer (buffer_),
      m_datastore (datastore_),
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
    constexpr int maximumNumberOfRetries = 200;
    while (! threadShouldExit())
    {
        if (bool wasTriggered = newTriggerEvent.wait (100))
        {
            const ScopedLock lock (triggerQueueLock);
            bool averageBuffersWereUpdated = false;
            int iRetry = maximumNumberOfRetries;
            while (! captureRequestQueue.empty())
            {
                switch (processCaptureRequest (captureRequestQueue.front()))
                {
                    case RingBufferReadResult::Success:
                    case RingBufferReadResult::DataInRingBufferTooOld:
                        averageBuffersWereUpdated = true;
                        captureRequestQueue.pop_front();
                        break;
                    case RingBufferReadResult::NotEnoughNewData:
                        if (iRetry > 0)
                        {
                            wait (retryIntervalMs);
                            iRetry--;
                        }
                        else
                        {
                            captureRequestQueue.pop_front();
                        }
                        break;
                    case RingBufferReadResult::InvalidParameters:
                    case RingBufferReadResult::UnknownError:
                        assert (false);
                        break;
                }
            }
            if (averageBuffersWereUpdated)
            {
                // notify the processor that the data has been updated
                if (m_processor != nullptr)
                    m_processor->triggerAsyncUpdate();
            }
        }
    }
}

// process a single capture request on the ring buffer, running on the data collector thread
RingBufferReadResult DataCollector::processCaptureRequest (const CaptureRequest& request)
{
    auto result = ringBuffer->readAroundSample (
        request.triggerSample, request.preSamples, request.postSamples, m_collectBuffer);
    if (result == RingBufferReadResult::Success)
    {
        //// allocate on first time?
        //if (! m_averageBuffers.contains (request.triggerSource))
        //{
        //    m_averageBuffers.emplace (
        //        request.triggerSource,
        //        MultiChannelAverageBuffer (m_collectBuffer.getNumChannels(), m_collectBuffer.getNumSamples()));
        //}

        //// erase and resize if number of channels does not match
        //if (m_averageBuffers[request.triggerSource].getNumChannels()
        //        != m_collectBuffer.getNumChannels()
        //    || m_averageBuffers[request.triggerSource].getNumSamples()
        //           != m_collectBuffer.getNumSamples())
        //{
        //    m_averageBuffers.erase (request.triggerSource);
        //    m_averageBuffers.emplace (
        //        request.triggerSource,
        //        MultiChannelAverageBuffer (m_collectBuffer.getNumChannels(), m_collectBuffer.getNumSamples()));
        //}

        m_datastore->getRefToAverageBufferForTriggerSource (request.triggerSource)
            ->addBuffer (m_collectBuffer);

        //m_averageBuffers.at (request.triggerSource).addBuffer (m_collectBuffer);
    }
    return result;
}
MultiChannelAverageBuffer::MultiChannelAverageBuffer (int numChannels, int numSamples)
    : m_numChannels (numChannels),
      m_numSamples (numSamples)
{
    m_sumBuffer.setSize (numChannels, numSamples);
    m_sumSquaresBuffer.setSize (numChannels, numSamples);
    resetTrials();
}
MultiChannelAverageBuffer::MultiChannelAverageBuffer (MultiChannelAverageBuffer&& other) noexcept
    : m_numChannels (other.m_numChannels),
      m_numSamples (other.m_numSamples)
{
    m_sumBuffer = std::move (other.m_sumBuffer);
    m_sumSquaresBuffer = std::move (other.m_sumSquaresBuffer);
    m_numTrials = other.m_numTrials;
}
MultiChannelAverageBuffer&
    MultiChannelAverageBuffer::operator= (MultiChannelAverageBuffer&& other) noexcept
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
void MultiChannelAverageBuffer::addBuffer (const juce::AudioBuffer<float>& buffer)
{
    jassert (buffer.getNumChannels() == m_numChannels);
    jassert (buffer.getNumSamples() == m_numSamples);

    for (int ch = 0; ch < m_numChannels; ++ch)
    {
        auto* sumData = m_sumBuffer.getWritePointer (ch);
        // TODO: Use SIMD
        //m_sumBuffer.addFrom (ch, 0, buffer, ch, 0, m_numSamples);
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
AudioBuffer<float> MultiChannelAverageBuffer::getAverage() const
{
    AudioBuffer<float> outputBuffer;
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
AudioBuffer<float> MultiChannelAverageBuffer::getStandardDeviation() const
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

void MultiChannelAverageBuffer::resetTrials()
{
    m_sumBuffer.clear();
    m_sumSquaresBuffer.clear();
    m_numTrials = 0;
}
int MultiChannelAverageBuffer::getNumTrials() const { return m_numTrials; }
int MultiChannelAverageBuffer::getNumChannels() const
{
    assert (m_sumBuffer.getNumChannels() == m_sumSquaresBuffer.getNumChannels());
    return m_sumBuffer.getNumChannels();
}
int MultiChannelAverageBuffer::getNumSamples() const
{
    assert (m_sumBuffer.getNumChannels() == m_sumSquaresBuffer.getNumChannels());
    return m_sumBuffer.getNumSamples();
}
