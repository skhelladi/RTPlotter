#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "PlotConfigDialog.h"
#include "CSVReader.h"
#include "FileWatcher.h"
#include "PlotManager.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setPlotConfig(const QList<PlotConfig>& configs);
    void setReader(const CSVReader& reader);

private slots:
    void onFileChanged(const QString& path);
    void on_actionPause_triggered();
    void on_actionResetZoom_triggered();
    void on_actionExport_triggered();
    void on_actionOpenCSV_triggered();
    void on_actionConfigurePlots_triggered();
    void on_actionAbout_triggered();

private:
    Ui::MainWindow *ui;
    QList<PlotConfig> m_plotConfigs;
    CSVReader m_reader;
    FileWatcher m_fileWatcher;
    PlotManager m_plotManager;
    bool m_paused;
    QString m_projectPath;
    bool m_dirty;
    QLabel* m_projectLabel;
    QLabel* m_dirtyLabel;

    void saveProjectToPath(const QString& path);
    bool loadProjectFromPath(const QString& path);
    void importCSV(const QString& filePath = QString());
    void updateStatusBar();
protected:
    void closeEvent(QCloseEvent* event) override;

    void setupPlots();
    void saveConfigForFile(const QString& filePath);
    bool loadConfigForFile(const QString& filePath);
};

#endif // MAINWINDOW_H
