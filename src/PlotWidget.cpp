#include "PlotWidget.h"

#include <QPen>
#include <QColor>
#include <QVector>
#include <iostream>

PlotWidget::PlotWidget(QWidget *parent)
    : QCustomPlot(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setBackground(QColor(255, 255, 255));

    xAxis->setLabel("Time (samples)");
    yAxis->setLabel("Value");
    xAxis->setRange(0, 200);

    m_rawGraph = addGraph();
    m_rawGraph->setPen(QPen(QColor(0, 0, 255), 2));
    m_rawGraph->setName("Raw Data");

    m_firGraph = addGraph();
    m_firGraph->setPen(QPen(QColor(255, 0, 0), 2));
    m_firGraph->setName("FIR Filter");

    m_iirGraph = addGraph();
    m_iirGraph->setPen(QPen(QColor(0, 200, 0), 2));
    m_iirGraph->setName("IIR Filter");

    legend->setVisible(true);
    legend->setFont(QFont("Arial", 8));

    m_title = new QCPTextElement(this);
    m_title->setText("Data from Model");
    m_title->setFont(QFont("Arial", 12, QFont::Bold));
    plotLayout()->insertRow(0);
    plotLayout()->addElement(0, 0, m_title);

    setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void PlotWidget::setPlotTitle(const QString &title)
{
    if (m_title)
        m_title->setText(title);
}

void PlotWidget::setAxisLabels(const QString &xLabel, const QString &yLabel)
{
    xAxis->setLabel(xLabel);
    yAxis->setLabel(yLabel);
}

void PlotWidget::setPointCount(int count)
{
    m_maxPoints = count;
    xAxis->setRange(0, count);
}

void PlotWidget::setAutoScale(bool enabled)
{
    m_autoScale = enabled;
}

void PlotWidget::setCustomYRange(double min, double max)
{
    m_customYMin = min;
    m_customYMax = max;
}

void PlotWidget::resizeEvent(QResizeEvent *event)
{
    QCustomPlot::resizeEvent(event);
    replot();
}

void PlotWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_autoScale)
    {
        double zoomFactor = 1.0 + (event->angleDelta().y() / 1200.0);
        double center = yAxis->range().center();
        double range = yAxis->range().size() / zoomFactor;

        if (range > 0.001)
        {
            yAxis->setRange(center - range / 2, center + range / 2);
            replot();
            emit wheelZoom(zoomFactor, event->posF());
        }
    }
    event->accept();
}

void PlotWidget::updateData(
    const QVector<double> &keys,
    const QVector<double> &raw,
    const QVector<double> &fir,
    const QVector<double> &iir)
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (raw.isEmpty() || keys.isEmpty())
        return;

    m_rawGraph->setData(keys, raw);
    m_firGraph->setData(keys, fir);
    m_iirGraph->setData(keys, iir);

    if (m_autoScale)
    {
        yAxis->rescale();
    }

    replot();
}