# Entry point for downstream plugins consuming RD as a submodule.
#
# Hand-maintained: NOT touched by regenSource.py (which only rewrites
# SOURCES.cmake / TESTS.cmake). Safe to edit by hand.
#
# Produces:
#   RD_SOURCES        — absolute paths to all RD wrapper + rd_dsp .cpp/.h files,
#                       with RD_ProcessorSwapper(Editor).cpp filtered out
#                       (consumers define their own createPluginFilter).
#   RD_INCLUDE_DIRS   — include dirs needed to compile RD_SOURCES.
#
# Usage from a consumer plugin's CMakeLists.txt:
#   set(RD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/RD)
#   include(${RD_DIR}/CMAKE/CONSUMER_SOURCES.cmake)
#   target_sources(${PROJECT_NAME} PRIVATE ${RD_SOURCES})
#   target_include_directories(${PROJECT_NAME} PRIVATE ${RD_INCLUDE_DIRS})

set(_RD_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

# Pull both source lists. SOURCES.cmake is regen-script output (RD's own
# SOURCE/ tree); RD_DSP_SOURCES.cmake is hand-maintained (rd_dsp submodule).
include(${CMAKE_CURRENT_LIST_DIR}/SOURCES.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/RD_DSP_SOURCES.cmake)

set(RD_SOURCES "")

foreach(_rd_src ${SOURCES})
    # Skip RD_ProcessorSwapper(Editor).cpp — they define createPluginFilter,
    # which would collide with the consumer plugin's own definition.
    if(_rd_src MATCHES "RD_ProcessorSwapper(Editor)?\\.cpp$")
        continue()
    endif()
    list(APPEND RD_SOURCES "${_RD_ROOT}/${_rd_src}")
endforeach()

foreach(_rd_dsp_src ${RD_DSP_SOURCES})
    list(APPEND RD_SOURCES "${_RD_ROOT}/${_rd_dsp_src}")
endforeach()

# Clean up the intermediate lists so consumers don't accidentally use them.
unset(SOURCES)
unset(RD_DSP_SOURCES)

set(RD_INCLUDE_DIRS
    "${_RD_ROOT}/SOURCE"
    "${_RD_ROOT}/SUBMODULES/RD_DSP/SOURCE"
)

unset(_RD_ROOT)
