#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

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
    appendLog("Build started...");
}

void MainWindow::onCancelButtonClicked()
{
    close();
}


MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::appendLog(const QString& text)
{
    ui->logOutput->appendPlainText(text);
}
