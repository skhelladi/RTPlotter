#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "qcustomplot.h"
#include "ParserConfigDialog.h"
#include <QFileDialog>
#include <stdexcept>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QSaveFile>
#include <QPalette>
#include <QApplication>
#include <QSvgRenderer>
#include <QPixmap>
#include <QPainter>

// Helper to render an SVG resource to a QPixmap at a given size
static QPixmap loadSvgPixmap(const QString& path, const QSize& size)
{
    QSvgRenderer renderer(path);
    QPixmap pixmap(size);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    return pixmap;
}

// Helper function to load SVG and adapt color to current theme
static QIcon loadThemeAwareIcon(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QIcon();
    }
    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();
    
    // Get foreground text color from palette
    QColor textColor = QApplication::palette().color(QPalette::WindowText);
    QString colorHex = textColor.name();
    
    // Replace currentColor with actual color in the SVG
    svgContent.replace("fill=\"currentColor\"", QString("fill=\"%1\"").arg(colorHex));
    
    // Convert to pixmap and create icon
    QSvgRenderer renderer(svgContent.toUtf8());
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    
    return QIcon(pixmap);
}

// Special function for colored pause/resume icons
static QIcon loadColoredIcon(const QString& path, const QColor& color = QColor())
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QIcon();
    }
    QString svgContent = QString::fromUtf8(file.readAll());
    file.close();
    
    QColor finalColor = color;
    if (!color.isValid()) {
        finalColor = QApplication::palette().color(QPalette::WindowText);
    }
    QString colorHex = finalColor.name();
    
    svgContent.replace("fill=\"currentColor\"", QString("fill=\"%1\"").arg(colorHex));
    
    QSvgRenderer renderer(svgContent.toUtf8());
    QPixmap pixmap(16, 16);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    renderer.render(&painter);
    painter.end();
    
    return QIcon(pixmap);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // Set window icon from bundled logo
    this->setWindowIcon(QIcon(":/icons/icons/logo.svg"));
    // Ensure the menu bar from the .ui is visible inside the window on all platforms
    // (disable native menu bar to keep menus in the application window)
    if (ui->menuBar) {
        ui->menuBar->setNativeMenuBar(false);
        ui->menuBar->setVisible(true);
    }
    menuBar()->setNativeMenuBar(false);
    menuBar()->setVisible(true);
    connect(&m_fileWatcher, &FileWatcher::fileChanged, this, &MainWindow::onFileChanged);
    // Use menus/actions declared in the .ui file.
    menuBar()->setFixedHeight(22);

    // Connect UI actions (defined in ui/MainWindow.ui)
    // File menu: Open/Save/Save As
    connect(ui->actionOpenProject, &QAction::triggered, this, [this]() {
        qDebug() << "actionOpenProject triggered";
        QString p = QFileDialog::getOpenFileName(this, tr("Open Project or Data"), "", tr("RTPlotter Project (*.rtp);;CSV Files (*.csv);;All Files (*)"), nullptr, QFileDialog::DontUseNativeDialog);
        if (!p.isEmpty()) {
            if (p.endsWith(".csv", Qt::CaseInsensitive)) {
                importCSV(p);
            } else {
                loadProjectFromPath(p);
            }
        }
    });
    connect(ui->actionSaveProject, &QAction::triggered, this, [this]() {
        if (m_projectPath.isEmpty()) {
            qDebug() << "actionSaveProject triggered (Save As)";
            QString p = QFileDialog::getSaveFileName(this, tr("Save Project As"), "", tr("RTPlotter Project (*.rtp)"), nullptr, QFileDialog::DontUseNativeDialog);
            if (!p.isEmpty()) saveProjectToPath(p);
        } else {
            saveProjectToPath(m_projectPath);
        }
    });
    connect(ui->actionSaveProjectAs, &QAction::triggered, this, [this]() {
        qDebug() << "actionSaveProjectAs triggered";
        QString p = QFileDialog::getSaveFileName(this, tr("Save Project As"), "", tr("RTPlotter Project (*.rtp)"), nullptr, QFileDialog::DontUseNativeDialog);
        if (!p.isEmpty()) saveProjectToPath(p);
    });
    // Import data -> open parser dialog
    connect(ui->actionImportData, &QAction::triggered, this, &MainWindow::on_actionOpenCSV_triggered);
    // Exit
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    // Configuration -> Plot options
    connect(ui->actionPlotOptions, &QAction::triggered, this, &MainWindow::on_actionConfigurePlots_triggered);
    // Help -> About
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::on_actionAbout_triggered);

    // Differentiate icons: Open project vs Import data
    ui->actionOpenProject->setIcon(loadThemeAwareIcon(":/icons/icons/open.svg"));
    ui->actionImportData->setIcon(loadThemeAwareIcon(":/icons/icons/import.svg"));
    ui->actionSaveProject->setIcon(loadThemeAwareIcon(":/icons/icons/save.svg"));
    ui->actionSaveProjectAs->setIcon(loadThemeAwareIcon(":/icons/icons/save-as.svg"));
    ui->actionExit->setIcon(loadThemeAwareIcon(":/icons/icons/exit.svg"));
    ui->actionPlotOptions->setIcon(loadThemeAwareIcon(":/icons/icons/settings.svg"));
    ui->actionAbout->setIcon(loadThemeAwareIcon(":/icons/icons/help.svg"));

    // Set icons for toolbar actions
    ui->actionOpenCSV->setIcon(loadThemeAwareIcon(":/icons/icons/import.svg"));
    ui->actionConfigurePlots->setIcon(loadThemeAwareIcon(":/icons/icons/settings.svg"));
    // initial pause state = running (so show pause icon)
    m_paused = false;
    ui->actionPause->setIcon(loadThemeAwareIcon(":/icons/icons/pause.svg"));
    ui->actionPause->setToolTip(tr("Pause updates"));
    ui->actionResetZoom->setIcon(loadThemeAwareIcon(":/icons/icons/refresh.svg"));
    ui->actionExport->setIcon(loadThemeAwareIcon(":/icons/icons/export.svg"));

    // Ensure toolbar shows small icons only (no text) for a compact UI
    if (ui->toolBar) {
        ui->toolBar->setIconSize(QSize(16,16));
        ui->toolBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
        // Slightly reduce toolbar height for compact look
        ui->toolBar->setFixedHeight(28);
    }

    // Initialize status and dirty state
    m_dirty = false;
    m_projectLabel = new QLabel(tr("No project"));
    m_dirtyLabel = new QLabel(tr(""));
    if (ui->statusbar) {
        ui->statusbar->addPermanentWidget(m_projectLabel);
        ui->statusbar->addPermanentWidget(m_dirtyLabel);
    }
    updateStatusBar();

    // If a reader was already set earlier (rare), try to load saved config
    if (!m_reader.getFilePath().isEmpty()) {
        loadConfigForFile(m_reader.getFilePath());
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setPlotConfig(const QList<PlotConfig>& configs)
{
    m_plotConfigs = configs;
    // If a variable is designated as XAxis, assign it a special graph id (-1)
    for (int i = 0; i < m_plotConfigs.size(); ++i) {
        if (m_plotConfigs[i].role == PlotConfig::XAxis) {
            m_plotConfigs[i].graph = -1; // X axis not plotted as a Y curve
        }
    }
}

void MainWindow::setReader(const CSVReader& reader)
{
    m_reader = reader;
    m_fileWatcher.watchFile(m_reader.getFilePath());
    setupPlots();
}

void MainWindow::on_actionOpenCSV_triggered()
{
    importCSV();
}

void MainWindow::on_actionConfigurePlots_triggered()
{
    QStringList headers = m_reader.getHeaders();
    // Pass current plot configs as initial settings so the dialog preserves state
    PlotConfigDialog dlg(headers, m_plotConfigs, this);
    if (dlg.exec() == QDialog::Accepted) {
        setPlotConfig(dlg.getPlotConfig());
        // Clear existing plots and re-create
        setupPlots();
        m_dirty = true;
        updateStatusBar();
        if (!m_reader.getFilePath().isEmpty()) {
            saveConfigForFile(m_reader.getFilePath());
        }
    }
}

void MainWindow::on_actionAbout_triggered()
{
    // Create a rich About dialog that includes the logo
    QMessageBox about(this);
    about.setWindowTitle(tr("About RTPlotter"));
    about.setText(tr("<b>RTPlotter</b><br/>Real-Time Data Plotter<br/>Author: Prof. Sofiane KHELLADI &lt;sofiane@khelladi.page&gt;"));
    // Render the SVG at 64x64 and set as the icon pixmap
    QPixmap logo = loadSvgPixmap(":/icons/icons/logo.svg", QSize(64,64));
    if (!logo.isNull()) {
        about.setIconPixmap(logo);
    }
    about.exec();
}

void MainWindow::updateStatusBar()
{
    if (m_projectLabel) {
        if (m_projectPath.isEmpty()) m_projectLabel->setText(tr("No project"));
        else m_projectLabel->setText(QFileInfo(m_projectPath).fileName());
    }
    if (m_dirtyLabel) {
        if (m_dirty) m_dirtyLabel->setText(tr("Modified"));
        else m_dirtyLabel->setText(tr("Saved"));
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_dirty) {
        QMessageBox::StandardButton res = QMessageBox::question(this, tr("Unsaved changes"), tr("Project has unsaved changes. Save before exit?"),
                                                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
                                                              QMessageBox::Save);
        if (res == QMessageBox::Save) {
            if (m_projectPath.isEmpty()) {
                qDebug() << "closeEvent: asking Save As dialog";
                QString p = QFileDialog::getSaveFileName(this, tr("Save Project As"), "", tr("RTPlotter Project (*.rtp)"), nullptr, QFileDialog::DontUseNativeDialog);
                if (!p.isEmpty()) saveProjectToPath(p);
                else { event->ignore(); return; }
            } else {
                saveProjectToPath(m_projectPath);
            }
        } else if (res == QMessageBox::Cancel) {
            event->ignore();
            return;
        }
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::saveConfigForFile(const QString& filePath)
{
    if (filePath.isEmpty()) return;
    QFileInfo fi(filePath);
    QString cfgPath = filePath + ".rtplotter.json";
    QJsonObject root;
    // Parser config
    QJsonObject parserObj;
    parserObj["separator"] = QString(m_reader.getSeparator());
    parserObj["startLine"] = m_reader.getStartLine();
    parserObj["hasHeader"] = m_reader.getHasHeader();
    parserObj["ignoreNonNumeric"] = m_reader.getIgnoreNonNumeric();
    root["parser"] = parserObj;

    // Plot configs
    QJsonArray plotsArray;
    for (const PlotConfig& pc : m_plotConfigs) {
        QJsonObject o;
        o["name"] = pc.name;
        o["role"] = static_cast<int>(pc.role);
        o["graph"] = pc.graph;
        o["style"] = static_cast<int>(pc.style);
        o["thickness"] = pc.thickness;
        o["color"] = pc.color.name();
        plotsArray.append(o);
    }
    root["plots"] = plotsArray;

    QJsonDocument doc(root);
    QFile f(cfgPath);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(doc.toJson());
        f.close();
    } else {
        qWarning() << "Could not write config file:" << cfgPath;
    }
}

bool MainWindow::loadConfigForFile(const QString& filePath)
{
    if (filePath.isEmpty()) return false;
    QString cfgPath = filePath + ".rtplotter.json";
    QFile f(cfgPath);
    if (!f.exists()) return false;
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll();
    f.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError) return false;
    QJsonObject root = doc.object();
    if (root.contains("parser")) {
        QJsonObject p = root["parser"].toObject();
        if (p.contains("separator")) {
            QString s = p["separator"].toString();
            if (!s.isEmpty()) m_reader.setSeparator(s.at(0));
        }
        if (p.contains("startLine")) m_reader.setStartLine(p["startLine"].toInt());
        if (p.contains("hasHeader")) m_reader.setHasHeader(p["hasHeader"].toBool());
        if (p.contains("ignoreNonNumeric")) m_reader.setIgnoreNonNumeric(p["ignoreNonNumeric"].toBool());
        m_reader.setFile(filePath);
        m_reader.parse();
    }
    if (root.contains("plots")) {
        QJsonArray arr = root["plots"].toArray();
        QList<PlotConfig> configs;
        for (const QJsonValue& v : arr) {
            QJsonObject o = v.toObject();
            PlotConfig pc;
            pc.name = o["name"].toString();
            pc.role = static_cast<PlotConfig::Role>(o["role"].toInt());
            pc.graph = o["graph"].toInt();
            pc.style = static_cast<PlotConfig::Style>(o["style"].toInt());
            pc.thickness = o["thickness"].toInt();
            pc.color = QColor(o["color"].toString());
            configs.append(pc);
        }
        setPlotConfig(configs);
        setupPlots();
    }
    return true;
}

void MainWindow::importCSV(const QString& filePath)
{
    QString filePathLocal = filePath;
    if (filePathLocal.isEmpty()) {
        qDebug() << "importCSV: showing Open CSV dialog";
        filePathLocal = QFileDialog::getOpenFileName(this, tr("Open CSV File"), "", tr("CSV Files (*.csv);;All Files (*)"), nullptr, QFileDialog::DontUseNativeDialog);
    }
    if (filePathLocal.isEmpty()) return;

    // Try to read sidecar JSON (parser/plot configs) if present
    QJsonObject sidecarObj;
    {
        QFile f(filePathLocal + ".rtplotter.json");
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            if (doc.isObject()) sidecarObj = doc.object();
            f.close();
        }
    }

    // Open parser dialog pre-filled with the file path
    ParserConfigDialog dlg(this);
    dlg.setFilePath(filePathLocal);
    if (sidecarObj.contains("parser") && sidecarObj["parser"].isObject()) {
        dlg.applySettings(sidecarObj["parser"].toObject());
    }
    if (dlg.exec() == QDialog::Accepted) {
        CSVReader reader = dlg.getReader();
        if (reader.parse()) {
            // update reader
            m_reader = reader;
            m_fileWatcher.watchFile(m_reader.getFilePath());

            // mark dirty (user changed config)
            m_dirty = true;
            updateStatusBar();

            // If sidecar contains plot configs, prepare initial configs
            QList<PlotConfig> initialConfigs;
            if (sidecarObj.contains("plots") && sidecarObj["plots"].isArray()) {
                QJsonArray arr = sidecarObj["plots"].toArray();
                for (const QJsonValue& v : arr) {
                    if (!v.isObject()) continue;
                    QJsonObject o = v.toObject();
                    PlotConfig pc;
                    pc.name = o.value("name").toString();
                    pc.role = static_cast<PlotConfig::Role>(o.value("role").toInt());
                    pc.graph = o.value("graph").toInt();
                    pc.style = static_cast<PlotConfig::Style>(o.value("style").toInt());
                    pc.thickness = o.value("thickness").toInt();
                    QString col = o.value("color").toString();
                    pc.color = QColor(col);
                    initialConfigs.append(pc);
                }
            }

            // Show plot configuration dialog after loading data
            PlotConfigDialog plotDlg(m_reader.getHeaders(), initialConfigs, this);
            if (plotDlg.exec() == QDialog::Accepted) {
                setPlotConfig(plotDlg.getPlotConfig());
                setupPlots();
                // save sidecar with parser + plots
                QJsonObject root;
                root["parser"] = m_reader.toJson();
                QJsonArray parr;
                for (const PlotConfig &pc : m_plotConfigs) {
                    QJsonObject o;
                    o["name"] = pc.name;
                    o["role"] = static_cast<int>(pc.role);
                    o["graph"] = pc.graph;
                    o["style"] = static_cast<int>(pc.style);
                    o["thickness"] = pc.thickness;
                    o["color"] = pc.color.name();
                    parr.append(o);
                }
                root["plots"] = parr;
                QFile sf(m_reader.getFilePath() + ".rtplotter.json");
                if (sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    sf.write(QJsonDocument(root).toJson());
                    sf.close();
                }
                // After saving sidecar, mark project dirty (unless saved later)
                m_dirty = true;
                updateStatusBar();
            }
        }
    }
}

