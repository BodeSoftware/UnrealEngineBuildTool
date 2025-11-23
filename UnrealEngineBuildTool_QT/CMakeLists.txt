cmake_minimum_required(VERSION 3.16)

project(UnrealEngineBuildTool_QT VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ADDED: Let Qt see the backend headers in the parent folder
include_directories(..)

qt_add_executable(UnrealEngineBuildTool_QT
    main.cpp               # Qt main file
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    # (Optional but recommended to show them inside QtCreator)
    ../EngineDetector.h
    ../ProcessUtils.h
    ../ToolchainManager.h
)

target_link_libraries(UnrealEngineBuildTool_QT PRIVATE Qt6::Widgets)

set_target_properties(UnrealEngineBuildTool_QT PROPERTIES
    WIN32_EXECUTABLE ON
)
