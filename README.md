# UnrealEngineBuildTool

Automatically build Unreal Engine C++ projects for non-programmers.

🚀 Overview

The UnrealEngineBuildTool (UEBuilder) is a lightweight, standalone command-line application designed to simplify the process of compiling Unreal Engine C++ code. It eliminates the need to manually execute the full Unreal Build Tool (UBT) command line, set up environment variables, or rely on a functional Visual Studio installation.

It automatically performs necessary checks, suchates the correct engine version, and presents a simple menu to choose your build configuration (Development, DebugGame, Shipping) and target (Editor or Standalone Game).

🛠️ Usage for End-Users (Using the UEBuilder.exe File)

This is the recommended path for non-programmers who simply need to compile C++ code within a project.

Prerequisites

Visual Studio Build Tools: The tool will automatically check for and offer to install the required Microsoft Visual C++ Build Tools and Windows SDK if they are missing. This is necessary for C++ compilation to work.

Execution Steps

Launch: Double-click the UEBuilder.exe file.

Project Selection: When prompted, enter the file path to your Unreal Engine Project FOLDER.

Example: C:\Users\YourName\Documents\Unreal Projects\MyGameProject

Note: The tool automatically detects the .uproject file inside the folder.

Engine Detection: The tool reads the project file, determines the associated Unreal Engine version (e.g., 5.3), and automatically locates the correct UnrealBuildTool.exe on your system.

Select Build Options: Use the menu (1, 2, 3) to select:

Configuration: (e.g., Development, Shipping)

Target: (e.g., Editor for fast iteration, Game for a final executable)

Build: Select option 3. BUILD PROJECT to start the compilation process. The build log will be streamed directly to the console in real-time.

💻 Building from Source (For Developers)

This section is for users who want to modify the UEBuilder source code or build the executable themselves.

Compile and Link: Use the Microsoft Visual C++ compiler (cl) command: 

"cl main.cpp /EHsc /std:c++17 /Fe:UEBuilder.exe"
