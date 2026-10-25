# FFmpeg 7.1, LGPL shared build (spec §3.1, §3.8), fetched by scripts/fetch-deps.ps1 into third_party/ffmpeg.
# Gives the imported targets FFmpeg::avformat, avcodec, avutil, swscale, avfilter and the list of DLLs to copy.
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/third_party/ffmpeg" CACHE PATH "FFmpeg LGPL shared build (bin, include, lib)")

find_path(FFMPEG_INCLUDE_DIR libavcodec/avcodec.h PATHS "${FFMPEG_ROOT}/include" NO_DEFAULT_PATH)

set(_ffmpeg_components avformat avcodec avutil swscale avfilter)
set(FFMPEG_DLLS "")
foreach(_c IN LISTS _ffmpeg_components)
    find_library(FFMPEG_${_c}_LIBRARY NAMES ${_c} PATHS "${FFMPEG_ROOT}/lib" NO_DEFAULT_PATH)
    file(GLOB _dll "${FFMPEG_ROOT}/bin/${_c}-*.dll")
    if(FFMPEG_${_c}_LIBRARY AND _dll AND NOT TARGET FFmpeg::${_c})
        add_library(FFmpeg::${_c} SHARED IMPORTED)
        set_target_properties(FFmpeg::${_c} PROPERTIES
            IMPORTED_IMPLIB "${FFMPEG_${_c}_LIBRARY}"
            IMPORTED_LOCATION "${_dll}"
            INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIR}")
        list(APPEND FFMPEG_DLLS ${_dll})
    endif()
    list(APPEND _ffmpeg_required FFMPEG_${_c}_LIBRARY)
endforeach()
# swresample is no AvLoop dependency, but avcodec and avfilter link to it.
file(GLOB _swr "${FFMPEG_ROOT}/bin/swresample-*.dll")
list(APPEND FFMPEG_DLLS ${_swr})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFMPEG_INCLUDE_DIR ${_ffmpeg_required}
    FAIL_MESSAGE "FFmpeg not found in ${FFMPEG_ROOT} — run scripts\\fetch-deps.ps1")
set(FFMPEG_BIN_DIR "${FFMPEG_ROOT}/bin")
