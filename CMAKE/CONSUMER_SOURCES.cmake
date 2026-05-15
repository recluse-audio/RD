# Entry point for downstream plugins consuming RD as a submodule.
#
# Hand-maintained: NOT touched by regenSource.py (which only rewrites
# SOURCES.cmake / TESTS.cmake). Safe to edit by hand.
#
# Produces:
#   RD_SOURCES        — absolute paths to RD's wrapper .cpp/.h files (RD's own
#                       SOURCE/ tree), with RD_ProcessorSwapper(Editor).cpp
#                       filtered out (consumers define their own
#                       createPluginFilter).
#   RD_INCLUDE_DIRS   — include dir(s) needed to compile RD_SOURCES.
#
# rd_dsp is NOT bundled here. It ships its own CMakeLists.txt that builds the
# RD_DSP static library. Consumers must add_subdirectory it and link:
#
#   set(RD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/SUBMODULES/RD)
#   add_subdirectory(${RD_DIR}/SUBMODULES/RD_DSP)   # produces RD_DSP target
#   include(${RD_DIR}/CMAKE/CONSUMER_SOURCES.cmake) # RD wrapper sources only
#   target_sources(${PROJECT_NAME} PRIVATE ${RD_SOURCES})
#   target_include_directories(${PROJECT_NAME} PRIVATE ${RD_INCLUDE_DIRS})
#   target_link_libraries(${PROJECT_NAME} PRIVATE RD_DSP)

set(_RD_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")

include(${CMAKE_CURRENT_LIST_DIR}/SOURCES.cmake)

set(RD_SOURCES "")

foreach(_rd_src ${SOURCES})
    # Skip RD_ProcessorSwapper(Editor).cpp — they define createPluginFilter,
    # which would collide with the consumer plugin's own definition.
    if(_rd_src MATCHES "RD_ProcessorSwapper(Editor)?\\.cpp$")
        continue()
    endif()
    list(APPEND RD_SOURCES "${_RD_ROOT}/${_rd_src}")
endforeach()

unset(SOURCES)

# rd_dsp's include dir comes from the RD_DSP target's PUBLIC propagation.
set(RD_INCLUDE_DIRS "${_RD_ROOT}/SOURCE")

unset(_RD_ROOT)
