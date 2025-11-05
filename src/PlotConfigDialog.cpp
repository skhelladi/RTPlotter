#include "PlotConfigDialog.h"
#include "ui_PlotConfigDialog.h"
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColorDialog>

PlotConfigDialog::PlotConfigDialog(const QStringList& variables, const QList<PlotConfig>& initialConfigs, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PlotConfigDialog),
    m_variables(variables),
    m_initialConfigs(initialConfigs)
{
    ui->setupUi(this);
    setupTable();
}

PlotConfigDialog::~PlotConfigDialog()
{
    delete ui;
}

void PlotConfigDialog::setupTable()
{
    ui->plotConfigTableWidget->setRowCount(m_variables.size());
    // Palette of default colors to assign per variable (cycled)
    QVector<QColor> defaultColors = {Qt::red, Qt::green, Qt::blue, Qt::magenta, Qt::cyan, Qt::yellow, Qt::darkRed, Qt::darkGreen, Qt::darkBlue};
    for (int i = 0; i < m_variables.size(); ++i) {
        ui->plotConfigTableWidget->setItem(i, 0, new QTableWidgetItem(m_variables.at(i)));

        QComboBox* roleComboBox = new QComboBox();
        roleComboBox->addItems({"None", "X-Axis", "Y-Axis"});
        ui->plotConfigTableWidget->setCellWidget(i, 1, roleComboBox);

        QSpinBox* graphSpinBox = new QSpinBox();
        graphSpinBox->setMinimum(0);
        ui->plotConfigTableWidget->setCellWidget(i, 2, graphSpinBox);

        QComboBox* styleComboBox = new QComboBox();
        styleComboBox->addItems({"Line", "Points", "Line+Points"});
        ui->plotConfigTableWidget->setCellWidget(i, 3, styleComboBox);

        QSpinBox* thicknessSpinBox = new QSpinBox();
        thicknessSpinBox->setMinimum(1);
        thicknessSpinBox->setMaximum(10);
        ui->plotConfigTableWidget->setCellWidget(i, 4, thicknessSpinBox);

        QPushButton* colorButton = new QPushButton("Choose Color");
        colorButton->setProperty("row", i);  // Store the row number for reference
        QColor def = defaultColors[i % defaultColors.size()];
        // Default color first
        m_colors[i] = def;
        // If initial configs supplied, use those values (match by name)
        if (!m_initialConfigs.isEmpty()) {
            for (const PlotConfig &pc : m_initialConfigs) {
                if (pc.name == m_variables.at(i)) {
                    roleComboBox->setCurrentIndex(static_cast<int>(pc.role));
                    graphSpinBox->setValue(pc.graph);
                    styleComboBox->setCurrentIndex(static_cast<int>(pc.style));
                    thicknessSpinBox->setValue(pc.thickness);
                    if (pc.color.isValid()) m_colors[i] = pc.color;
                    break;
                }
            }
        }
        updateColorButtonDisplay(i, m_colors[i]);
        connect(colorButton, &QPushButton::clicked, this, &PlotConfigDialog::onColorButtonClicked);
        ui->plotConfigTableWidget->setCellWidget(i, 5, colorButton);
    }
}

void PlotConfigDialog::updateColorButtonDisplay(int row, const QColor& color)
{
    QPushButton* button = qobject_cast<QPushButton*>(ui->plotConfigTableWidget->cellWidget(row, 5));
    if (button) {
        // Create a small colored pixmap to use as an icon so the color is visible on all styles
        QPixmap pix(16,16);
        pix.fill(color);
        button->setIcon(QIcon(pix));
        button->setIconSize(QSize(16,16));
        // Keep a short tooltip with the hex color
        button->setToolTip(color.name());
        // Set minimal stylesheet so text doesn't obscure icon
        button->setStyleSheet("padding:2px;");
    }
}

QList<PlotConfig> PlotConfigDialog::getPlotConfig() const
{
    QList<PlotConfig> configs;
    for (int i = 0; i < m_variables.size(); ++i) {
        PlotConfig config;
        config.name = m_variables.at(i);
        config.role = static_cast<PlotConfig::Role>(static_cast<QComboBox*>(ui->plotConfigTableWidget->cellWidget(i, 1))->currentIndex());
        config.graph = static_cast<QSpinBox*>(ui->plotConfigTableWidget->cellWidget(i, 2))->value();
        config.style = static_cast<PlotConfig::Style>(static_cast<QComboBox*>(ui->plotConfigTableWidget->cellWidget(i, 3))->currentIndex());
        config.thickness = static_cast<QSpinBox*>(ui->plotConfigTableWidget->cellWidget(i, 4))->value();
        config.color = m_colors.value(i, Qt::blue);  // Récupérer la couleur stockée
        configs.append(config);
    }
    return configs;
}

void PlotConfigDialog::onColorButtonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button) {
        int row = button->property("row").toInt();
        QColor initialColor = m_colors.value(row, Qt::blue);
        QColor color = QColorDialog::getColor(initialColor, this, "Choisir une couleur");
        if (color.isValid()) {
            m_colors[row] = color;  // Stocker la couleur sélectionnée
            updateColorButtonDisplay(row, color);
        }
    }
}
