# Plan: link rd_dsp as a CMake target, stop carrying loose sources

## Why

rd_dsp already ships a working `CMakeLists.txt` that builds a `RD_DSP` static library with `SOURCE/` as its `PUBLIC` include dir. This is the same consumption model JUCE uses (`add_subdirectory` + link).

RD currently ignores that and consumes rd_dsp as **loose source files**:

```cmake
include(CMAKE/RD_DSP_SOURCES.cmake)
target_sources(RD PRIVATE ${SOURCES} ${RD_DSP_SOURCES})
target_include_directories(RD PRIVATE ... SUBMODULES/RD_DSP/SOURCE)
```

This forces RD to hand-maintain `CMAKE/RD_DSP_SOURCES.cmake` whenever rd_dsp adds a file, and pushes the same burden onto every consumer of RD (RD_SYNTH, PULSARELLO, future plugins). It's the workaround that made `CONSUMER_SOURCES.cmake` necessary in the first place. Fix the root cause; the workaround mostly evaporates.

## End state

### `RD/CMakeLists.txt`

Replace the rd_dsp source-list block with a subdirectory + link:

```cmake
# Embed rd_dsp as a CMake subproject. Guard so a parent (consumer plugin)
# that already added it wins — RD's wrapper sources then compile against
# the already-defined RD_DSP target.
if(NOT TARGET RD_DSP)
    set(RD_DSP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(RD_DSP_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
    add_subdirectory(SUBMODULES/RD_DSP)
endif()

include(CMAKE/SOURCES.cmake)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE RD_DSP)
# rd_dsp's PUBLIC include dir (SUBMODULES/RD_DSP/SOURCE) propagates via the link.
```

Deletes / removals:

- `RD/CMAKE/RD_DSP_SOURCES.cmake` — gone.
- `${RD_DSP_SOURCES}` reference in `RD/CMakeLists.txt` — gone.
- Hard-coded `${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/RD_DSP/SOURCE` in `target_include_directories` — gone.

Rename RD's `BUILD_TESTS` option to `RD_BUILD_TESTS` so it doesn't collide with the rd_dsp flag (even though rd_dsp's flag will be renamed too — see prerequisite below). Update RD's `build_tests.py` / `rebuild_all.py` helper scripts to pass the new name. Update `.claude/CLAUDE.md` build-commands block to show `-DRD_BUILD_TESTS=ON`.

### `RD/CMAKE/CONSUMER_SOURCES.cmake`

Strip the rd_dsp inclusion. Final shape:

```cmake
set(_RD_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
include(${CMAKE_CURRENT_LIST_DIR}/SOURCES.cmake)

set(RD_SOURCES "")
foreach(_rd_src ${SOURCES})
    if(_rd_src MATCHES "RD_ProcessorSwapper(Editor)?\\.cpp$")
        continue()
    endif()
    list(APPEND RD_SOURCES "${_RD_ROOT}/${_rd_src}")
endforeach()
unset(SOURCES)

set(RD_INCLUDE_DIRS "${_RD_ROOT}/SOURCE")   # rd_dsp include now comes via RD_DSP target
unset(_RD_ROOT)
```

Update the file's header comment block to drop rd_dsp claims.

### Consumer plugins (PULSARELLO, RD_SYNTH, future)

New pattern in their `CMakeLists.txt`:

```cmake
set(RD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/RD)

# rd_dsp first — provides the RD_DSP target RD's source files compile against.
set(RD_DSP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(RD_DSP_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
add_subdirectory(${RD_DIR}/SUBMODULES/RD_DSP)

# RD wrapper sources (rd_dsp not bundled — link instead).
include(${RD_DIR}/CMAKE/CONSUMER_SOURCES.cmake)

target_sources(${PROJECT_NAME} PRIVATE ${RD_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${RD_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE RD_DSP)
```

### `init_plugin_repo` skill

Update the boilerplate `CMakeLists.txt` snippet so newly scaffolded plugins emit the pattern above. Update the Submodules section of the boilerplate `.claude/CLAUDE.md` to mention rd_dsp arriving via RD's submodule tree and being consumed via `add_subdirectory` + link.

## Prerequisite work in rd_dsp

rd_dsp currently exposes `BUILD_TESTS` and `BUILD_STANDALONE` as plain option names. Both collide with names every consumer also wants to use. Rename in rd_dsp to:

- `RD_DSP_BUILD_TESTS` (default ON only when rd_dsp is the top-level CMake project)
- `RD_DSP_BUILD_STANDALONE` (same default rule)

Detect "top-level" via `if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)`. When embedded, both default OFF — consumers don't have to remember to suppress them.

Update rd_dsp's HELPER_SCRIPTS that pass `-DBUILD_TESTS=...` / `-DBUILD_STANDALONE=...`. Update its `.claude/CLAUDE.md` build-commands section. Commit + push from inside the rd_dsp repo.

This step is small but is the unblocker — once rd_dsp uses prefixed flag names, the consumer-side `set(... CACHE BOOL ... FORCE)` lines are nice-to-have rather than required.

## Execution order

1. **rd_dsp** — rename options (`BUILD_TESTS` → `RD_DSP_BUILD_TESTS`, `BUILD_STANDALONE` → `RD_DSP_BUILD_STANDALONE`), flip defaults so embedded use is OFF, fix HELPER_SCRIPTS, update its `.claude/CLAUDE.md`. Commit + push from rd_dsp's own repo (NOT from inside RD's submodule checkout — the submodule pointer in RD gets bumped in step 2).
2. **RD** — bump rd_dsp submodule pointer to the commit from step 1. Rewrite RD's `CMakeLists.txt` per "End state" above. Delete `CMAKE/RD_DSP_SOURCES.cmake`. Rename RD's `BUILD_TESTS` to `RD_BUILD_TESTS`. Update `CONSUMER_SOURCES.cmake` to drop rd_dsp. Update RD HELPER_SCRIPTS + `.claude/CLAUDE.md`. Build, run ctest, confirm green. Commit + push.
3. **PULSARELLO** — bump RD submodule pointer. Apply the consumer-plugin pattern above. Build VST3 + Tests, confirm 30 `rd_dsp::*` unresolved externals are gone. Commit + push.
4. **RD_SYNTH** — bump RD submodule pointer. Apply same pattern. Build, confirm still green. Commit + push.
5. **`init_plugin_repo` skill** — update SKILL.md boilerplate. Sanity-check by scaffolding a throwaway plugin and configuring it.

