/**
 * test_main.cpp
 * Created by Ryan Devens
 *
 * Custom Catch2 main that disables DebugLogger output during tests.
 * This keeps test output clean while preserving the ability to enable
 * specific logging categories when debugging individual tests.
 */

#include <catch2/catch_session.hpp>
#include "../SOURCE/Util/DebugLogger.h"
#include "../SOURCE/Util/Juce_Header.h"

int main(int argc, char* argv[])
{
    // Owns JUCE's GUI/event singletons (MessageManager, TimerThread, ShutdownDetector)
    // for the whole process. Required so tests using AudioProcessorGraph / Timer don't
    // leak ShutdownDetector and don't trip JUCE_ASSERT_MESSAGE_MANAGER_EXISTS during
    // teardown. Per-test TestUtils::SetupAndTeardown still marks the message thread.
    juce::ScopedJuceInitialiser_GUI juceInit;

    DebugLogger::enableAll(false);

    int result = Catch::Session().run(argc, argv);

    return result;
}
