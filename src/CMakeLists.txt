# AvLoop's modules (spec §3.2): core (Qt Core only) → media (FFmpeg) → render → export → ui → app / cli.

add_library(avloop_core STATIC
    core/AppInfo.h core/AppInfo.cpp core/Rational.h
    core/Geometry.h core/Geometry.cpp core/VideoTransform.h core/VideoTransform.cpp
    core/CropShape.h core/CropShape.cpp core/MaskShape.h core/MaskShape.cpp core/ShapeCombine.h core/ShapeCombine.cpp core/ShapeAlign.h core/ShapeAlign.cpp core/VideoLayer.h core/VideoLayer.cpp core/Sequence.h core/Sequence.cpp core/SequenceEdit.h core/SequenceEdit.cpp core/SequenceView.h core/SequenceView.cpp core/AspectRatio.h core/AspectRatio.cpp core/Keyframes.h core/Keyframes.cpp
    core/Timecode.h core/Timecode.cpp core/TimeRange.h core/TimeRange.cpp
    core/LoopSettings.h core/LoopSettings.cpp core/LoopMap.h core/LoopMap.cpp core/TimeMap.h core/TimeMap.cpp
    core/QualitySettings.h core/QualitySettings.cpp core/ClipEffects.h core/ClipEffects.cpp
    core/Project.h core/Project.cpp core/ProjectEdit.cpp core/Commands.h core/ProjectJson.h core/ProjectJson.cpp core/ProjectSections.cpp
    core/ProjectMigrate.h core/ProjectMigrate.cpp core/Presets.h core/Presets.cpp core/AppSettings.h core/AppSettings.cpp)
target_include_directories(avloop_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/generated)
# Qt Gui only for QUndoStack/QUndoCommand, which Qt 6 moved there from Widgets (spec §11.2).
target_link_libraries(avloop_core PUBLIC Qt6::Core Qt6::Gui PRIVATE avloop_warnings)

add_library(avloop_media STATIC
    media/FfmpegPtr.h media/MediaInfo.h media/MediaProbe.h media/MediaProbe.cpp
    media/Decoder.h media/DecoderImpl.h media/Decoder.cpp media/DecoderSeek.cpp media/DecoderHw.cpp
    media/SourceNormalizer.h media/SourceNormalizer.cpp
    media/FrameCache.h media/FrameCache.cpp media/DiskCache.h media/DiskCache.cpp
    media/DecodeWorker.h media/DecodeWorker.cpp media/ThumbnailWorker.h media/ThumbnailWorker.cpp
    media/Interpolator.h media/Interpolator.cpp media/FrameIndex.h media/FrameIndex.cpp media/WorkerThread.h
    media/Encoder.h media/Encoder.cpp media/EncoderOptions.h media/EncoderOptions.cpp media/Muxer.h media/Muxer.cpp
    media/GifPalette.h media/GifPalette.cpp
    media/SystemMemory.h media/SystemMemory.cpp)
target_link_libraries(avloop_media
    PUBLIC avloop_core Qt6::Core
    PRIVATE FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale FFmpeg::avfilter dxgi psapi avloop_warnings)

add_library(avloop_render STATIC
    render/Shaders.h render/Shaders.cpp render/MaskBuilder.h render/MaskBuilder.cpp render/DistanceField.h render/DistanceField.cpp render/SequenceRenderer.h render/SequenceRenderer.cpp
    render/RenderJob.h render/RenderJob.cpp render/CpuRenderer.h render/CpuRenderer.cpp
    render/GlRenderer.h render/GlRenderer.cpp render/FrameRenderer.h render/FrameRenderer.cpp
    render/RenderService.h render/RenderService.cpp render/EffectsPass.h render/EffectsPass.cpp)
target_link_libraries(avloop_render PUBLIC avloop_core avloop_media Qt6::Gui Qt6::OpenGL
    PRIVATE Qt6::Concurrent avloop_warnings)

add_library(avloop_export STATIC
    export/FormatProfiles.h export/FormatProfiles.cpp export/OutputVerify.h export/OutputVerify.cpp
    export/ExportJob.h export/ExportJob.cpp export/SizeEstimator.h export/SizeEstimator.cpp
    export/LoopFinder.h export/LoopFinder.cpp export/ExportQueue.h export/ExportQueue.cpp)
target_link_libraries(avloop_export PUBLIC avloop_core avloop_media avloop_render
    PRIVATE Qt6::Concurrent FFmpeg::avformat FFmpeg::avcodec FFmpeg::avutil FFmpeg::swscale avloop_warnings)

