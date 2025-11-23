#pragma once
#include "processutils.h"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <iostream>
#include <algorithm> 
#include <vector> 

namespace UEBuilder {

    namespace fs = std::filesystem; // Define fs namespace alias here for clarity

    struct EngineInfo {
        std::wstring Version;
        std::wstring RootPath;
        std::wstring UBTPath;
        std::wstring UATPath; // RunUAT.bat
        bool IsValid = false;
    };

    class EngineDetector {
    public:
        // Simple JSON-like parser to get "EngineAssociation"
        static std::wstring GetEngineAssociation(const std::wstring& projectPath) {
            std::ifstream file(projectPath);
            if (!file.is_open()) return L"";

            std::string line;
            while (std::getline(file, line)) {
                if (line.find("\"EngineAssociation\"") != std::string::npos) {
                    size_t start = line.find(":", line.find("EngineAssociation"));
                    size_t firstQuote = line.find("\"", start);
                    size_t secondQuote = line.find("\"", firstQuote + 1);

                    if (firstQuote != std::string::npos && secondQuote != std::string::npos) {
                        std::string ver = line.substr(firstQuote + 1, secondQuote - firstQuote - 1);
                        return std::wstring(ver.begin(), ver.end());
                    }
                }
            }
            return L"";
        }

        static EngineInfo FindEngine(const std::wstring& association) {
            EngineInfo info;
            info.Version = association;

            // FIX for C2362: Declare registry key variables early to prevent 'goto' from skipping initialization
            std::wstring regKeyCU;
            std::wstring regKeyLM;
            std::wstring regKeyLM_Space;
            std::wstring regKeyWow;

            std::wcout << L"[Debug] Looking for Engine Version: " << association << L"..." << std::endl;

            // --- STEP 1: DIRECT FILE SYSTEM SCAN (Program Files) ---

            // Standard locations where Epic Games installs engines
            std::vector<fs::path> installRoots = {
                L"C:/Program Files/Epic Games",
                L"C:/Program Files (x86)/Epic Games" // Fallback for some systems
            };

            // The required engine folder name, e.g., "UE_5.5"
            std::wstring requiredFolderName = L"UE_" + association;

            std::wcout << L"[Debug] Attempting direct file system scan for folder: " << requiredFolderName << L"..." << std::endl;

            for (const auto& root : installRoots) {
                fs::path potentialPath = root / requiredFolderName;
                if (fs::exists(potentialPath) && fs::is_directory(potentialPath)) {
                    // Check if UnrealBuildTool exists here to confirm validity
                    fs::path ubtCheck = potentialPath / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.exe";
                    if (fs::exists(ubtCheck)) {
                        info.RootPath = potentialPath.wstring();
                        std::wcout << L"[Info] Found engine via direct scan: " << info.RootPath << std::endl;
                        goto EngineFound; // Skip registry checks if found here
                    }
                }
            }

            // --- STEP 2: REGISTRY CHECKS (only if direct scan fails) ---

            // 2. Check Registry: Current User (Source Builds / Custom Registrations)
            regKeyCU = L"Software\\Epic Games\\Unreal Engine\\Builds"; // Now only assignment
            info.RootPath = ProcessUtils::ReadRegistryString(HKEY_CURRENT_USER, regKeyCU, association);

            if (info.RootPath.empty()) {
                std::wcout << L"[Debug] Not found in HKCU\\" << regKeyCU << std::endl;

                // 2a. Check Registry: Local Machine (Standard Launcher Installs - No Space)
                regKeyLM = L"SOFTWARE\\EpicGames\\Unreal Engine\\" + association; // Now only assignment
                info.RootPath = ProcessUtils::ReadRegistryString(HKEY_LOCAL_MACHINE, regKeyLM, L"InstalledDirectory");
            }

            if (info.RootPath.empty()) {
                // 2b. Check Registry: Local Machine (Alternative - With Space)
                regKeyLM_Space = L"SOFTWARE\\Epic Games\\Unreal Engine\\" + association; // Now only assignment
                info.RootPath = ProcessUtils::ReadRegistryString(HKEY_LOCAL_MACHINE, regKeyLM_Space, L"InstalledDirectory");
            }

            if (info.RootPath.empty()) {
                std::wcout << L"[Debug] Not found in HKLM\\" << L"SOFTWARE\\Epic( )Games\\Unreal Engine\\" + association << std::endl;

                // 3. Check Registry: WOW6432Node (Common fallback for Launcher on x64 Windows)
                regKeyWow = L"SOFTWARE\\WOW6432Node\\EpicGames\\Unreal Engine\\" + association; // Now only assignment
                info.RootPath = ProcessUtils::ReadRegistryString(HKEY_LOCAL_MACHINE, regKeyWow, L"InstalledDirectory");
            }

        EngineFound:; // Label for jump

            // --- STEP 3: FALLBACK (MANUAL INPUT) ---
            if (info.RootPath.empty()) {
                std::wcout << L"[Debug] Failed to find any registry key for " << association << L" either." << std::endl;
                std::wcout << L"\n[Action Required] Could not auto-detect Unreal Engine " << association << L"." << std::endl;
                std::wcout << L"Please manually enter the path to your Unreal Engine folder." << std::endl;
                std::wcout << L"(Example: C:\\Program Files\\Epic Games\\UE_5.5)" << std::endl;
                std::wcout << L"> ";

                std::getline(std::wcin, info.RootPath);

                // Sanitize input (quotes and whitespace)
                if (!info.RootPath.empty()) {
                    info.RootPath.erase(0, info.RootPath.find_first_not_of(L" \t\n\r"));
                    info.RootPath.erase(info.RootPath.find_last_not_of(L" \t\n\r") + 1);
                    if (info.RootPath.front() == L'"') info.RootPath.erase(0, 1);
                    if (info.RootPath.back() == L'"') info.RootPath.pop_back();
                    // Normalize slashes
                    std::replace(info.RootPath.begin(), info.RootPath.end(), L'\\', L'/');
                }
            }

            // Validation
            if (!info.RootPath.empty()) {
                std::wcout << L"[Debug] Using Path: " << info.RootPath << std::endl;

                fs::path root(info.RootPath);
                fs::path ubt = root / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.exe";
                fs::path uat = root / "Engine" / "Build" / "BatchFiles" / "RunUAT.bat";

                if (fs::exists(ubt)) {
                    info.UBTPath = ubt.wstring();
                    info.UATPath = uat.wstring();
                    info.IsValid = true;
                }
                else {
                    std::wcout << L"[Debug] Path found, but UnrealBuildTool.exe is missing at: " << ubt.wstring() << std::endl;
                }
            }

            return info;
        }
    };
}