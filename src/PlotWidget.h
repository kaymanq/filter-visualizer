#ifndef PLOTWIDGET_H
#define PLOTWIDGET_H

#include <QWidget>
#include "qcustomplot.h"
#include <vector>
#include <mutex>

class PlotWidget : public QCustomPlot
{
    Q_OBJECT

public:
    explicit PlotWidget(QWidget *parent = nullptr);

    void setPlotTitle(const QString &title);
    void setAxisLabels(const QString &xLabel, const QString &yLabel);
    void setPointCount(int count);

    void updateData(
        const QVector<double> &keys,
        const QVector<double> &raw,
        const QVector<double> &fir,
        const QVector<double> &iir);

private:
    QCPGraph *m_rawGraph{nullptr};
    QCPGraph *m_firGraph{nullptr};
    QCPGraph *m_iirGraph{nullptr};

    int m_maxPoints{200};
    std::mutex m_dataMutex;
    QCPTextElement *m_title{nullptr};
};

#endif // PLOTWIDGET_H