## Risks / gotchas

- **Two `add_subdirectory(rd_dsp)` calls in one build** — happens when consumer plugin adds rd_dsp directly AND RD's CMakeLists also tries to. The `if(NOT TARGET RD_DSP)` guard in RD's CMakeLists handles this — the consumer wins, RD picks up the existing target.
- **rd_dsp's `target_include_directories(... PUBLIC SOURCE)`** — PUBLIC propagation is what makes `target_link_libraries(consumer PRIVATE RD_DSP)` carry the include dir over. Don't change it to PRIVATE.
- **`/W4` on rd_dsp** — applied with PRIVATE, doesn't leak. Safe.
- **Catch2 FetchContent duplication** — both rd_dsp and consumer plugins fetch Catch2 v3. Keeping `RD_DSP_BUILD_TESTS=OFF` for embedded use avoids the dup. If it ever happens, FetchContent dedupes by name (`Catch2`).
- **macOS universal-binary block** at the top of rd_dsp's CMakeLists sets `CMAKE_OSX_ARCHITECTURES` only when `$ENV{CI}` is set and `CMAKE_BUILD_TYPE STREQUAL Release` — harmless under embedded use. Leave it.
- **JUCE's plugin filter** — RD's CMakeLists historically pulled rd_dsp as loose sources so JUCE's plugin macros applied uniformly. Linking rd_dsp as a static lib should be fine since rd_dsp has zero JUCE dependency and its TUs don't participate in the plugin macros. Verify by checking that VST3 + Standalone still build clean after step 2.

## Verification after each step

- `cmake -B BUILD -DRD_BUILD_TESTS=ON` — clean configure, no warning about deprecated/unknown flags.
- `cmake --build BUILD --target Tests` — links, no `rd_dsp::*` unresolved externals.
- `ctest` — green.
- For plugin repos: VST3 + Standalone targets also link.

## Files touched (RD side only — what this repo will commit)

- `RD/CMakeLists.txt` — rewrite rd_dsp consumption + rename `BUILD_TESTS` → `RD_BUILD_TESTS`.
- `RD/CMAKE/CONSUMER_SOURCES.cmake` — drop rd_dsp section.
- `RD/CMAKE/RD_DSP_SOURCES.cmake` — delete.
- `RD/.claude/CLAUDE.md` — update build-command examples, drop references to `RD_DSP_SOURCES.cmake`.
- `RD/SUBMODULES/RD_DSP` — submodule pointer bump (from step 1's commit).
- Any RD HELPER_SCRIPTS that pass `-DBUILD_TESTS=...` — switch to `-DRD_BUILD_TESTS=...`.
