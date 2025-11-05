#include "PlotManager.h"
#include <QDebug>
#include <stdexcept>

PlotManager::PlotManager(QObject *parent) : QObject(parent)
{

}

void PlotManager::addPlot(int plotId, QCustomPlot* plot)
{
    m_plots[plotId] = plot;
}

void PlotManager::addCurve(int plotId, const PlotConfig& config, const QVector<double>& x, const QVector<double>& y)
{
    qDebug() << "Adding curve:" << config.name << "to plot" << plotId;
    qDebug() << "x data size:" << x.size();
    qDebug() << "y data size:" << y.size();
    
    // Protection: vérifier que les données sont valides
    if (x.isEmpty() || y.isEmpty()) {
        qWarning() << "Cannot add curve: empty data vectors";
        return;
    }
    
    if (x.size() != y.size()) {
        qWarning() << "Cannot add curve: X and Y data sizes don't match (" << x.size() << "vs" << y.size() << ")";
        return;
    }
    
    if (!m_plots.contains(plotId)) {
        qWarning() << "Cannot add curve: plot" << plotId << "does not exist";
        return;
    }
    
    try {
        QCustomPlot* plot = m_plots[plotId];
        plot->addGraph();
        plot->graph()->setName(config.name);
        plot->graph()->setData(x, y);

        QPen pen;
        pen.setColor(config.color);
        pen.setWidth(config.thickness);
        plot->graph()->setPen(pen);

        if (config.style == PlotConfig::LineAndPoints)
        {
            plot->graph()->setScatterStyle(QCPScatterStyle::ssCircle);
        }
        else if (config.style == PlotConfig::Points)
        {
            plot->graph()->setLineStyle(QCPGraph::lsNone);
            plot->graph()->setScatterStyle(QCPScatterStyle::ssCircle);
        }

        plot->rescaleAxes();
        plot->replot();
    } catch (const std::exception& e) {
        qCritical() << "Exception while adding curve:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception while adding curve";
    }
}

void PlotManager::updateCurve(int plotId, const QString& name, const QVector<double>& newX, const QVector<double>& newY)
{
    qDebug() << "Updating curve:" << name << "in plot" << plotId;
    qDebug() << "newX data size:" << newX.size();
    qDebug() << "newY data size:" << newY.size();
    
    // Protection: vérifier que les données sont valides
    if (newX.isEmpty() || newY.isEmpty()) {
        qWarning() << "Cannot update curve: empty data vectors";
        return;
    }
    
    if (newX.size() != newY.size()) {
        qWarning() << "Cannot update curve: X and Y data sizes don't match (" << newX.size() << "vs" << newY.size() << ")";
        return;
    }
    
    if (!m_plots.contains(plotId)) {
        qWarning() << "Cannot update curve: plot" << plotId << "does not exist";
        return;
    }
    
    try {
        QCustomPlot* plot = m_plots[plotId];
        for (int i = 0; i < plot->graphCount(); ++i)
        {
            if (plot->graph(i)->name() == name)
            {
                plot->graph(i)->setData(newX, newY);
                plot->rescaleAxes();
                plot->replot();
                return;
            }
        }
        qWarning() << "Curve not found:" << name;
    } catch (const std::exception& e) {
        qCritical() << "Exception while updating curve:" << e.what();
    } catch (...) {
        qCritical() << "Unknown exception while updating curve";
    }
}

void PlotManager::resetZoom(int plotId)
{
    if (m_plots.contains(plotId))
    {
        m_plots[plotId]->rescaleAxes();
        m_plots[plotId]->replot();
    }
}

QList<QCustomPlot*> PlotManager::getPlots() const
{
    return m_plots.values();
}

void PlotManager::clearPlots()
{
    // Do not delete widgets here: the MainWindow is responsible for removing
    // and deleting splitter child widgets. Just clear the internal map.
    m_plots.clear();
}
