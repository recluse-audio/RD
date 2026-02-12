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

int main(int argc, char* argv[])
{
    // Disable all debug logging by default for clean test output
    DebugLogger::enableAll(false);

    // Run Catch2 tests
    int result = Catch::Session().run(argc, argv);

    return result;
}
