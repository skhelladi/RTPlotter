#ifndef PLOTMANAGER_H
#define PLOTMANAGER_H

#include <QObject>
#include <QMap>
#include "qcustomplot.h"
#include "PlotConfigDialog.h"

class PlotManager : public QObject
{
    Q_OBJECT
public:
    explicit PlotManager(QObject *parent = nullptr);
    void addPlot(int plotId, QCustomPlot* plot);
    void addCurve(int plotId, const PlotConfig& config, const QVector<double>& x, const QVector<double>& y);
    void updateCurve(int plotId, const QString& name, const QVector<double>& newX, const QVector<double>& newY);
    void resetZoom(int plotId);
    QList<QCustomPlot*> getPlots() const;
    void clearPlots();

private:
    QMap<int, QCustomPlot*> m_plots;
};

#endif // PLOTMANAGER_H