void MainWindow::saveProjectToPath(const QString& path)
{
    if (path.isEmpty()) return;
    QString outPath = path;
    if (!outPath.endsWith(".rtp")) outPath += ".rtp";

    QJsonObject root;
    root["dataFile"] = m_reader.getFilePath();
    root["parser"] = m_reader.toJson();
    QJsonArray parr;
    for (const PlotConfig &pc : m_plotConfigs) {
        QJsonObject o;
        o["name"] = pc.name;
        o["role"] = static_cast<int>(pc.role);
        o["graph"] = pc.graph;
        o["style"] = static_cast<int>(pc.style);
        o["thickness"] = pc.thickness;
        o["color"] = pc.color.name();
        parr.append(o);
    }
    root["plots"] = parr;
    root["paused"] = m_paused;

    QSaveFile f(outPath);
    if (!f.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, tr("Save Project"), tr("Could not write project file: %1").arg(outPath));
        return;
    }
    f.write(QJsonDocument(root).toJson());
    if (!f.commit()) {
        QMessageBox::warning(this, tr("Save Project"), tr("Could not commit project file: %1").arg(outPath));
        return;
    }
    m_projectPath = outPath;
    m_dirty = false;
    updateStatusBar();
}

bool MainWindow::loadProjectFromPath(const QString& path)
{
    if (path.isEmpty()) return false;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, tr("Open Project"), tr("Could not open project file: %1").arg(path));
        return false;
    }
    QByteArray data = f.readAll();
    f.close();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, tr("Open Project"), tr("Invalid project file: %1").arg(path));
        return false;
    }
    QJsonObject root = doc.object();
    // Load parser
    if (root.contains("parser") && root["parser"].isObject()) {
        m_reader.fromJson(root["parser"].toObject());
    }
    // If dataFile present, set and parse
    if (root.contains("dataFile")) {
        QString df = root["dataFile"].toString();
        if (!df.isEmpty()) {
            m_reader.setFile(df);
            if (!m_reader.parse()) {
                QMessageBox::warning(this, tr("Open Project"), tr("Failed to parse data file: %1").arg(df));
                // continue, but plots won't be shown
            }
        }
    }
    // Load plots
    QList<PlotConfig> configs;
    if (root.contains("plots") && root["plots"].isArray()) {
        QJsonArray arr = root["plots"].toArray();
        for (const QJsonValue &v : arr) {
            if (!v.isObject()) continue;
            QJsonObject o = v.toObject();
            PlotConfig pc;
            pc.name = o.value("name").toString();
            pc.role = static_cast<PlotConfig::Role>(o.value("role").toInt());
            pc.graph = o.value("graph").toInt();
            pc.style = static_cast<PlotConfig::Style>(o.value("style").toInt());
            pc.thickness = o.value("thickness").toInt();
            pc.color = QColor(o.value("color").toString());
            configs.append(pc);
        }
    }
    setPlotConfig(configs);
    // Set reader and setup plots (no dialogs)
    setReader(m_reader);
    // paused state
    if (root.contains("paused")) {
        m_paused = root["paused"].toBool();
        if (m_paused) ui->actionPause->setIcon(loadColoredIcon(":/icons/icons/resume.svg", QColor("#4CAF50")));
        else ui->actionPause->setIcon(loadThemeAwareIcon(":/icons/icons/pause.svg"));
    }
    m_projectPath = path;
    m_dirty = false;
    updateStatusBar();
    return true;
}

