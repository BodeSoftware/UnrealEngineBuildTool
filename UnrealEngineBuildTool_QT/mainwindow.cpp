#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QMetaObject>

#include <filesystem>
#include <thread>
#include <algorithm>

#include "ToolchainManager.h"
#include "EngineDetector.h"
#include "ProcessUtils.h"

using namespace UEBuilder;
namespace fs = std::filesystem;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    connect(ui->browseButton, &QPushButton::clicked,
            this, &MainWindow::onBrowseButtonClicked);

    connect(ui->buildButton, &QPushButton::clicked,
            this, &MainWindow::onBuildButtonClicked);

    connect(ui->cancelButton, &QPushButton::clicked,
            this, &MainWindow::onCancelButtonClicked);

    connect(ui->cleanButton, &QPushButton::clicked,
            this, &MainWindow::onCleanButtonClicked);
}

void MainWindow::onBrowseButtonClicked()
{
    QString folder = QFileDialog::getExistingDirectory(
        this,
        "Select Unreal Project Folder"
        );

    if (!folder.isEmpty()) {
        ui->projectPathEdit->setText(folder);
        appendLog("Selected folder: " + folder);
    }
}

void MainWindow::onBuildButtonClicked()
{
    //----------------------------------------------------------
    // 1. Get path from the UI
    //----------------------------------------------------------
    QString qPath = ui->projectPathEdit->text().trimmed();
    if (qPath.isEmpty()) {
        QMessageBox::warning(this, "Missing Path",
                             "Please select an Unreal project folder or .uproject file first.");
        return;
    }

    //----------------------------------------------------------
    // 2. Check MSVC Build Tools
    //----------------------------------------------------------
    ToolchainManager toolManager;
    if (!toolManager.IsMSVCInstalled()) {
        QMessageBox::critical(this, "MSVC Build Tools Missing",
                              "Microsoft C++ Build Tools were not detected.\n\n"
                              "Please install them (or use the CLI auto-install) "
                              "before building.");
        return;
    }

    //----------------------------------------------------------
    // 3. Convert QString → std::wstring and sanitize
    //----------------------------------------------------------
    std::wstring inputPathStr = qPath.toStdWString();

    if (!inputPathStr.empty()) {

        // Trim whitespace
        inputPathStr.erase(0, inputPathStr.find_first_not_of(L" \t\n\r"));
        inputPathStr.erase(inputPathStr.find_last_not_of(L" \t\n\r") + 1);

        // Strip quotes
        if (!inputPathStr.empty() && inputPathStr.front() == L'"')
            inputPathStr.erase(0, 1);
        if (!inputPathStr.empty() && inputPathStr.back() == L'"')
            inputPathStr.pop_back();

        // Fix drive prefix
        if (inputPathStr.length() > 1 &&
            inputPathStr[1] == L':' && inputPathStr[0] == L'/') {
            inputPathStr.erase(0, 1);
        }
        else if (inputPathStr.length() > 1 &&
                 inputPathStr[1] == L'/' &&
                 (inputPathStr[0] >= L'A' && inputPathStr[0] <= L'Z')) {
            inputPathStr.insert(1, L":");
        }

        // Normalize slashes
        std::replace(inputPathStr.begin(), inputPathStr.end(), L'\\', L'/');
    }

    //----------------------------------------------------------
    // 4. Resolve folder → .uproject
    //----------------------------------------------------------
    fs::path targetPath(inputPathStr);
    std::wstring projectPathStr;

    if (targetPath.extension() == L".uproject") {

        projectPathStr = targetPath.wstring();

    } else if (fs::is_directory(targetPath)) {

        bool found = false;
        for (const auto &entry : fs::directory_iterator(targetPath)) {
            if (entry.is_regular_file() && entry.path().extension() == L".uproject") {
                projectPathStr = entry.path().wstring();
                found = true;
                break;
            }
        }

        if (!found) {
            QMessageBox::critical(this, "No .uproject Found",
                                  "No .uproject file was found in the selected folder.");
            return;
        }

    } else {
        QMessageBox::critical(this, "Invalid Path",
                              "The selected path is not a folder or a .uproject file.");
        return;
    }

    if (!fs::exists(projectPathStr)) {
        QMessageBox::critical(this, "Path Error",
                              "The resolved .uproject path does not exist.");
        return;
    }

    //----------------------------------------------------------
    // 5. Engine detection
    //----------------------------------------------------------
    std::wstring association =
        EngineDetector::GetEngineAssociation(projectPathStr);

    if (association.empty()) {
        QMessageBox::critical(this, "Engine Detection Failed",
                              "Could not read EngineAssociation from the .uproject file.");
        return;
    }

    EngineInfo engine = EngineDetector::FindEngine(association);

    if (!engine.IsValid) {
        QMessageBox::critical(this, "Unreal Engine Not Found",
                              "Could not locate the Unreal Engine installation.\n"
                              "Verify this version is installed and registered.");
        return;
    }

    //----------------------------------------------------------
    // 6. Build command (always Development)
    //----------------------------------------------------------
    std::wstring filename =
        fs::path(projectPathStr).stem().wstring();

    std::string config   = "Development";
    std::string platform = "Win64";
    std::string target   = "Editor"; // always Editor for now

    std::wstring buildTarget =
        (target == "Game")
            ? filename
            : filename + std::wstring(target.begin(), target.end());

    std::wstring wConfig(config.begin(), config.end());
    std::wstring wPlatform(platform.begin(), platform.end());

    std::wstring args =
        buildTarget + L" " + wPlatform + L" " + wConfig +
        L" -project=\"" + projectPathStr + L"\"" +
        L" -waitmutex -progress";

    appendLog("Starting build...\n");

    // Disable button during build
    ui->buildButton->setEnabled(false);
    ui->buildButton->setText("Building...");

    //----------------------------------------------------------
    // 7. Run UBT in background thread
    //----------------------------------------------------------
    auto ubtPath = engine.UBTPath; // copy for lambda

    std::thread([this, ubtPath, args]()
                {
                    bool success = ProcessUtils::RunProcess(
                        ubtPath,
                        args,
                        L"",
                        [this](const std::string &line)
                        {
                            QString qLine = QString::fromStdString(line);
                            QMetaObject::invokeMethod(
                                this,
                                [this, qLine]() { appendLog(qLine); },
                                Qt::QueuedConnection
                                );
                        }
                        );

                    QMetaObject::invokeMethod(
                        this,
                        [this, success]()
                        {
                            appendLog(success
                                          ? "\n--- BUILD SUCCESSFUL ---\n"
                                          : "\n--- BUILD FAILED ---\n");

                            // Re-enable build button
                            ui->buildButton->setEnabled(true);
                            ui->buildButton->setText("Build");
                        },
                        Qt::QueuedConnection
                        );
                }).detach();
}

