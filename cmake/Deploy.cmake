# Runtime layout of build/bin: the FFmpeg DLLs are copied beside the binaries after each build of a target that
# links FFmpeg; Qt's DLLs come from windeployqt (scripts/build.ps1 for builds, scripts/deploy.ps1 for the installer).
function(avloop_copy_ffmpeg_dlls target)
    foreach(_dll IN LISTS FFMPEG_DLLS)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_dll}" "$<TARGET_FILE_DIR:${target}>"
            VERBATIM)
    endforeach()
endfunction()

# The environment a test runs in: Qt's and FFmpeg's DLLs on PATH, Qt's plugins, the offscreen platform unless the
# test needs a real display (GL tests pass DISPLAY_PLATFORM).
function(avloop_test_environment test)
    cmake_parse_arguments(ARG "REAL_DISPLAY" "" "" ${ARGN})
    set(_path "PATH=path_list_prepend:${AVLOOP_QT_BIN}" "PATH=path_list_prepend:${FFMPEG_BIN_DIR}")
    set_property(TEST ${test} PROPERTY ENVIRONMENT_MODIFICATION ${_path})
    set(_env "QT_PLUGIN_PATH=${AVLOOP_QT_PLUGINS}")
    if(NOT ARG_REAL_DISPLAY)
        list(APPEND _env "QT_QPA_PLATFORM=offscreen")
    endif()
    set_property(TEST ${test} PROPERTY ENVIRONMENT ${_env})
endfunction()
