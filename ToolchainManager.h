#pragma once
#include "ProcessUtils.h"
#include <filesystem>
#include <iostream>

namespace UEBuilder {

    namespace fs = std::filesystem; // Define fs namespace alias here for clarity

    class ToolchainManager {
    public:
        // Checks if MSVC is generally available by looking for a known path
        // Note: A robust check would query the VS Installer API, but checking path is easier for a learner
        bool IsMSVCInstalled() {
            // Check a common default path for VS2022
            // In a production app, we would use "vswhere.exe" to find this dynamically
            fs::path defaultPath = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\VC\\Tools\\MSVC";
            fs::path buildToolsPath = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2022\\BuildTools\\VC\\Tools\\MSVC";

            return fs::exists(defaultPath) || fs::exists(buildToolsPath);
        }

        void InstallTools() {
            std::wcout << L"[Toolchain] MSVC not detected. Initiating Auto-Install..." << std::endl;

            // 1. Download
            std::wstring installerUrl = L"https://aka.ms/vs/17/release/vs_BuildTools.exe";
            std::wstring installerPath = L"vs_BuildTools.exe"; // Save in current dir

            std::wcout << L"[Toolchain] Downloading installer..." << std::endl;
            if (ProcessUtils::DownloadFile(installerUrl, installerPath)) {
                std::wcout << L"[Toolchain] Download complete." << std::endl;
            }
            else {
                std::cerr << "[Error] Failed to download VS Build Tools." << std::endl;
                return;
            }

            // 2. Run Silent Install
            // Arguments strictly from your request
            std::wstring args = L"--quiet --wait --norestart "
                L"--add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 "
                L"--add Microsoft.VisualStudio.Component.Windows10SDK.19041 "
                L"--includeRecommended";

            std::wcout << L"[Toolchain] Installing... This may take a while. Do not close." << std::endl;

            bool result = ProcessUtils::RunProcess(installerPath, args, L"", [](const std::string& log) {
                // The bootstrapper might not output much to stdout, but we capture it anyway
                std::cout << log;
                });

            if (result) {
                std::wcout << L"[Toolchain] Installation successful!" << std::endl;
            }
            else {
                std::cerr << "[Error] Installation failed or cancelled." << std::endl;
            }
        }
    };
}