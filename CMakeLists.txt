cmake_minimum_required(VERSION 3.16)
project(RTPlotter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets PrintSupport Svg REQUIRED)

set(SOURCES
    src/main.cpp
    src/CSVReader.cpp
    src/ParserConfigDialog.cpp
    src/PlotConfigDialog.cpp
    src/MainWindow.cpp
    src/qcustomplot.cpp
    src/FileWatcher.cpp
    src/PlotManager.cpp
)

set(HEADERS
    src/CSVReader.h
    src/ParserConfigDialog.h
    src/PlotConfigDialog.h
    src/MainWindow.h
    src/qcustomplot.h
    src/FileWatcher.h
    src/PlotManager.h
)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/ui)

add_executable(RTPlotter ${SOURCES} ${HEADERS} ui/ParserConfigDialog.ui ui/PlotConfigDialog.ui ui/MainWindow.ui resources/resources.qrc)

target_link_libraries(RTPlotter PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::PrintSupport Qt6::Svg)

target_include_directories(RTPlotter PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(APPLE)
    # Build as a macOS bundle so macdeployqt and mac packaging work nicely
    set_target_properties(RTPlotter PROPERTIES MACOSX_BUNDLE TRUE)
    # Recommend an icns file named RTPlotter.icns to be placed in resources/icons/
    set_target_properties(RTPlotter PROPERTIES MACOSX_BUNDLE_ICON_FILE "RTPlotter.icns")
endif()

# Install rules (useful for CPack and packaging)
install(TARGETS RTPlotter
                RUNTIME DESTINATION bin
                BUNDLE DESTINATION .)

# If an icns was provided, install it so CPack or packaging scripts can pick it up
install(FILES ${CMAKE_SOURCE_DIR}/resources/icons/RTPlotter.icns DESTINATION . OPTIONAL)

# CPack configuration for packaging (.dmg on macOS via DragNDrop, deb on Linux if desired)
set(CPACK_PACKAGE_NAME "RTPlotter")
set(CPACK_PACKAGE_VENDOR "RTPlotter")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RTPlotter — real-time CSV plotter")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "0")
if(APPLE)
    # Use DragNDrop to create a DMG on macOS
    set(CPACK_GENERATOR "DragNDrop")
elseif(UNIX)
    # On Linux, offer DEB and TGZ (user can enable/disable by installing packagers)
    set(CPACK_GENERATOR "DEB;TGZ")
endif()

include(InstallRequiredSystemLibraries)
include(CPack)
