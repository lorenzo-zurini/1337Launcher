cmake_minimum_required(VERSION 3.16)

project(1337Launcher VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Widgets Network)

add_library(miniz STATIC third_party/miniz/miniz.c)
target_include_directories(miniz PUBLIC third_party/miniz)
target_compile_definitions(miniz PUBLIC MINIZ_NO_ZLIB_COMPATIBLE_NAMES MINIZ_NO_STDIO MINIZ_NO_TIME)

add_executable(1337Launcher WIN32 MACOSX_BUNDLE
    src/main.cpp
    src/Paths.h src/Paths.cpp
    src/Net.h src/Net.cpp
    src/Account.h src/Account.cpp
    src/DownloadManager.h src/DownloadManager.cpp
    src/VersionInfo.h src/VersionInfo.cpp
    src/Instance.h src/Instance.cpp
    src/GameInstaller.h src/GameInstaller.cpp
    src/GameLauncher.h src/GameLauncher.cpp
    src/Zip.h src/Zip.cpp
    src/MainWindow.h src/MainWindow.cpp
    src/NewInstanceDialog.h src/NewInstanceDialog.cpp
    src/SettingsDialog.h src/SettingsDialog.cpp
    src/ConsoleWindow.h src/ConsoleWindow.cpp
    resources/resources.qrc
)

target_compile_definitions(1337Launcher PRIVATE
    LAUNCHER_VERSION="${PROJECT_VERSION}"
)
target_link_libraries(1337Launcher PRIVATE Qt6::Widgets Qt6::Network miniz)

include(GNUInstallDirs)
install(TARGETS 1337Launcher
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    BUNDLE DESTINATION .)
if(UNIX AND NOT APPLE)
    install(FILES resources/1337launcher.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(FILES resources/1337launcher.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
endif()