add_library(avloop_ui STATIC
    ui/MainWindow.h ui/MainWindow.cpp ui/MainMenu.cpp ui/MainWindowActions.cpp ui/MainWindowCanvas.cpp
    ui/MainWindowTimeline.cpp ui/MainWindowPreview.cpp ui/MainWindowExport.cpp ui/MainWindowProject.cpp
    ui/dialogs/SettingsDialog.h ui/dialogs/SettingsDialog.cpp ui/dialogs/AboutDialog.h ui/dialogs/AboutDialog.cpp
    ui/dialogs/ShortcutOverlay.h ui/dialogs/ShortcutOverlay.cpp
    ui/export/ExportBar.h ui/export/ExportBar.cpp ui/export/LoopFinderWindow.h ui/export/LoopFinderWindow.cpp
    ui/MainWindowFinder.cpp ui/MainWindowLayers.cpp ui/MainWindowMonitor.cpp ui/MonitorComposite.h ui/panels/ProjectPanel.h ui/panels/ProjectPanel.cpp ui/MainWindowBatch.cpp ui/export/BatchWindow.h ui/export/BatchWindow.cpp
    ui/preview/PreviewWindow.h ui/preview/PreviewWindow.cpp ui/preview/PreviewWidgets.h ui/preview/PreviewWidgets.cpp
    ui/preview/PreviewPlayer.h ui/preview/PreviewPlayer.cpp ui/preview/PreviewEncode.cpp
    ui/WalkRunner.h ui/WalkRunner.cpp ui/WalkSteps.cpp
    ui/theme/Theme.h ui/theme/Theme.cpp
    ui/theme/IconFactory.h ui/theme/IconFactory.cpp
    ui/widgets/Toast.h ui/widgets/Toast.cpp
    ui/panels/Section.h ui/panels/Section.cpp
    ui/panels/SidePanel.h ui/panels/SidePanel.cpp ui/panels/CropPanel.cpp ui/panels/LoopPanel.cpp
    ui/panels/OutputPanel.cpp ui/panels/QualityPanel.cpp ui/panels/PresetPanel.cpp ui/panels/EffectsPanel.cpp ui/panels/ShapesPanel.cpp ui/panels/LayersPanel.cpp ui/widgets/ColourField.h ui/widgets/ColourField.cpp ui/widgets/ResetLabel.h ui/widgets/ResetLabel.cpp
    ui/canvas/DropZone.h ui/canvas/DropZone.cpp
    ui/canvas/Canvas.h ui/canvas/Canvas.cpp ui/canvas/CanvasView.cpp ui/canvas/CanvasGl.cpp ui/canvas/CanvasGlState.h ui/canvas/CanvasOverlay.cpp
    ui/tools/Tool.h ui/tools/ToolManager.h ui/tools/ToolManager.cpp
    ui/tools/SelectionTool.cpp ui/tools/ZoomTool.cpp ui/tools/HandTool.cpp
    ui/tools/CropTool.cpp ui/tools/CropHandles.h ui/tools/CropHandles.cpp ui/tools/ShapeTool.cpp ui/tools/PenTool.cpp ui/tools/PathEdit.h ui/tools/PathEdit.cpp ui/tools/ShapeEdit.h
    ui/tools/ShapeEdit.cpp ui/tools/InOutTool.cpp ui/tools/KeyframeTool.cpp
    ui/options/OptionsBar.h ui/options/OptionsBar.cpp ui/options/SelectionOptions.cpp ui/options/ZoomOptions.cpp
    ui/options/CropOptions.cpp ui/options/ShapeOptions.cpp ui/options/AlignBar.cpp
    ui/widgets/StepSpinBox.h ui/widgets/StepSpinBox.cpp ui/widgets/TimeEdit.h ui/widgets/TimeEdit.cpp
    ui/timeline/Transport.h ui/timeline/Transport.cpp
    ui/timeline/Timeline.h ui/timeline/Timeline.cpp ui/timeline/SequenceLanes.h ui/timeline/SequenceLanes.cpp ui/timeline/SequenceLanesInput.cpp ui/timeline/TimelinePaint.cpp ui/timeline/TimelineInput.cpp
    ui/timeline/TimelineScale.h ui/timeline/TimelineScale.cpp
    ui/timeline/PlaybackClock.h ui/timeline/PlaybackClock.cpp)
target_link_libraries(avloop_ui
    PUBLIC avloop_core avloop_media avloop_render avloop_export Qt6::Widgets Qt6::Svg Qt6::OpenGLWidgets Qt6::Test Qt6::Concurrent
    PRIVATE Qt6::GuiPrivate avloop_warnings) # GuiPrivate: the walk runner's application state (WalkRunner.cpp)

set(AVLOOP_RC "")
if(EXISTS ${CMAKE_SOURCE_DIR}/resources/app/avloop.ico)
    set(AVLOOP_RC ${CMAKE_SOURCE_DIR}/resources/app/avloop.rc)
endif()
add_executable(avloop WIN32 app/main.cpp app/Diagnostics.h app/Diagnostics.cpp ${CMAKE_SOURCE_DIR}/resources/resources.qrc
    ${AVLOOP_RC})
target_link_libraries(avloop PRIVATE avloop_ui dbghelp avloop_warnings)
avloop_copy_ffmpeg_dlls(avloop)

add_executable(avloop-cli cli/main.cpp cli/CliMedia.cpp cli/CliRender.cpp cli/CliExport.cpp cli/CliFind.cpp
               cli/CliSequence.cpp)
target_link_libraries(avloop-cli PRIVATE avloop_core avloop_media avloop_render avloop_export Qt6::Gui
    FFmpeg::avcodec FFmpeg::avformat FFmpeg::avutil avloop_warnings)
avloop_copy_ffmpeg_dlls(avloop-cli)

# A developer's tool, never shipped: renders resources/app/avloop.svg into the multi-size avloop.ico.
add_executable(avloop-iconbake ${CMAKE_SOURCE_DIR}/tools/icons/iconbake.cpp)
target_link_libraries(avloop-iconbake PRIVATE Qt6::Gui Qt6::Svg avloop_warnings)
