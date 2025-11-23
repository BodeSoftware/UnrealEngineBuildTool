#pragma once
#include <windows.h>
#include <string>
#include <iostream>
#include <functional>
#include <vector>

// Link against these libraries
#pragma comment(lib, "urlmon.lib") 
#pragma comment(lib, "Advapi32.lib") // For Registry

namespace UEBuilder {

    // Callback function type for real-time log handling
    using LogCallback = std::function<void(const std::string&)>;

    class ProcessUtils {
    public:
        // Downloads a file from the internet to a local path
        static bool DownloadFile(const std::wstring& url, const std::wstring& destPath) {
            HRESULT hr = URLDownloadToFileW(NULL, url.c_str(), destPath.c_str(), 0, NULL);
            return hr == S_OK;
        }

        // Runs a command and streams output to a callback function
        static bool RunProcess(const std::wstring& command, const std::wstring& args, const std::wstring& workDir, LogCallback onLog) {
            HANDLE hReadPipe, hWritePipe;
            SECURITY_ATTRIBUTES saAttr;

            // Set the bInheritHandle flag so pipe handles are inherited by child process
            saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = NULL;

            // Create a pipe for the child process's STDOUT
            if (!CreatePipe(&hReadPipe, &hWritePipe, &saAttr, 0)) return false;

            // Ensure the read handle to the pipe for STDOUT is not inherited
            SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

            STARTUPINFOW si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            si.hStdError = hWritePipe;  // Capture stderr
            si.hStdOutput = hWritePipe; // Capture stdout
            si.dwFlags |= STARTF_USESTDHANDLES;

            ZeroMemory(&pi, sizeof(pi));

            std::wstring fullCmd = L"\"" + command + L"\" " + args;
            std::wstring currentDir = workDir.empty() ? L"" : workDir;

            // Create the Child Process
            BOOL success = CreateProcessW(
                NULL,
                &fullCmd[0],     // Command line
                NULL,            // Process handle not inheritable
                NULL,            // Thread handle not inheritable
                TRUE,            // Set handle inheritance to TRUE
                CREATE_NO_WINDOW,// Don't show a pop-up console
                NULL,            // Use parent's environment block
                currentDir.empty() ? NULL : currentDir.c_str(),
                &si,
                &pi
            );

            // We can close the write end of the pipe now; the child has it.
            CloseHandle(hWritePipe);

            if (!success) {
                CloseHandle(hReadPipe);
                return false;
            }

            // Read output from the child process
            DWORD dwRead;
            CHAR chBuf[4096];
            bool bSuccess = FALSE;

            while (true) {
                bSuccess = ReadFile(hReadPipe, chBuf, 4095, &dwRead, NULL);
                if (!bSuccess || dwRead == 0) break;

                chBuf[dwRead] = '\0'; // Null terminate
                if (onLog) onLog(std::string(chBuf));
            }

            // Wait for process to finish
            WaitForSingleObject(pi.hProcess, INFINITE);

            DWORD exitCode = 0;
            GetExitCodeProcess(pi.hProcess, &exitCode);

            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);

            return exitCode == 0;
        }

        // Helper to check registry keys (used to find Unreal)
        static std::wstring ReadRegistryString(HKEY hKeyRoot, const std::wstring& subKey, const std::wstring& valueName) {
            HKEY hKey;
            if (RegOpenKeyExW(hKeyRoot, subKey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
                return L"";
            }

            WCHAR buffer[512];
            DWORD bufferSize = sizeof(buffer);
            if (RegQueryValueExW(hKey, valueName.c_str(), 0, NULL, (LPBYTE)buffer, &bufferSize) != ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return L"";
            }

            RegCloseKey(hKey);
            return std::wstring(buffer);
        }
    };
}