void MainWindow::onFileChanged(const QString& path)
{
    try {
        m_reader.readNewLines();
        QVector<QVector<double>> data = m_reader.getData();
        QStringList headers = m_reader.getHeaders();

        if (data.isEmpty() || headers.isEmpty()) {
            return;
        }

        // Trouver l'index de l'axe X
        int x_axis_index = -1;
        for(int i=0; i < m_plotConfigs.size(); ++i)
        {
            if(m_plotConfigs[i].role == PlotConfig::XAxis)
            {
                x_axis_index = headers.indexOf(m_plotConfigs[i].name);
                break;
            }
        }

        if(x_axis_index == -1 || x_axis_index >= data.first().size()) {
            return;
        }

        QVector<double> x_data;
        for(int i=0; i < data.size(); ++i)
        {
            if (x_axis_index < data[i].size()) {
                x_data.append(data[i][x_axis_index]);
            }
        }

        if (x_data.isEmpty()) {
            return;
        }

        for (int i = 0; i < m_plotConfigs.size(); ++i)
        {
            if (m_plotConfigs[i].role == PlotConfig::YAxis)
            {
                // Trouver l'index réel de cette variable
                int y_axis_index = headers.indexOf(m_plotConfigs[i].name);
                if (y_axis_index == -1 || y_axis_index >= data.first().size()) {
                    continue;
                }

                QVector<double> y_data;
                for(int j=0; j < data.size(); ++j)
                {
                    if (y_axis_index < data[j].size()) {
                        y_data.append(data[j][y_axis_index]);
                    }
                }

                if (!y_data.isEmpty()) {
                    m_plotManager.updateCurve(m_plotConfigs[i].graph, m_plotConfigs[i].name, x_data, y_data);
                    m_plotManager.resetZoom(m_plotConfigs[i].graph);
                }
            }
        }
    } catch (const std::exception& e) {
        qCritical() << "Exception in onFileChanged():" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in onFileChanged()";
    }
}

