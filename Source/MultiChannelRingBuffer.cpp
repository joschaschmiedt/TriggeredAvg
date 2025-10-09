#include "MultiChannelRingBuffer.h"

#include <juce_audio_basics/juce_audio_basics.h> // for AudioBuffer

using namespace TriggeredAverage;
using namespace juce;

MultiChannelRingBuffer::MultiChannelRingBuffer (int numChannels_, int bufferSize_)
    : m_buffer (numChannels_, bufferSize_),
      m_nChannels (numChannels_),
      m_bufferSize (bufferSize_)
{
    m_sampleNumbers.resize (m_bufferSize);
    m_buffer.clear();
}

void MultiChannelRingBuffer::addData (const AudioBuffer<float>& inputBuffer,
                                      int64 firstSampleNumber)
{
    const std::scoped_lock lock (writeLock);

    const int numSamplesIn = inputBuffer.getNumSamples();
    if (numSamplesIn <= 0)
        return;

    // If the incoming block is larger than the buffer, only keep the last bufferSize samples.
    const int writeCount = std::min (numSamplesIn, m_bufferSize);
    const int srcOffset = numSamplesIn - writeCount;

    // first segment (until end of ring)
    const int spaceToEnd = m_bufferSize - m_writeIndex.load();
    const int blockSize1 = std::min (writeCount, spaceToEnd);

    if (blockSize1 > 0)
    {
        for (int ch = 0; ch < std::min (m_nChannels, inputBuffer.getNumChannels()); ++ch)
        {
            m_buffer.copyFrom (ch, m_writeIndex.load(), inputBuffer, ch, srcOffset, blockSize1);
        }

        for (int i = 0; i < blockSize1; ++i)
        {
            m_sampleNumbers.getReference (m_writeIndex + i) = firstSampleNumber + srcOffset + i;
        }
    }

    // second segment (from start of ring)
    if (const int blockSize2 = writeCount - blockSize1; blockSize2 > 0)
    {
        for (int ch = 0; ch < std::min (m_nChannels, inputBuffer.getNumChannels()); ++ch)
        {
            m_buffer.copyFrom (ch, 0, inputBuffer, ch, srcOffset + blockSize1, blockSize2);
        }

        for (int i = 0; i < blockSize2; ++i)
        {
            m_sampleNumbers.getReference (i) = firstSampleNumber + srcOffset + blockSize1 + i;
        }
    }

    // Advance write index and valid size (overwrite semantics)
    m_writeIndex = (m_writeIndex + writeCount) % m_bufferSize;
    int newValid = m_nValidSamplesInBuffer + writeCount;
    if (newValid > m_bufferSize)
        newValid = m_bufferSize;
    m_nValidSamplesInBuffer = newValid;

    // Track the latest absolute sample number according to the input timeline
    m_nextSampleNumber.store (firstSampleNumber + numSamplesIn);
}

RingBufferReadResult
    MultiChannelRingBuffer::readTriggeredData (int64 centerSample,
                                               int preSamples,
                                               int postSamples,
                                               Array<int> channelIndices,
                                               AudioBuffer<float>& outputBuffer) const
{
    auto [result, startSample] = getStartSampleForTriggeredRead (centerSample, preSamples, postSamples);
    if (result != RingBufferReadResult::Success)
        return result;

    // TODO: This should be lock free if we stay behind the write index
    const std::scoped_lock lock (writeLock);
    auto bufferStartPos = startSample.value();
    const int64 totalSamples = preSamples + postSamples;

    outputBuffer.setSize (channelIndices.size(), totalSamples);

    for (int outCh = 0; outCh < channelIndices.size(); ++outCh)
    {
        const int sourceCh = channelIndices[outCh];
        if (sourceCh < 0 || sourceCh >= m_nChannels)
        {
            outputBuffer.clear (outCh, 0, totalSamples);
            continue;
        }

        // We can copy in up to 2 blocks due to wraparound
        const int firstBlock = std::min (totalSamples, m_bufferSize - bufferStartPos);
        if (firstBlock > 0)
            outputBuffer.copyFrom (outCh, 0, m_buffer, sourceCh, bufferStartPos, firstBlock);

        const int secondBlock = totalSamples - firstBlock;
        if (secondBlock > 0)
            outputBuffer.copyFrom (outCh, firstBlock, m_buffer, sourceCh, 0, secondBlock);
    }

    return RingBufferReadResult::Success;
}

std::pair<RingBufferReadResult, std::optional<int64>>
    MultiChannelRingBuffer::getStartSampleForTriggeredRead (int64 centerSample,
                                                  int preSamples,
                                                  int postSamples) const
{
    // this does not require locking as it only reads atomic variables
    const int totalSamples = preSamples + postSamples;
    if (totalSamples <= 0)
        return { RingBufferReadResult::InvalidParameters, std::nullopt };

    const int64 startSample = centerSample - preSamples;
    const int64 endExclusive = startSample + totalSamples;
    
    const int64 currentSample = m_nextSampleNumber.load();
    const int nValidSamplesInBuffer = m_nValidSamplesInBuffer.load();
    const int64 oldestSample = currentSample - nValidSamplesInBuffer;

    // window must be inside [oldest, current)
    if (startSample < oldestSample)
        return { RingBufferReadResult::DataInRingBufferTooOld, std::nullopt };
    
    if (endExclusive > currentSample)
        return { RingBufferReadResult::NotEnoughNewData, std::nullopt };

    // Calculate ring buffer position
    const int writeIndex = m_writeIndex.load();
    const int oldestIndex = (writeIndex - nValidSamplesInBuffer + m_bufferSize) % m_bufferSize;
    const int64 bufferStartPos = (oldestIndex + (startSample - oldestSample)) % m_bufferSize;
    
    return { RingBufferReadResult::Success, bufferStartPos };
}

void MultiChannelRingBuffer::reset()
{
    const std::scoped_lock lock (writeLock);
    m_buffer.clear();
    m_nextSampleNumber.store (0);
    m_writeIndex.store (0);
    m_nValidSamplesInBuffer.store (0);
}
