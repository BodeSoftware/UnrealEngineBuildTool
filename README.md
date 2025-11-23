# UnrealEngineBuildTool

Automatically build Unreal Engine C++ projects for non-programmers.

# 🚀 Overview

UEBuilder is a standalone tool (CLI + GUI) that allows non-programmers and team members without Visual Studio to build Unreal Engine C++ projects easily.

It removes the need for:

Opening Visual Studio

Understanding UBT command-line syntax

Running complicated batch files

Configuring build environments manually

UEBuilder automatically:

Detects the correct Unreal Engine installation

Finds the right UnrealBuildTool.exe

Validates that the MSVC Build Tools are installed

Builds your C++ project with correct parameters

Streams build output in real time

Detects when a Clean is required (Intermediate issues)

Can automatically clean & rebuild

UEBuilder supports both:

✔ Command-Line Mode (legacy)
✔ Full Qt6 GUI Mode (new)

## 🖥️ New GUI Application (Qt6)

The new UEBuilder GUI is a fully interactive Windows application that offers:

✔ Browse button for selecting projects
✔ Live build log with color-coded errors
✔ Build button (auto-disabled during build)
✔ Cancel button
✔ Auto “Clean Project” detection
✔ One-click Clean → Rebuild
✔ Real-time UBT output streaming
✔ Error highlighting
✔ No coding knowledge required

This version is ideal for:

Designers

Producers

Artists

QA testers

Anyone who needs to build the project without touching Visual Studio

## 🛠️ GUI Usage Instructions
1. Launch the app

Run:

UnrealEngineBuildTool_QT.exe

2. Select your Unreal Engine project

Click Browse, then select either:

The project folder, or

The .uproject file

The tool automatically detects the engine version and location.

3. Press Build

UEBuilder will:

Check MSVC Build Tools

Resolve the .uproject

Detect the engine

Run UnrealBuildTool

Stream output live

4. Error Detection

Errors appear in red text.

If the tool detects Intermediate/Saved/Binaries corruption, the Clean button becomes available.

5. Clean & Auto-Rebuild

Click Clean to automatically remove:

Intermediate/
Saved/
Binaries/


Then the tool will automatically rebuild.

### 🖥️ Command Line Version (Legacy Mode)

Still included and usable — especially for automation or scripting.

How to Run
UEBuilder.exe

Features

Engine detection

Build config selection

Automatic MSVC check

Real-time build output

No Visual Studio required

Typical CLI Flow

Enter project directory

UEBuilder finds the .uproject

UEBuilder locates the correct Unreal Engine

Choose build type (Development, Shipping, etc.)

Build

## Prerequisites

Even though UEBuilder avoids using Visual Studio, you still need:

✔ Microsoft Visual C++ Build Tools
✔ Windows 10/11 SDK

UEBuilder automatically detects missing components and prompts the user.

# 💻 Building from Source (For Developers)

To compile the CLI version manually:

cl main.cpp /EHsc /std:c++17 /Fe:UEBuilder.exe


To build the GUI version, use Qt Creator:

Open CMakeLists.txt

Configure with Qt6 (MSVC 64-bit)

Build normally

Dependencies:

Qt6 Widgets

CMake 3.16+

MSVC 2022 toolchain
