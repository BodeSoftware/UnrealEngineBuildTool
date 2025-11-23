#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onBrowseButtonClicked();
    void onBuildButtonClicked();
    void onCancelButtonClicked();
    void onCleanButtonClicked();

private:
    Ui::MainWindow *ui;

    void appendLog(const QString& text);

    // --------------------------
    // Build/Clean state tracking
    // --------------------------
    bool buildRunning = false;   // True only while a build is active
    bool cleanNeeded = false;    // Becomes true if log suggests a clean is required
};

#endif // MAINWINDOW_H