void MainWindow::setupPlots()
{
    try {
        QVector<QVector<double>> data = m_reader.getData();
        QStringList headers = m_reader.getHeaders();

        qDebug() << "setupPlots() called";
        qDebug() << "Data size:" << data.size();
        qDebug() << "Headers:" << headers;
        qDebug() << "PlotConfigs size:" << m_plotConfigs.size();

        // Vérification basique
        if (data.isEmpty() || headers.isEmpty()) {
            qWarning() << "setupPlots(): No data or headers found!";
            return;
        }

        if (m_plotConfigs.isEmpty()) {
            qWarning() << "setupPlots(): No plot configs!";
            return;
        }

        // Trouver l'index de l'axe X
        int x_axis_index = -1;
        for(int i=0; i < m_plotConfigs.size(); ++i)
        {
            if(m_plotConfigs[i].role == PlotConfig::XAxis)
            {
                x_axis_index = headers.indexOf(m_plotConfigs[i].name);
                qDebug() << "X-Axis found:" << m_plotConfigs[i].name << "at index" << x_axis_index;
                break;
            }
        }

        if(x_axis_index == -1) {
            qWarning() << "No X-Axis found!";
            return;
        }

        if (x_axis_index >= data.first().size()) {
            qWarning() << "X-Axis index out of bounds:" << x_axis_index << ">=" << data.first().size();
            return;
        }

        QVector<double> x_data;
        for(int i=0; i < data.size(); ++i)
        {
            if (x_axis_index < data[i].size()) {
                x_data.append(data[i][x_axis_index]);
            } else {
                qWarning() << "Row" << i << "has insufficient columns";
            }
        }

        if (x_data.isEmpty()) {
            qWarning() << "X data is empty!";
            return;
        }

        // Clear existing plots/widgets before creating new ones
        while (ui->splitter->count() > 0) {
            QWidget* w = ui->splitter->widget(0);
            if (w) {
                w->setParent(nullptr);
                delete w;
            }
        }
        m_plotManager.clearPlots();

        QMap<int, QCustomPlot*> plots;
        for (int i = 0; i < m_plotConfigs.size(); ++i)
        {
            if (m_plotConfigs[i].role == PlotConfig::YAxis)
            {
                int graphNum = m_plotConfigs[i].graph;
                if (!plots.contains(graphNum))
                {
                    QCustomPlot* newPlot = new QCustomPlot();
                    newPlot->setInteraction(QCP::iRangeDrag, true);
                    newPlot->setInteraction(QCP::iRangeZoom, true);
                    newPlot->legend->setVisible(true);
                    ui->splitter->addWidget(newPlot);
                    m_plotManager.addPlot(graphNum, newPlot);
                    plots[graphNum] = newPlot;
                    qDebug() << "Created plot:" << graphNum;
                }

                // Trouver l'index réel de cette variable dans les données
                int y_axis_index = headers.indexOf(m_plotConfigs[i].name);
                if (y_axis_index == -1) {
                    qWarning() << "Y-Axis variable not found:" << m_plotConfigs[i].name;
                    continue;
                }

                if (y_axis_index >= data.first().size()) {
                    qWarning() << "Y-Axis index out of bounds:" << y_axis_index << ">=" << data.first().size();
                    continue;
                }

                QVector<double> y_data;
                for(int j=0; j < data.size(); ++j)
                {
                    if (y_axis_index < data[j].size()) {
                        y_data.append(data[j][y_axis_index]);
                    } else {
                        qWarning() << "Row" << j << "has insufficient columns for Y data";
                    }
                }

                if (y_data.isEmpty()) {
                    qWarning() << "Y data is empty for:" << m_plotConfigs[i].name;
                    continue;
                }

                qDebug() << "Adding curve:" << m_plotConfigs[i].name << "to plot" << graphNum;
                m_plotManager.addCurve(graphNum, m_plotConfigs[i], x_data, y_data);
            }
        }

        qDebug() << "setupPlots() finished";
    } catch (const std::exception& e) {
        qCritical() << "Exception in setupPlots():" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception in setupPlots()";
    }
}

