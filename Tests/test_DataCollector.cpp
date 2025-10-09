#include "DataCollector.h"
#include "MultiChannelRingBuffer.h"
#include "TriggeredAvgNode.h"
#include <JuceHeader.h>
#include <ProcessorHeaders.h>
#include <gtest/gtest.h>
#include <memory>

using namespace TriggeredAverage;
using namespace testing;

// Mock TriggeredAvgNode for testing
class MockTriggeredAvgNode : public GenericProcessor
{
public:
    MockTriggeredAvgNode() : GenericProcessor ("MockTriggeredAvgNode") {}

    void process (AudioBuffer<float>& buffer) override {}
    AudioProcessorEditor* createEditor() override { return nullptr; }

    // Add any methods that DataCollector might call on the node
    void updateSettings() override {}
};

class DataCollectorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Initialize JUCE for GUI components
        scopedJuceInit = std::make_unique<ScopedJuceInitialiser_GUI>();

        // Create mock objects
        mockNode = std::make_unique<MockTriggeredAvgNode>();
        ringBuffer = std::make_unique<MultiChannelRingBuffer> (4, 1000); // 4 channels, 1000 samples

        // Create DataCollector
        dataCollector = std::make_unique<DataCollector> (mockNode.get(), ringBuffer.get());
    }

    void TearDown() override
    {
        // Stop the collector thread before destroying
        if (dataCollector)
        {
            dataCollector->stopThread (1000);
            dataCollector.reset();
        }

        ringBuffer.reset();
        mockNode.reset();
        scopedJuceInit.reset();
    }

    // Helper function to create a TTL event
    TTLEventPtr createTTLEvent (uint16 streamId, int64 sampleNumber, uint8 line, bool state)
    {
        auto eventChannel =
            new EventChannel (EventChannel::TTL, streamId, 0, 0, "Test TTL Channel");
        auto stream = new DataStream ("Test Stream", "Test description", "test.source", streamId);

        return TTLEvent::createTTLEvent (eventChannel, stream, sampleNumber, line, state);
    }

    std::unique_ptr<ScopedJuceInitialiser_GUI> scopedJuceInit;
    std::unique_ptr<MockTriggeredAvgNode> mockNode;
    std::unique_ptr<MultiChannelRingBuffer> ringBuffer;
    std::unique_ptr<DataCollector> dataCollector;
};

TEST_F (DataCollectorTest, ConstructorInitialization)
{
    EXPECT_TRUE (dataCollector != nullptr);
    EXPECT_FALSE (dataCollector->isThreadRunning());
}

TEST_F (DataCollectorTest, BasicTTLEventRegistration)
{
    // Create a TTL event
    auto ttlEvent = createTTLEvent (0, 1000, 1, true); // Stream 0, sample 1000, line 1, state true

    ASSERT_TRUE (ttlEvent != nullptr);
    EXPECT_EQ (ttlEvent->getSampleNumber(), 1000);
    EXPECT_EQ (ttlEvent->getLine(), 1);
    EXPECT_EQ (ttlEvent->getState(), true);

    // Register the event
    dataCollector->registerTTLTrigger (ttlEvent);

    // The event should be queued internally
    // We can't directly test the queue, but we can test that the method doesn't crash
    SUCCEED();
}

TEST_F (DataCollectorTest, MessageTriggerRegistration)
{
    String testMessage = "Test trigger message";
    int64 sampleNumber = 5000;

    // Register a message trigger
    dataCollector->registerMessageTrigger (testMessage, sampleNumber);

    // The message should be queued internally
    // We can't directly test the queue, but we can test that the method doesn't crash
    SUCCEED();
}

TEST_F (DataCollectorTest, ThreadLifecycle)
{
    // Start the thread
    dataCollector->startThread();
    EXPECT_TRUE (dataCollector->isThreadRunning());

    // Let it run briefly
    Thread::sleep (10);

    // Stop the thread
    dataCollector->stopThread (1000);
    EXPECT_FALSE (dataCollector->isThreadRunning());
}

TEST_F (DataCollectorTest, MultipleEventRegistration)
{
    // Register multiple TTL events
    for (int i = 0; i < 5; ++i)
    {
        auto ttlEvent = createTTLEvent (0, 1000 + i * 100, 1, i % 2 == 0); // Alternating states
        dataCollector->registerTTLTrigger (ttlEvent);
    }

    // Register multiple message triggers
    for (int i = 0; i < 3; ++i)
    {
        String message = "Message " + String (i);
        dataCollector->registerMessageTrigger (message, 2000 + i * 50);
    }

    // All events should be registered without crashing
    SUCCEED();
}

TEST_F (DataCollectorTest, ThreadSafety)
{
    // Start the thread
    dataCollector->startThread();

    // Register events from the main thread while the worker thread is running
    for (int i = 0; i < 10; ++i)
    {
        auto ttlEvent = createTTLEvent (0, 1000 + i, 1, true);
        dataCollector->registerTTLTrigger (ttlEvent);

        // Small delay to allow thread switching
        Thread::sleep (1);
    }

    // Stop the thread
    dataCollector->stopThread (1000);

    // Should complete without deadlocks or crashes
    SUCCEED();
}

// Test with actual data in the ring buffer
TEST_F (DataCollectorTest, WithRingBufferData)
{
    // Add some data to the ring buffer first
    AudioBuffer<float> testBuffer (4, 100);
    for (int ch = 0; ch < 4; ++ch)
    {
        for (int sample = 0; sample < 100; ++sample)
        {
            testBuffer.setSample (ch, sample, static_cast<float> (ch * 100 + sample));
        }
    }

    ringBuffer->addData (testBuffer, 0);
    EXPECT_EQ (ringBuffer->getCurrentSampleNumber(), 100);

    // Now register a trigger event
    auto ttlEvent = createTTLEvent (0, 50, 1, true); // Trigger at sample 50
    dataCollector->registerTTLTrigger (ttlEvent);

    // The data collector should be able to work with the ring buffer data
    SUCCEED();
}

// Test edge cases
TEST_F (DataCollectorTest, EdgeCases)
{
    // Test with null TTL event (should handle gracefully)
    // Note: We can't actually pass null due to the TTLEventPtr type,
    // but we can test boundary conditions

    // Test with very large sample numbers
    auto ttlEvent = createTTLEvent (0, INT64_MAX, 7, false);
    dataCollector->registerTTLTrigger (ttlEvent);

    // Test with empty message
    dataCollector->registerMessageTrigger ("", 0);

    // Test with very long message
    String longMessage = String::repeatedString ("A", 10000);
    dataCollector->registerMessageTrigger (longMessage, 12345);

    SUCCEED();
}
