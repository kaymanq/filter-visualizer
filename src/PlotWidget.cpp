#include "PlotWidget.h"

#include <QPen>
#include <QColor>
#include <QVector>

PlotWidget::PlotWidget(QWidget *parent)
    : QCustomPlot(parent)
{

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

void PlotWidget::updateData(
    const std::vector<float> &raw,
    const std::vector<float> &fir,
    const std::vector<float> &iir)
{

    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (raw.empty())
        return;

    QVector<double> keys(raw.size());
    QVector<double> rawData(raw.size());
    QVector<double> firData(fir.size());
    QVector<double> iirData(iir.size());

    for (size_t i = 0; i < raw.size(); ++i)
    {
        keys[i] = static_cast<double>(i);
        rawData[i] = static_cast<double>(raw[i]);
    }

    for (size_t i = 0; i < fir.size(); ++i)
    {
        firData[i] = static_cast<double>(fir[i]);
    }

    for (size_t i = 0; i < iir.size(); ++i)
    {
        iirData[i] = static_cast<double>(iir[i]);
    }

    m_rawGraph->setData(keys, rawData);
    m_firGraph->setData(keys, firData);
    m_iirGraph->setData(keys, iirData);

    yAxis->rescale();
    replot();
}