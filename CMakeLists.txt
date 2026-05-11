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

# Detect: is RD being configured as top-level, but actually living inside
# another project's SUBMODULES/ folder? That means the user cd'd into the
# nested RD and tried to build it standalone — won't work, since JUCE is
# at the parent's SUBMODULES/JUCE, not RD's.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    get_filename_component(_RD_PARENT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" DIRECTORY)
    get_filename_component(_RD_PARENT_NAME "${_RD_PARENT_DIR}" NAME)
    if(_RD_PARENT_NAME STREQUAL "SUBMODULES")
        message(FATAL_ERROR
            "\n"
            "============================================================\n"
            "  YOU ARE TRYING TO BUILD RD SUBMODULE AS A PLUGIN,\n"
            "  INSIDE A PLUGIN. NOT GONNA HAPPEN.\n"
            "============================================================\n"
            "  RD lives at: ${CMAKE_CURRENT_SOURCE_DIR}\n"
            "  Parent dir:  ${_RD_PARENT_DIR}\n"
            "\n"
            "  To build the parent project, run the build from there.\n"
            "  To build RD standalone, clone it as its own repo.\n"
            "============================================================\n"
        )
    endif()
endif()

# /FS prevents PDB contention when MSVC compiles JUCE modules in parallel
if(MSVC)
    add_compile_options(/FS)
endif()

# JUCE submodule
add_subdirectory(SUBMODULES/JUCE)

# Auto-detect top-level vs submodule context
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(_RD_PLUGIN_DEFAULT ON)
else()
    set(_RD_PLUGIN_DEFAULT OFF)
endif()
option(BUILD_AS_PLUGIN "Build RD as a VST3/Standalone plugin" ${_RD_PLUGIN_DEFAULT})

if(BUILD_AS_PLUGIN)
    # Plugin target
    set(FORMATS VST3 Standalone)
    if(APPLE)
        list(APPEND FORMATS AU)
    endif()
    juce_add_plugin("${PROJECT_NAME}"
        COMPANY_NAME "Recluse-Audio"
        BUNDLE_ID com.recluseaudio.rd
        PLUGIN_MANUFACTURER_CODE Recl
        PLUGIN_CODE RDrd
        FORMATS "${FORMATS}"
        PRODUCT_NAME "${PROJECT_NAME}"
        COPY_PLUGIN_AFTER_BUILD FALSE
        VST3_CATEGORIES Fx
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    )
else()
    # Static library target for use as a submodule
    add_library("${PROJECT_NAME}" STATIC)
endif()

# Source files
include(CMAKE/SOURCES.cmake)
target_sources("${PROJECT_NAME}" PRIVATE ${SOURCES})

target_include_directories("${PROJECT_NAME}" PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/RD_DSP/SOURCE
)

target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_20)

if(MSVC)
    target_compile_options("${PROJECT_NAME}" PRIVATE
        /wd4244  # conversion warnings
    )
endif()

target_compile_definitions("${PROJECT_NAME}"
    PUBLIC
        BUILD_AS_PLUGIN=$<BOOL:${BUILD_AS_PLUGIN}>
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
    PRIVATE
        RD_DEBUG_ALL=$<BOOL:${RD_DEBUG_ALL}>
        RD_DEBUG_GRAIN_CREATION=$<BOOL:${RD_DEBUG_GRAIN_CREATION}>
        RD_DEBUG_GRAIN_PROCESSING=$<BOOL:${RD_DEBUG_GRAIN_PROCESSING}>
        RD_DEBUG_PITCH_DETECTION=$<BOOL:${RD_DEBUG_PITCH_DETECTION}>
        RD_DEBUG_SAMPLE_DETAIL=$<BOOL:${RD_DEBUG_SAMPLE_DETAIL}>
        RD_DEBUG_OUTPUT_STATS=$<BOOL:${RD_DEBUG_OUTPUT_STATS}>
        RD_DEBUG_GAIN_PROCESSING=$<BOOL:${RD_DEBUG_GAIN_PROCESSING}>
)

set(JUCE_DEPENDENCIES
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_opengl
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_plugin_client
    juce::juce_dsp
)

target_link_libraries("${PROJECT_NAME}"
    PRIVATE
        ${JUCE_DEPENDENCIES}
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Intel IPP on Windows
if(MSVC)
    find_package(IPP)
    if(IPP_FOUND)
        target_link_libraries("${PROJECT_NAME}" PUBLIC IPP::ipps IPP::ippcore IPP::ippi IPP::ippcv)
        message("IPP LIBRARIES FOUND")
        target_compile_definitions("${PROJECT_NAME}" PUBLIC PAMPLEJUCE_IPP=1)
    else()
        message("IPP LIBRARIES *NOT* FOUND")
    endif()
endif()

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

#==============================================================================
# Version Header Generation
#==============================================================================
set(VERSION_TXT    "${CMAKE_SOURCE_DIR}/VERSION.txt")
set(VERSION_HEADER "${CMAKE_SOURCE_DIR}/SOURCE/Util/Version.h")

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_target(update_version_header
    COMMAND ${Python3_EXECUTABLE}
            "${CMAKE_SOURCE_DIR}/SUBMODULES/PLUGIN_SCRIPTS/HELPER_SCRIPTS/update_version.py"
            "${VERSION_TXT}"
            "${VERSION_HEADER}"
    COMMENT "Updating version header"
)

add_dependencies("${PROJECT_NAME}" update_version_header)

# Compiler color diagnostics
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
endif()

#==============================================================================
# Tests (optional)
#==============================================================================
option(BUILD_TESTS "Build unit tests" ON)

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

    add_executable(Tests)
    include(CMAKE/TESTS.cmake)
    target_sources(Tests PRIVATE ${TEST_SOURCES} ${SOURCES})

    target_compile_features(Tests PRIVATE cxx_std_20)

    if(MSVC)
        target_compile_options(Tests PRIVATE
            /wd4244
            /FS
            /bigobj
        )
    endif()

    target_include_directories(Tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/RD_DSP/SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/TESTS
    )

    target_link_libraries(Tests PRIVATE
        Catch2::Catch2WithMain
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_dsp
    )

    target_compile_definitions(Tests PRIVATE
        JucePlugin_Name="RD"
        JucePlugin_WantsMidiInput=0
        JucePlugin_ProducesMidiOutput=0
        JucePlugin_IsMidiEffect=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_MODAL_LOOPS_PERMITTED=1
        RD_DEBUG_ALL=$<BOOL:${RD_DEBUG_ALL}>
        RD_DEBUG_GRAIN_CREATION=$<BOOL:${RD_DEBUG_GRAIN_CREATION}>
        RD_DEBUG_GRAIN_PROCESSING=$<BOOL:${RD_DEBUG_GRAIN_PROCESSING}>
        RD_DEBUG_PITCH_DETECTION=$<BOOL:${RD_DEBUG_PITCH_DETECTION}>
        RD_DEBUG_SAMPLE_DETAIL=$<BOOL:${RD_DEBUG_SAMPLE_DETAIL}>
        RD_DEBUG_OUTPUT_STATS=$<BOOL:${RD_DEBUG_OUTPUT_STATS}>
        RD_DEBUG_GAIN_PROCESSING=$<BOOL:${RD_DEBUG_GAIN_PROCESSING}>
    )

    include(${Catch2_SOURCE_DIR}/extras/Catch.cmake)
    catch_discover_tests(Tests)
endif()
