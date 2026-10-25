# Qt 6.8 LTS (spec §3.1, §3.8): Core, Gui, Widgets, OpenGL, OpenGLWidgets, Svg, Concurrent, Test — LGPLv3,
# dynamically linked. scripts/build.ps1 puts the Qt prefix on CMAKE_PREFIX_PATH.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Gui Widgets OpenGL OpenGLWidgets Svg Concurrent Test)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

# Where Qt's own binaries and plugins live, for the tests' environment and for windeployqt.
get_target_property(AVLOOP_QMAKE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(AVLOOP_QT_BIN "${AVLOOP_QMAKE}" DIRECTORY)
get_filename_component(AVLOOP_QT_PREFIX "${AVLOOP_QT_BIN}" DIRECTORY)
set(AVLOOP_QT_PLUGINS "${AVLOOP_QT_PREFIX}/plugins")
