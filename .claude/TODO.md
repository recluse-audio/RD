# TODO

## Bugs / Test Gaps

- **DataLogger end-to-end length coverage missing.** `synthesis_grains.csv` from `TESTS/GRANULATOR/OUTPUT/Granulator Somewhere end-to-end/TEST_CASE_ROOT_DIR/DATA_LOG_OUTPUT_DIR/synthesis_grains.csv` only contained grains/detections from first few thousand samples — should have spanned full input. This bug should have been caught by tests.
  - Fix: every end-to-end DataLogger test must assert log file covers full duration of input (e.g. last logged sample index ≈ total samples processed, or grain/detection events present near end of buffer, not just start).
  - Apply to: Granulator, GrainShifterProcessor, PitchManager, any processor with DataLogger child cascade.
