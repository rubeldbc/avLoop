# AvLoop's modules (spec §3.2): core (Qt Core only) → media (FFmpeg) → render → export → ui → app / cli.

add_library(avloop_core STATIC
    core/AppInfo.h core/AppInfo.cpp core/Rational.h)
target_include_directories(avloop_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)
target_link_libraries(avloop_core PUBLIC Qt6::Core PRIVATE avloop_warnings)

add_library(avloop_media STATIC
    media/FfmpegPtr.h media/MediaInfo.h media/MediaProbe.h media/MediaProbe.cpp
    media/Decoder.h media/DecoderImpl.h media/Decoder.cpp media/DecoderSeek.cpp media/DecoderHw.cpp
    media/SourceNormalizer.h media/SourceNormalizer.cpp
    media/FrameCache.h media/FrameCache.cpp media/DiskCache.h media/DiskCache.cpp
    media/DecodeWorker.h media/DecodeWorker.cpp
    media/SystemMemory.h media/SystemMemory.cpp)
target_link_libraries(avloop_media
    PUBLIC avloop_core Qt6::Core
    PRIVATE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale FFmpeg::avfilter dxgi psapi avloop_warnings)

add_library(avloop_ui STATIC
    ui/MainWindow.h ui/MainWindow.cpp ui/MainMenu.cpp ui/MainWindowActions.cpp
    ui/WalkRunner.h ui/WalkRunner.cpp ui/WalkSteps.cpp
    ui/theme/Theme.h ui/theme/Theme.cpp
    ui/theme/IconFactory.h ui/theme/IconFactory.cpp
    ui/widgets/Toast.h ui/widgets/Toast.cpp
    ui/panels/Section.h ui/panels/Section.cpp
    ui/panels/SidePanel.h ui/panels/SidePanel.cpp
    ui/canvas/DropZone.h ui/canvas/DropZone.cpp
    ui/timeline/Transport.h ui/timeline/Transport.cpp)
target_link_libraries(avloop_ui
    PUBLIC avloop_core avloop_media Qt6::Widgets Qt6::Svg Qt6::OpenGLWidgets Qt6::Test Qt6::Concurrent
    PRIVATE avloop_warnings)

set(AVLOOP_RC "")
if(EXISTS ${CMAKE_SOURCE_DIR}/resources/app/avloop.ico)
    set(AVLOOP_RC ${CMAKE_SOURCE_DIR}/resources/app/avloop.rc)
endif()
add_executable(avloop WIN32 app/main.cpp ${CMAKE_SOURCE_DIR}/resources/resources.qrc ${AVLOOP_RC})
target_link_libraries(avloop PRIVATE avloop_ui avloop_warnings)
avloop_copy_ffmpeg_dlls(avloop)

add_executable(avloop-cli cli/main.cpp cli/CliMedia.cpp)
target_link_libraries(avloop-cli PRIVATE avloop_core avloop_media Qt6::Gui
    FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil avloop_warnings)
avloop_copy_ffmpeg_dlls(avloop-cli)

# A developer's tool, never shipped: renders resources/app/avloop.svg into the multi-size avloop.ico.
add_executable(avloop-iconbake ${CMAKE_SOURCE_DIR}/tools/icons/iconbake.cpp)
target_link_libraries(avloop-iconbake PRIVATE Qt6::Gui Qt6::Svg avloop_warnings)
