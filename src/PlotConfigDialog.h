#ifndef PLOTCONFIGDIALOG_H
#define PLOTCONFIGDIALOG_H

#include <QDialog>
#include <QStringList>
#include <QColor>

namespace Ui {
class PlotConfigDialog;
}

struct PlotConfig {
    QString name;
    enum Role { None, XAxis, YAxis };
    Role role;
    int graph;
    enum Style { Line, Points, LineAndPoints };
    Style style;
    int thickness;
    QColor color;
};

class PlotConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PlotConfigDialog(const QStringList& variables, const QList<PlotConfig>& initialConfigs = QList<PlotConfig>(), QWidget *parent = nullptr);
    ~PlotConfigDialog();

    QList<PlotConfig> getPlotConfig() const;

private slots:
    void onColorButtonClicked();

private:
    Ui::PlotConfigDialog *ui;
    QStringList m_variables;
    QMap<int, QColor> m_colors;
    QList<PlotConfig> m_initialConfigs;

    void setupTable();
    void updateColorButtonDisplay(int row, const QColor& color);
};

#endif // PLOTCONFIGDIALOG_H
