#include "PlotWidget.h"

#include <QPen>
#include <QColor>
#include <QVector>
#include <iostream>

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
    const QVector<double> &keys,
    const QVector<double> &raw,
    const QVector<double> &fir,
    const QVector<double> &iir)
{

    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (raw.isEmpty() || keys.isEmpty())
        return;

    int size = keys.size();
    if (raw.size() != size || fir.size() != size || iir.size() != size)
    {
        static int warnCounter = 0;
        if (warnCounter++ % 100 == 0)
        {
            std::cout << "PlotWidget: размеры не совпадают! keys=" << size
                      << ", raw=" << raw.size()
                      << ", fir=" << fir.size()
                      << ", iir=" << iir.size() << std::endl;
        }
        QVector<double> rawFixed(size), firFixed(size), iirFixed(size);
        for (int i = 0; i < size; ++i)
        {
            rawFixed[i] = (i < raw.size()) ? raw[i] : 0.0;
            firFixed[i] = (i < fir.size()) ? fir[i] : 0.0;
            iirFixed[i] = (i < iir.size()) ? iir[i] : 0.0;
        }
        m_rawGraph->setData(keys, rawFixed);
        m_firGraph->setData(keys, firFixed);
        m_iirGraph->setData(keys, iirFixed);
    }
    else
    {
        m_rawGraph->setData(keys, raw);
        m_firGraph->setData(keys, fir);
        m_iirGraph->setData(keys, iir);
    }

    yAxis->rescale();
    replot();
}