# Compiler rules for every AvLoop target (spec §2.7, §3.1): MSVC v143, /W4 /WX /permissive- /utf-8.
# Third-party headers (Qt's imported targets, FFmpeg) are external and their warnings never fail the build.
if(NOT MSVC)
    message(FATAL_ERROR "AvLoop builds with MSVC (Visual Studio 2022, v143) only")
endif()

add_library(avloop_warnings INTERFACE)
target_compile_options(avloop_warnings INTERFACE
    /W4 /WX /permissive- /utf-8 /Zc:__cplusplus /Zc:preprocessor /EHsc
    /external:anglebrackets /external:W0)
target_compile_definitions(avloop_warnings INTERFACE
    NOMINMAX WIN32_LEAN_AND_MEAN UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)

if(AVLOOP_ASAN)
    target_compile_options(avloop_warnings INTERFACE /fsanitize=address /Zi)
    target_link_options(avloop_warnings INTERFACE /INCREMENTAL:NO /DEBUG)
endif()
