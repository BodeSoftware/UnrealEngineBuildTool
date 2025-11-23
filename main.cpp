#define NOMINMAX

#include "processutils.h"
#include "toolchainmanager.h"
#include "enginedetector.h"
#include <iostream>
#include <string>
#include <filesystem>
#include <algorithm> 
#include <sstream>
#include <limits>    
#include <windows.h> 

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
        // Only clear the buffer if we actually used cin above
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
    std::cout << "\nEnter path to the Unreal Project FOLDER (or the .uproject file): ";
    std::wstring inputPathStr;

    std::getline(std::wcin, inputPathStr);

    if (!inputPathStr.empty()) {
        // 1. Trim leading and trailing whitespace
        inputPathStr.erase(0, inputPathStr.find_first_not_of(L" \t\n\r"));
        inputPathStr.erase(inputPathStr.find_last_not_of(L" \t\n\r") + 1);

        // 2. Sanitize quotes from drag-and-drop
        if (inputPathStr.front() == L'"') inputPathStr.erase(0, 1);
        if (inputPathStr.back() == L'"') inputPathStr.pop_back();

        // 3. Path sanitization fix (for common missing drive letter issue)
        if (inputPathStr.length() > 1 && inputPathStr[1] == L':' && inputPathStr[0] == L'/') {
            inputPathStr.erase(0, 1);
        }
        else if (inputPathStr.length() > 1 && inputPathStr[1] == L'/' && (inputPathStr[0] >= L'A' && inputPathStr[0] <= L'Z')) {
            inputPathStr.insert(1, L":");
        }

        // 4. Standardize path separators (use forward slashes for filesystem consistency)
        std::replace(inputPathStr.begin(), inputPathStr.end(), L'\\', L'/');
    }

    fs::path targetPath(inputPathStr);
    std::wstring projectPathStr;

    if (targetPath.extension() == L".uproject") {
        // Case 1: User pasted the full .uproject file path
        projectPathStr = targetPath.wstring();
    }
    else if (fs::is_directory(targetPath)) {
        // Case 2: User pasted the project folder path (the preferred method)
        std::wcout << L"[Info] Searching for .uproject file in folder: " << targetPath.wstring() << std::endl;

        // Iterate through the directory to find the first .uproject file
        bool found = false;
        for (const auto& entry : fs::directory_iterator(targetPath)) {
            if (entry.is_regular_file() && entry.path().extension() == L".uproject") {
                projectPathStr = entry.path().wstring();
                std::wcout << L"[Info] Auto-detected .uproject file: " << entry.path().filename().wstring() << std::endl;
                found = true;
                break; // Use the first one found
            }
        }

        if (!found) {
            std::cerr << "[Error] No .uproject file found inside the provided directory.\n";
            std::wcout << L"Attempted Folder: " << targetPath.wstring() << std::endl;
            system("pause");
            return 1;
        }

    }
    else {
        // Case 3: Invalid path provided (neither file nor folder exists)
        std::cerr << "[Error] Path does not exist or is invalid. Please check the path carefully.\n";
        std::wcout << L"Attempted Path: " << targetPath.wstring() << std::endl;
        system("pause");
        return 1;
    }

    // FINAL CHECK: Ensure the resolved path exists (it should if logic above worked)
    if (!fs::exists(projectPathStr)) {
        std::cerr << "[Fatal Error] The resolved .uproject path does not exist.\n";
        std::wcout << L"Resolved Path: " << projectPathStr << std::endl;
        system("pause");
        return 1;
    }


    // --- STEP 3: Engine Detection ---
    std::wstring association = EngineDetector::GetEngineAssociation(projectPathStr);
    std::wcout << L"[Info] Project uses Engine: " << association << std::endl;

    EngineInfo engine = EngineDetector::FindEngine(association);
    if (!engine.IsValid) {
        // NOTE: The FindEngine function now includes a manual input fallback if registry fails.
        // This check only runs if the final result of FindEngine is still invalid (i.e., user didn't enter a valid path manually).
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