void MainWindow::onCancelButtonClicked()
{
    close();
}

// ----------------------------------------------------
// CLEAN BUTTON SLOT
// ----------------------------------------------------
void MainWindow::onCleanButtonClicked()
{
    if (!cleanNeeded)
        return;

    appendLog("\n--- CLEANING PROJECT ---\n");

    ui->cleanButton->setEnabled(false);
    ui->cleanButton->setText("Cleaning...");

    // Clean in a background thread
    std::thread([this]()
                {
                    // Resolve project root from what is in the line edit
                    fs::path selectedPath(ui->projectPathEdit->text().toStdWString());
                    fs::path projectRoot;

                    if (selectedPath.extension() == L".uproject") {
                        projectRoot = selectedPath.parent_path();
                    } else if (fs::is_directory(selectedPath)) {
                        projectRoot = selectedPath;
                    } else {
                        // If it's not valid, just bail out on the clean
                        QMetaObject::invokeMethod(
                            this,
                            [this]()
                            {
                                appendLog("\n--- CLEAN FAILED: Invalid project path ---\n");
                                ui->cleanButton->setText("Clean");
                                ui->cleanButton->setEnabled(false);
                            },
                            Qt::QueuedConnection
                            );
                        return;
                    }

                    // Delete Intermediate, Saved, Binaries
                    fs::remove_all(projectRoot / "Intermediate");
                    fs::remove_all(projectRoot / "Saved");
                    fs::remove_all(projectRoot / "Binaries");

                    // When done, trigger rebuild on UI thread
                    QMetaObject::invokeMethod(
                        this,
                        [this]()
                        {
                            appendLog("\n--- CLEAN COMPLETE — REBUILDING NOW ---\n");

                            cleanNeeded = false;
                            ui->cleanButton->setText("Clean");
                            ui->cleanButton->setEnabled(false);

                            // Auto rebuild
                            onBuildButtonClicked();
                        },
                        Qt::QueuedConnection
                        );

                }).detach();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::appendLog(const QString &text)
{
    QString line  = text;
    QString lower = line.toLower();

    // ----------------------------------------------------
    // 1. Detect when a clean is needed (before any coloring)
    // ----------------------------------------------------
    if (!cleanNeeded &&
        (lower.contains("intermediate") ||
         lower.contains("msb3073") ||
         lower.contains("ubt error") ||
         lower.contains("action failed") ||
         lower.contains("build failed") ||
         lower.contains("could not find") ||
         lower.contains("cannot open include file")))
    {
        cleanNeeded = true;
        ui->cleanButton->setEnabled(true);
    }

    // ----------------------------------------------------
    // 2. Detect typical *error* patterns (case-insensitive)
    // ----------------------------------------------------
    bool isError =
        lower.contains("error:") ||
        lower.contains("error ")  ||
        lower.contains("failed")  ||
        lower.contains("unresolved external") ||
        lower.contains("fatal error");

    if (isError)
    {
        // Wrap in HTML for red text
        QString html = "<span style=\"color:#ff4444;\">" +
                       line.toHtmlEscaped() +
                       "</span>";

        ui->logOutput->appendHtml(html);
        return;
    }

    // ----------------------------------------------------
    // 3. Default: append as normal plain text
    // ----------------------------------------------------
    ui->logOutput->appendPlainText(text);
}