void MainWindow::on_actionPause_triggered()
{
    // Toggle pause state
    if (!m_paused) {
        // Pause
        m_fileWatcher.stop();
        m_paused = true;
        ui->actionPause->setIcon(loadColoredIcon(":/icons/icons/resume.svg", QColor("#4CAF50")));
        ui->actionPause->setText(tr("Resume"));
        ui->actionPause->setToolTip(tr("Resume updates"));
    } else {
        // Resume
        m_fileWatcher.watchFile(m_reader.getFilePath());
        m_paused = false;
        ui->actionPause->setIcon(loadThemeAwareIcon(":/icons/icons/pause.svg"));
        ui->actionPause->setText(tr("Pause"));
        ui->actionPause->setToolTip(tr("Pause updates"));
    }
}

void MainWindow::on_actionResetZoom_triggered()
{
    for (QCustomPlot* plot : m_plotManager.getPlots())
    {
        plot->rescaleAxes();
        plot->replot();
    }
}

void MainWindow::on_actionExport_triggered()
{
    qDebug() << "actionExport triggered";
    QString filePath = QFileDialog::getSaveFileName(this, "Export Plot", "", "PNG (*.png);;JPEG (*.jpg);;PDF (*.pdf)", nullptr, QFileDialog::DontUseNativeDialog);
    if (!filePath.isEmpty())
    {
        QCustomPlot* plot = qobject_cast<QCustomPlot*>(ui->splitter->widget(0));
        if (plot)
        {
            if (filePath.endsWith(".png"))
                plot->savePng(filePath);
            else if (filePath.endsWith(".jpg"))
                plot->saveJpg(filePath);
            else if (filePath.endsWith(".pdf"))
                plot->savePdf(filePath);
        }
    }
}
