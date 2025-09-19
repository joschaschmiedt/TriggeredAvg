#include "MultiChannelRingBuffer.h"

MultiChannelRingBuffer::MultiChannelRingBuffer (int numChannels_, int bufferSize_)
    : buffer (numChannels_, bufferSize_),
      currentSampleNumber (0),
      writeIndex (0),
      validSize (0),
      numChannels (numChannels_),
      bufferSize (bufferSize_)
{
    sampleNumbers.resize (bufferSize);
    //buffer.clear();
}

void MultiChannelRingBuffer::addData (const AudioBuffer<float>& inputBuffer,
                                      int64 firstSampleNumber)
{
    const ScopedLock lock (writeLock);

    const int numSamplesIn = inputBuffer.getNumSamples();
    if (numSamplesIn <= 0)
        return;

    // If the incoming block is larger than the buffer, only keep the last bufferSize samples.
    const int writeCount = jmin (numSamplesIn, bufferSize);
    const int srcOffset = numSamplesIn - writeCount;

    // first segment (until end of ring)
    const int spaceToEnd = bufferSize - writeIndex.load();
    const int blockSize1 = jmin (writeCount, spaceToEnd);

    if (blockSize1 > 0)
    {
        for (int ch = 0; ch < jmin (numChannels, inputBuffer.getNumChannels()); ++ch)
        {
            buffer.copyFrom (ch, writeIndex, inputBuffer, ch, srcOffset, blockSize1);
        }

        for (int i = 0; i < blockSize1; ++i)
        {
            sampleNumbers.getReference (writeIndex + i) = firstSampleNumber + srcOffset + i;
        }
    }

    // second segment (from start of ring)
    if (const int blockSize2 = writeCount - blockSize1; blockSize2 > 0)
    {
        for (int ch = 0; ch < jmin (numChannels, inputBuffer.getNumChannels()); ++ch)
        {
            buffer.copyFrom (ch, 0, inputBuffer, ch, srcOffset + blockSize1, blockSize2);
        }

        for (int i = 0; i < blockSize2; ++i)
        {
            sampleNumbers.getReference (i) = firstSampleNumber + srcOffset + blockSize1 + i;
        }
    }

    // Advance write index and valid size (overwrite semantics)
    writeIndex = (writeIndex + writeCount) % bufferSize;
    int newValid = validSize + writeCount;
    if (newValid > bufferSize)
        newValid = bufferSize;
    validSize = newValid;

    // Track the latest absolute sample number according to the input timeline
    currentSampleNumber.store (firstSampleNumber + numSamplesIn);
}

bool MultiChannelRingBuffer::readTriggeredData (int64 triggerSample,
                                                int preSamples,
                                                int postSamples,
                                                Array<int> channelIndices,
                                                AudioBuffer<float>& outputBuffer) const
{
    const ScopedLock lock (writeLock);

    const int totalSamples = preSamples + postSamples;
    if (totalSamples <= 0)
        return false;

    const int64 startSample = triggerSample - preSamples;

    // window must be inside [oldest, current)
    const int64 currentSample = currentSampleNumber.load();
    const int vSize = validSize.load();
    const int64 oldestSample = currentSample - vSize;

    if (startSample < oldestSample)
        return false; // Data too old

    const int64 endExclusive = startSample + totalSamples;
    if (endExclusive > currentSample)
        return false; // Not enough new data yet

    // Calculate ring buffer positions
    const int oldestIndex = (bufferSize + (writeIndex.load() - vSize) % bufferSize) % bufferSize;
    const int64 offsetFromOldest = startSample - oldestSample; // 0 .. vSize-1
    const int bufferStartPos = (int) ((oldestIndex + (int) offsetFromOldest) % bufferSize);

    outputBuffer.setSize (channelIndices.size(), totalSamples);

    for (int outCh = 0; outCh < channelIndices.size(); ++outCh)
    {
        const int sourceCh = channelIndices[outCh];
        if (sourceCh < 0 || sourceCh >= numChannels)
        {
            outputBuffer.clear (outCh, 0, totalSamples);
            continue;
        }

        // We can copy in up to 2 blocks due to wraparound
        const int firstBlock = jmin (totalSamples, bufferSize - bufferStartPos);
        if (firstBlock > 0)
            outputBuffer.copyFrom (outCh, 0, buffer, sourceCh, bufferStartPos, firstBlock);

        const int secondBlock = totalSamples - firstBlock;
        if (secondBlock > 0)
            outputBuffer.copyFrom (outCh, firstBlock, buffer, sourceCh, 0, secondBlock);
    }

    return true;
}

bool MultiChannelRingBuffer::hasEnoughDataForRead (int64 triggerSample, int preSamples, int postSamples) const
{
    // this does not require locking as it only reads atomic variables
    const int totalSamples = preSamples + postSamples;
    if (totalSamples <= 0)
        return false;

    const int64 startSample = triggerSample - preSamples;

    // window must be inside [oldest, current)
    const int64 currentSample = currentSampleNumber.load();
    const int vSize = validSize.load();
    const int64 oldestSample = currentSample - vSize;

    if (startSample < oldestSample)
        return false; // Data too old

    const int64 endExclusive = startSample + totalSamples;
    if (endExclusive > currentSample)
        return false; // Not enough new data yet

    return true;

}

void MultiChannelRingBuffer::reset()
{
    const ScopedLock lock (writeLock);
    buffer.clear();
    currentSampleNumber.store (0);
    writeIndex.store (0);
    validSize.store (0);
}
