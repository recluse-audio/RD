cmake_minimum_required(VERSION 3.24.1)

set(PROJECT_NAME "RD")

# Read version
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" CURRENT_VERSION)
string(STRIP "${CURRENT_VERSION}" CURRENT_VERSION)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION} LANGUAGES C CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_XCODE_GENERATE_SCHEME OFF)

# macOS: universal binary on CI Release builds
if ((DEFINED ENV{CI}) AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
    message("Building for Apple Silicon and x86_64")
    set(CMAKE_OSX_ARCHITECTURES arm64 x86_64)
endif()

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Support macOS down to High Sierra")

set_property(GLOBAL PROPERTY USE_FOLDERS YES)
option(JUCE_ENABLE_MODULE_SOURCE_GROUPS "Show all module sources in IDE projects" ON)

# Detect top-level vs embedded
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_RD_IS_TOPLEVEL ON)
else()
    set(_RD_IS_TOPLEVEL OFF)
endif()

# Submodule-in-parent guardrail: refuse standalone build from inside a parent
# plugin's SUBMODULES/ dir (JUCE etc. aren't there — parent must drive the build).
if(_RD_IS_TOPLEVEL)
    get_filename_component(_RD_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
    get_filename_component(_RD_PARENT_NAME "${_RD_PARENT_DIR}" NAME)
    if(_RD_PARENT_NAME STREQUAL "SUBMODULES")
        message(FATAL_ERROR
            "\n"
            "============================================================\n"
            "  RD configured standalone from inside another project's\n"
            "  SUBMODULES/ folder. JUCE submodule won't be available.\n"
            "============================================================\n"
            "  RD lives at: ${CMAKE_CURRENT_SOURCE_DIR}\n"
            "  Parent dir:  ${_RD_PARENT_DIR}\n"
            "\n"
            "  Build the parent project from its root, or clone RD as\n"
            "  its own checkout for standalone work.\n"
            "============================================================\n"
        )
    endif()
endif()

# /FS prevents PDB contention when MSVC compiles JUCE modules in parallel
if(MSVC)
    add_compile_options(/FS)
endif()

# JUCE only when top-level (for the Tests executable). Consumers provide JUCE.
if(_RD_IS_TOPLEVEL)
    add_subdirectory(SUBMODULES/JUCE)
endif()

# rd_dsp — provides the RD_DSP static library. Guard so a parent that already
# added rd_dsp wins (RD's INTERFACE sources then compile against the parent's
# RD_DSP target).
if(NOT TARGET RD_DSP)
    add_subdirectory(SUBMODULES/RD_DSP)
endif()

#==============================================================================
# RD INTERFACE library
#
# Sources are exposed via INTERFACE so they compile into the consumer plugin
# target — that lets JUCE's plugin filter apply (Oboe, headless VST3 hosting,
# harfbuzz-subset don't leak). RD never builds a plugin or static lib of its
# own; develop RD via a thin scratch plugin sub-project.
#==============================================================================
include(CMAKE/SOURCES.cmake)

set(_RD_INTERFACE_SOURCES "")
foreach(_rd_src ${SOURCES})
    list(APPEND _RD_INTERFACE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/${_rd_src}")
endforeach()
unset(SOURCES)

add_library(RD INTERFACE)
target_sources(RD INTERFACE ${_RD_INTERFACE_SOURCES})
target_include_directories(RD INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/SOURCE)
target_compile_features(RD INTERFACE cxx_std_20)
target_link_libraries(RD INTERFACE RD_DSP)

#==============================================================================
# Debug Logging Options
#==============================================================================
option(RD_DEBUG_ALL              "Enable all RD debug logging"          OFF)
option(RD_DEBUG_GRAIN_CREATION   "Enable grain creation debug logging"  OFF)
option(RD_DEBUG_GRAIN_PROCESSING "Enable grain processing debug logging" OFF)
option(RD_DEBUG_PITCH_DETECTION  "Enable pitch detection debug logging" OFF)
option(RD_DEBUG_SAMPLE_DETAIL    "Enable sample-level debug logging"    OFF)
option(RD_DEBUG_OUTPUT_STATS     "Enable output statistics logging"     OFF)
option(RD_DEBUG_GAIN_PROCESSING  "Enable gain processing debug logging" OFF)

target_compile_definitions(RD INTERFACE
    RD_DEBUG_ALL=$<BOOL:${RD_DEBUG_ALL}>
    RD_DEBUG_GRAIN_CREATION=$<BOOL:${RD_DEBUG_GRAIN_CREATION}>
    RD_DEBUG_GRAIN_PROCESSING=$<BOOL:${RD_DEBUG_GRAIN_PROCESSING}>
    RD_DEBUG_PITCH_DETECTION=$<BOOL:${RD_DEBUG_PITCH_DETECTION}>
    RD_DEBUG_SAMPLE_DETAIL=$<BOOL:${RD_DEBUG_SAMPLE_DETAIL}>
    RD_DEBUG_OUTPUT_STATS=$<BOOL:${RD_DEBUG_OUTPUT_STATS}>
    RD_DEBUG_GAIN_PROCESSING=$<BOOL:${RD_DEBUG_GAIN_PROCESSING}>
)

# Intel IPP on Windows — propagates to consumer plugin if found.
if(MSVC)
    find_package(IPP)
    if(IPP_FOUND)
        target_link_libraries(RD INTERFACE IPP::ipps IPP::ippcore IPP::ippi IPP::ippcv)
        target_compile_definitions(RD INTERFACE PAMPLEJUCE_IPP=1)
        message("IPP LIBRARIES FOUND")
    else()
        message("IPP LIBRARIES *NOT* FOUND")
    endif()
endif()

#==============================================================================
# Version Header Generation — regenerate at configure time so embedded
# consumers don't need to wire a custom target into RD's INTERFACE library.
#==============================================================================
find_package(Python3 REQUIRED COMPONENTS Interpreter)

execute_process(
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/PLUGIN_SCRIPTS/HELPER_SCRIPTS/update_version.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt"
            "${CMAKE_CURRENT_SOURCE_DIR}/SOURCE/Util/Version.h"
    RESULT_VARIABLE _rd_version_rc
)
if(NOT _rd_version_rc EQUAL 0)
    message(WARNING "RD: update_version.py failed (rc=${_rd_version_rc})")
endif()

# Compiler color diagnostics
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

#==============================================================================
# Tests — only when RD is the top-level project. Consumers build their own.
#==============================================================================
if(_RD_IS_TOPLEVEL)
    option(BUILD_TESTS "Build RD unit tests" ON)

    if(BUILD_TESTS)
        include(FetchContent)
        FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_PROGRESS TRUE
            GIT_SHALLOW TRUE
            GIT_TAG v3.1.0
        )
        FetchContent_MakeAvailable(Catch2)

        enable_testing()

        # juce_add_console_app (not add_executable) so JUCE generates its
        # JuceLibraryCode wrapper TUs (include_juce_graphics.cpp etc.) and
        # marks the target so raw module .cpp files aren't injected as
        # INTERFACE_SOURCES — without this the JUCE 8 module .cpps compile
        # standalone and fail (int64/Graphics/jassert all undefined).
        juce_add_console_app(Tests PRODUCT_NAME "Tests")
        include(CMAKE/TESTS.cmake)
        target_sources(Tests PRIVATE ${TEST_SOURCES})

        target_compile_features(Tests PRIVATE cxx_std_20)

        if(MSVC)
            target_compile_options(Tests PRIVATE /wd4244 /FS /bigobj)
        endif()

        target_include_directories(Tests PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${CMAKE_CURRENT_SOURCE_DIR}/TESTS
        )

        target_link_libraries(Tests PRIVATE
            RD
            Catch2::Catch2WithMain
            juce::juce_audio_utils
            juce::juce_audio_processors
            juce::juce_dsp
            juce::juce_recommended_config_flags
            juce::juce_recommended_warning_flags
        )

        target_compile_definitions(Tests PRIVATE
            JucePlugin_Name="RD"
            JucePlugin_IsSynth=1
            JucePlugin_WantsMidiInput=1
            JucePlugin_ProducesMidiOutput=0
            JucePlugin_IsMidiEffect=0
            JUCE_WEB_BROWSER=0
            JUCE_USE_CURL=0
            JUCE_MODAL_LOOPS_PERMITTED=1
        )

        include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
        catch_discover_tests(Tests)
    endif()
endif()
