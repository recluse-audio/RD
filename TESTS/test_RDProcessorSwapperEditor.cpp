#include <catch2/catch_test_macros.hpp>
#include "../SOURCE/PROCESSORS/RD_ProcessorSwapper.h"
#include "../SOURCE/EDITORS/RD_ProcessorSwapperEditor.h"
#include "TEST_UTILS/TestUtils.h"

struct RD_ProcessorSwapperEditorTests
{
    static void onProcessorSelected (RD_ProcessorSwapperEditor& editor, int comboBoxId)
    {
        editor._onProcessorSelected (comboBoxId);
    }
};

TEST_CASE("RD_ProcessorSwapperEditor _onProcessorSelected updates active processor index", "[RD_ProcessorSwapperEditor]")
{
    TestUtils::SetupAndTeardown setup;
    RD_ProcessorSwapper swapper;
    RD_ProcessorSwapperEditor editor (swapper);

    SECTION("Selecting kTDPSOLA updates the active processor index")
    {
        const int id = static_cast<int> (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA) + 1;
        RD_ProcessorSwapperEditorTests::onProcessorSelected (editor, id);

        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA);
    }

    SECTION("Selecting kGain after kTDPSOLA updates the active processor index")
    {
        const int tdpsolaId = static_cast<int> (RD_ProcessorSwapper::ProcessorIndex::kTDPSOLA) + 1;
        const int gainId    = static_cast<int> (RD_ProcessorSwapper::ProcessorIndex::kGain)    + 1;

        RD_ProcessorSwapperEditorTests::onProcessorSelected (editor, tdpsolaId);
        RD_ProcessorSwapperEditorTests::onProcessorSelected (editor, gainId);

        REQUIRE(swapper.getActiveProcessorIndex() == RD_ProcessorSwapper::ProcessorIndex::kGain);
    }
}
