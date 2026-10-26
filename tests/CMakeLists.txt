# AvLoop's unit tests (spec §14.1): Qt Test executables, run by `ctest -R avloop`. Each test runs with Qt's and
# FFmpeg's DLLs on PATH and the offscreen platform unless it needs a real display (labelled gpu).

function(avloop_add_test name)
    cmake_parse_arguments(ARG "GPU;RESOURCES" "" "SOURCES;LIBS" ${ARGN})
    set(_res "")
    if(ARG_RESOURCES)
        set(_res ${CMAKE_SOURCE_DIR}/resources/resources.qrc)
    endif()
    add_executable(test-${name} ${ARG_SOURCES} ${_res})
    target_link_libraries(test-${name} PRIVATE Qt6::Test ${ARG_LIBS} avloop_warnings)
    add_test(NAME avloop-${name} COMMAND test-${name})
    if(ARG_GPU)
        avloop_test_environment(avloop-${name} REAL_DISPLAY)
        set_tests_properties(avloop-${name} PROPERTIES LABELS gpu)
    else()
        avloop_test_environment(avloop-${name})
    endif()
endfunction()

avloop_add_test(theme RESOURCES SOURCES test-theme.cpp LIBS avloop_ui)
avloop_add_test(icons RESOURCES SOURCES test-icons.cpp LIBS avloop_ui)
avloop_add_test(aspect SOURCES test-aspect.cpp LIBS avloop_ui)
avloop_add_test(mask SOURCES test-mask.cpp LIBS avloop_render avloop_core)
avloop_add_test(renderer-cpu SOURCES test-renderer-cpu.cpp RenderCases.h LIBS avloop_render)
avloop_add_test(timecode SOURCES test-timecode.cpp LIBS avloop_ui)
foreach(_t geometry commands project-json loopmap timemap presets)
    avloop_add_test(${_t} SOURCES test-${_t}.cpp LIBS avloop_core)
endforeach()

# The media tests read the lab's samples (tools/media/make-samples.mjs); a missing sample skips its case.
foreach(_t probe decoder-seek memory interpolator export)
    avloop_add_test(${_t} SOURCES test-${_t}.cpp Samples.h LIBS avloop_media)
    target_compile_definitions(test-${_t} PRIVATE AVLOOP_SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples")
    target_include_directories(test-${_t} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
endforeach()
# The export job on a sample, on the CPU (QGuiApplication on the offscreen platform).
avloop_add_test(export-job SOURCES test-export-job.cpp Samples.h LIBS avloop_export)
target_compile_definitions(test-export-job PRIVATE AVLOOP_SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples")
target_include_directories(test-export-job PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# The Debug leak gate: the CRT debug heap around repeated cycles (skips in Release).
avloop_add_test(leaks SOURCES test-leaks.cpp Samples.h LIBS avloop_export)
target_compile_definitions(test-leaks PRIVATE AVLOOP_SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples")
target_include_directories(test-leaks PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
# test-export reads the written file's packets itself.
target_link_libraries(test-export PRIVATE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil)

# GPU/CPU parity needs an OpenGL 3.3 context: a real display, never CI (labelled gpu).
avloop_add_test(renderer GPU SOURCES test-renderer.cpp RenderCases.h Samples.h LIBS avloop_render)
target_compile_definitions(test-renderer PRIVATE AVLOOP_SAMPLES_DIR="${CMAKE_SOURCE_DIR}/samples")
target_include_directories(test-renderer PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(test-renderer-cpu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
