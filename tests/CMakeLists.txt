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
