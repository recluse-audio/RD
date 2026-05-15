# Plan: consume RD via `add_subdirectory` + link, drop loose-source pattern

## Why

Consumer plugins (PULSARELLO, RD_SYNTH, future) used to pull RD into their
build as **loose source files**:

```cmake
include(${RD_DIR}/CMAKE/SOURCES.cmake)
foreach(_rd_src ${SOURCES})
    list(APPEND RD_SOURCES "${RD_DIR}/${_rd_src}")
endforeach()
target_sources(${PROJECT_NAME} PRIVATE ${RD_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${RD_DIR}/SOURCE ${RD_DIR}/SUBMODULES/RD_DSP/SOURCE)
```

That was a workaround for two real problems:

1. **JUCE plugin filter.** If RD were built as a static library that links
   `juce::juce_audio_processors` etc., JUCE doesn't apply its plugin filter
   (only `juce_add_plugin` targets get that). Oboe / headless VST3 host /
   harfbuzz-subset `.cpp` files would compile into the static lib verbatim
   and leak into every consumer.
2. **rd_dsp inclusion.** RD pulled rd_dsp as loose sources too via a
   hand-maintained `CMAKE/RD_DSP_SOURCES.cmake`, which consumers also had
   to manage.

Both problems are solved cleanly by making RD an **INTERFACE library** that
exposes its sources via INTERFACE, and by having RD do `add_subdirectory`
on rd_dsp internally.

## End state

### `RD/CMakeLists.txt`

- No `BUILD_AS_PLUGIN`. No `juce_add_plugin`. No `add_library(RD STATIC)`.
- `add_subdirectory(SUBMODULES/JUCE)` happens **only when RD is the top-level
  project** (needed for the `Tests` executable). Embedded consumers add JUCE
  themselves first.
- `add_subdirectory(SUBMODULES/RD_DSP)` always (guarded by `if(NOT TARGET RD_DSP)`
  so a parent that already added rd_dsp wins).
- `add_library(RD INTERFACE)` with:
  - `target_sources(RD INTERFACE <all of RD's SOURCE/...>)` — every file from
    `CMAKE/SOURCES.cmake`. Nothing filtered (no `createPluginFilter` lives in
    RD's source tree anymore — it was deleted from `RD_ProcessorSwapper.cpp`).
  - `target_include_directories(RD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/SOURCE)`
  - `target_link_libraries(RD INTERFACE RD_DSP)` — rd_dsp's PUBLIC include dir
    propagates through.
  - `target_compile_definitions(RD INTERFACE RD_DEBUG_*=$<BOOL:${...}>)`.
  - On MSVC, IPP linkage + `PAMPLEJUCE_IPP=1` go on the INTERFACE if found.
- `Tests` target only when `_RD_IS_TOPLEVEL` is true. Links `RD`,
  `Catch2::Catch2WithMain`, the JUCE bits it needs.
- Version header generated at **configure time** via `execute_process` (not a
  custom target), so embedded consumers don't need to wire up
  `add_dependencies` on an INTERFACE library.

### Consumer plugins (PULSARELLO, RD_SYNTH, future)

```cmake
add_subdirectory(SUBMODULES/JUCE)
add_subdirectory(SUBMODULES/RD)   # provides RD INTERFACE + RD_DSP

juce_add_plugin(${PROJECT_NAME} ...)

include(CMAKE/SOURCES.cmake)
target_sources(${PROJECT_NAME} PRIVATE ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/SOURCE)
target_link_libraries(${PROJECT_NAME} PRIVATE RD ${JUCE_DEPENDENCIES} ...)
```

Tests target links `RD` too — RD's INTERFACE sources get compiled into the
test executable, and `RD_DSP` is linked transitively.

### Deletions

- `RD/CMAKE/RD_DSP_SOURCES.cmake` — already gone (replaced by add_subdirectory).
- `RD/CMAKE/CONSUMER_SOURCES.cmake` — gone (INTERFACE library replaces it).
- `createPluginFilter()` from `SOURCE/PROCESSORS/RD_ProcessorSwapper.cpp` —
  gone. Consumer plugins define their own.

### `init_plugin_repo` skill

Boilerplate updated:
- Consumer pattern is `add_subdirectory(SUBMODULES/RD)` + `target_link_libraries(... RD)`.
- Submodule init must be **recursive** because RD's CMakeLists does
  `add_subdirectory(SUBMODULES/RD_DSP)`.

## Tradeoff accepted

RD no longer builds a VST3 / Standalone of its own. Develop RD via a thin
scratch sub-project (one of the existing plugin repos works, or scaffold a
new one with `/init_plugin_repo`). RD top-level builds only the `Tests`
executable.

## Risks / gotchas

- **JUCE added twice.** Top-level RD adds JUCE; embedded RD skips it. Guarded by
  `_RD_IS_TOPLEVEL`.
- **rd_dsp added twice.** Guarded by `if(NOT TARGET RD_DSP)`.
- **JUCE plugin filter.** Gone as a concern. RD's sources are INTERFACE; they
  compile into the consumer's `juce_add_plugin` target, so the filter applies.
- **Submodule recursion.** Consumers must `git submodule update --init --recursive`.
  RD's own CI / dev clones already do this.
- **Tests on RD top-level vs embedded.** RD's `Tests` target only builds when
  top-level. Consumers build their own tests.

## Files touched on this branch (RD side)

- `RD/CMakeLists.txt` — INTERFACE library rewrite.
- `RD/CMAKE/CONSUMER_SOURCES.cmake` — deleted.
- `RD/CMAKE/RD_DSP_SOURCES.cmake` — deleted (earlier step).
- `RD/SOURCE/PROCESSORS/RD_ProcessorSwapper.cpp` — `createPluginFilter` removed.

## Consumer-side changes (separate repos, separate commits)

- `PULSARELLO/CMakeLists.txt` — `add_subdirectory(SUBMODULES/RD)` + link `RD`.
- `RD_SYNTH/CMakeLists.txt` — same.
- `~/.claude/skills/init_plugin_repo/SKILL.md` — boilerplate updated.
