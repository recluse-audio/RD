# macOS Plugin Build Instructions

How to enable AU (Audio Unit) builds for an RD-style JUCE plugin on macOS, and how the build script driver discovers format targets.

## Background: the problem

`SUBMODULES/PLUGIN_SCRIPTS/HELPER_SCRIPTS/rebuild_all.py` builds every plugin format target in sequence. On macOS its `get_targets()` always appends `RD_AU` to the build list:

```python
# rebuild_all.py
def get_targets() -> list[str]:
    targets = [
        f"{PLUGIN_NAME}",
        f"{PLUGIN_NAME}_Standalone",
        f"{PLUGIN_NAME}_VST3",
    ]
    if sys.platform == "darwin":
        targets.append(f"{PLUGIN_NAME}_AU")
    return targets
```

But `juce_add_plugin` only generates the `<plugin>_AU` CMake target if `AU` is listed in its `FORMATS`. If it isn't, you get:

```
make: *** No rule to make target `RD_AU'.  Stop.
Build failed with exit code 2
```

The traceback that follows is just Python re-raising the failed `subprocess.run` — the real error is the line above it. Always read the build output above the Python traceback.

## The fix

Add `AU` to the `FORMATS` list passed to `juce_add_plugin`.

### Reference: Pulsar_2.1 (flat, single-target build)

Pulsar lists all three formats unconditionally:

```cmake
# Pulsar_2.1/CMakeLists.txt:37
set(FORMATS VST3 AU Standalone)

juce_add_plugin(Pulsar
    ...
    FORMATS ${FORMATS}
    ...
)
```

That's the entire AU configuration — no plist tweaks, no codesigning hooks, no hardened-runtime flags in CMake. JUCE handles the AU bundle layout, Info.plist, and component registration on its own.

### RD (plugin/static-library split)

RD's CMakeLists.txt auto-detects whether it's the top-level project or a submodule, and only emits the plugin targets in the top-level case. AU should only be requested on Apple platforms (it's not valid on Windows/Linux), so the AU append is guarded:

```cmake
# RD/CMakeLists.txt — inside if(BUILD_AS_PLUGIN)
set(FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND FORMATS AU)
endif()
juce_add_plugin("${PROJECT_NAME}"
    ...
    FORMATS "${FORMATS}"
    ...
)
```

Two things to note vs. Pulsar:

1. The `if(APPLE)` guard exists because RD is intended to be cross-platform; Pulsar's CMakeLists is mac-only in practice.
2. The block sits *inside* the `BUILD_AS_PLUGIN` branch — when RD is included as a static library submodule, no plugin targets are generated at all.

## After changing FORMATS: reconfigure

Editing `FORMATS` is a CMake-level change. The existing `BUILD/` cache was generated without AU, so cmake's target list is stale. You must **reconfigure**, not just rebuild:

```bash
cmake -B BUILD/ -DBUILD_TESTS=ON
```

…or run `rebuild_all.py --clean` to wipe and regenerate the build dir. After reconfigure, `RD_AU` will exist as a target and `rebuild_all.py` will succeed.

## Reading build failures from rebuild_all.py

The script wraps `cmake --build` in `subprocess.run(check=True)`, so any failure raises `CalledProcessError` and prints a Python traceback. The traceback itself is noise — the meaningful error is whatever cmake/clang/ld printed *before* the `+ /opt/homebrew/bin/cmake --build ...` line that triggered it. Always scroll up past the traceback.

## Checklist when adding AU support to a new RD-style project

- [ ] Add `AU` to `FORMATS` in CMakeLists.txt (guard with `if(APPLE)` if cross-platform).
- [ ] Confirm `juce_add_plugin` has a sane `BUNDLE_ID`, `PLUGIN_MANUFACTURER_CODE` (4 chars, one uppercase), and `PLUGIN_CODE` (4 chars, one uppercase) — AU validation rejects bad codes.
- [ ] Reconfigure the build dir (delete `BUILD/` or `cmake -B BUILD/` again).
- [ ] Build the `<PluginName>_AU` target; it lands in `BUILD/<PluginName>_artefacts/<Config>/AU/<PluginName>.component`.
- [ ] To test: copy or symlink the `.component` into `~/Library/Audio/Plug-Ins/Components/` and run `auval -a` or open it in an AU host.
