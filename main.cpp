#define NOMINMAX

#include "processutils.h"
#include "toolchainmanager.h"
#include "enginedetector.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm> // Required for std::replace
#include <sstream>
#include <limits>    // Required for std::numeric_limits
#include <windows.h> // For WideCharToMultiByte

using namespace UEBuilder;
namespace fs = std::filesystem; // Define fs namespace alias here

// Helper function to convert wstring to string using Windows API
std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.length(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

// UI Helper - Made static as suggested
static void PrintHeader() {
    system("cls");
    std::cout << "============================================\n";
    std::cout << "      STANDALONE UNREAL ENGINE BUILDER      \n";
    std::cout << "============================================\n";
}

int main() {
    PrintHeader();

    // --- STEP 1: Toolchain Check ---
    ToolchainManager toolManager;
    std::cout << "[Init] Checking for MSVC Build Tools...\n";
    if (!toolManager.IsMSVCInstalled()) {
        std::cout << "[Init] MSVC not found.\n";
        std::cout << "Do you want to auto-install Visual Studio Build Tools? (y/n): ";
        char resp;
        std::cin >> resp;
        // FIX: Only clear the buffer if we actually used cin above
        std::cin.ignore((std::numeric_limits<std::streamsize>::max)(), '\n');

        if (resp == 'y' || resp == 'Y') {
            toolManager.InstallTools();
        }
        else {
            std::cout << "Cannot proceed without compiler. Exiting.\n";
            return 1;
        }
    }
    else {
        std::cout << "[Init] MSVC Build Tools detected.\n";
    }

    // --- STEP 2: Project Selection ---
    std::cout << "\nEnter path to .uproject file: ";
    std::wstring projectPathStr;

    // FIX: Removed unconditional ignore() here that was causing the freeze
    std::getline(std::wcin, projectPathStr);

    if (!projectPathStr.empty()) {
        // 1. Trim leading and trailing whitespace
        projectPathStr.erase(0, projectPathStr.find_first_not_of(L" \t\n\r"));
        projectPathStr.erase(projectPathStr.find_last_not_of(L" \t\n\r") + 1);

        // 2. Sanitize quotes from drag-and-drop
        if (projectPathStr.front() == L'"') projectPathStr.erase(0, 1);
        if (projectPathStr.back() == L'"') projectPathStr.pop_back();

        // 3. FIX: Check for and correct the common missing drive letter issue (e.g., if 'C' was dropped)
        if (projectPathStr.length() > 1 && projectPathStr[1] == L':' && projectPathStr[0] == L'/') {
            // If the path starts with /C:/ (caused by the initial path sanitization + the input error), remove the leading slash.
            projectPathStr.erase(0, 1);
        }
        else if (projectPathStr.length() > 1 && projectPathStr[1] == L'/' && (projectPathStr[0] >= L'A' && projectPathStr[0] <= L'Z')) {
            // If path looks like C/Unreal Projects... (missing the colon), add it. This is a heuristic guess.
            projectPathStr.insert(1, L":");
        }
        else if (projectPathStr.length() > 2 && projectPathStr[0] == L'/' && (projectPathStr[1] >= L'A' && projectPathStr[1] <= L'Z') && projectPathStr[2] == L'/') {
            // A common case: /C/Unreal Projects -> needs to be C:/Unreal Projects
            // Standardize path separators (\\ to /) AFTER quotes are gone.
            // But for the input error, we will replace backslashes below.
        }

        // 4. Standardize path separators (use forward slashes for filesystem consistency)
        std::replace(projectPathStr.begin(), projectPathStr.end(), L'\\', L'/');
    }

    if (!fs::exists(projectPathStr)) {
        std::cerr << "[Error] File does not exist. Please check the path carefully.\n";
        std::wcout << L"Attempted Path: " << projectPathStr << std::endl;
        system("pause");
        return 1;
    }

    // --- STEP 3: Engine Detection ---
    std::wstring association = EngineDetector::GetEngineAssociation(projectPathStr);
    std::wcout << L"[Info] Project uses Engine: " << association << std::endl;

    EngineInfo engine = EngineDetector::FindEngine(association);
    if (!engine.IsValid) {
        std::cerr << "[Error] Could not locate Unreal Engine installation for version "
            << WStringToString(association) << "\n";
        std::cerr << "Ensure the engine is registered or try generating project files manually once.\n";
        system("pause");
        return 1;
    }
    std::wcout << L"[Info] Found UBT at: " << engine.UBTPath << std::endl;

    // --- STEP 4: Configuration Menu ---
    std::string config = "Development";
    std::string platform = "Win64"; // Default
    std::string targetName = "Editor"; // Default suffix

    // Get Project Name from filename
    std::wstring filename = fs::path(projectPathStr).stem().wstring();

    while (true) {
        PrintHeader();
        std::wcout << L"Project: " << filename << L"\n";
        std::cout << "1. Configuration: " << config << "\n";
        std::cout << "2. Target Type: " << targetName << "\n";
        std::cout << "3. BUILD PROJECT\n";
        std::cout << "4. Exit\n";
        std::cout << "Select: ";

        int choice;
        std::cin >> choice;

        if (choice == 1) {
            std::cout << "Enter Config (DebugGame, Development, Shipping): ";
            std::cin >> config;
        }
        else if (choice == 2) {
            std::cout << "Enter Target (Game, Editor, Client): ";
            std::cin >> targetName;
        }
        else if (choice == 3) {
            // Construct UBT Command
            // Format: UnrealBuildTool.exe [ProjectName][TargetType] [Platform] [Config] -project="Path"

            std::wstring buildTarget;
            if (targetName == "Game") buildTarget = filename; // Pure game target usually matches project name
            else buildTarget = filename + std::wstring(targetName.begin(), targetName.end());

            std::wstring wConfig(config.begin(), config.end());
            std::wstring wPlatform(platform.begin(), platform.end());

            std::wstring args = buildTarget + L" " + wPlatform + L" " + wConfig +
                L" -project=\"" + projectPathStr + L"\"" +
                L" -waitmutex -progress";

            std::cout << "\n--- STARTING BUILD ---\n";

            bool success = ProcessUtils::RunProcess(engine.UBTPath, args, L"", [](const std::string& line) {
                // Colorize output simply for console
                if (line.find("error") != std::string::npos)
                    std::cout << "!! " << line; // Highlight error
                else
                    std::cout << line;
                });

            if (success) std::cout << "\n--- BUILD SUCCESSFUL ---\n";
            else std::cout << "\n--- BUILD FAILED ---\n";

            system("pause");
        }
        else if (choice == 4) {
            break;
        }
    }

    return 0;
}