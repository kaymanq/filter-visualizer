#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QLabel>

#include "UDPReceiver.h"
#include "UDPSender.h"
#include "FIRFilter.h"
#include "IIRFilter.h"
#include "DataBuffer.h"
#include "PlotWidget.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void dataReceived(const DataPoint &point);
    void firFiltered(const DataPoint &point);
    void iirFiltered(const DataPoint &point);

private slots:
    void onReceiveData(const DataPoint &point);
    void onFIRFiltered(const DataPoint &point);
    void onIIRFiltered(const DataPoint &point);
    void onSendTarget();
    void onUpdatePlot();
    void onStartStop();
    void onAlgorithmChanged();

private:
    void setupUI();
    void startAll();
    void stopAll();

    // Сетевые компоненты
    UDPReceiver m_udpReceiver;
    UDPSender m_udpSender;

    // Фильтры
    FIRFilter m_firFilter;
    IIRFilter m_iirFilter;

    // Буферы данных
    DataBuffer m_rawBuffer;
    DataBuffer m_firBuffer;
    DataBuffer m_iirBuffer;

    // Виджет графика
    PlotWidget *m_plotWidget{nullptr};

    // Элементы GUI
    QLineEdit *m_targetEdit{nullptr};
    QPushButton *m_sendButton{nullptr};
    QPushButton *m_startStopButton{nullptr};

    QLineEdit *m_receiveAddrEdit{nullptr};
    QLineEdit *m_receivePortEdit{nullptr};
    QLineEdit *m_sendAddrEdit{nullptr};
    QLineEdit *m_sendPortEdit{nullptr};

    QComboBox *m_firAlgorithmCombo{nullptr};
    QSpinBox *m_windowSizeSpin{nullptr};
    QDoubleSpinBox *m_firCutoffSpin{nullptr};

    QComboBox *m_iirAlgorithmCombo{nullptr};
    QDoubleSpinBox *m_alphaSpin{nullptr};
    QDoubleSpinBox *m_iirCutoffSpin{nullptr};
    QSpinBox *m_iirOrderSpin{nullptr};

    QSpinBox *m_plotSizeSpin{nullptr};
    QLabel *m_statusLabel{nullptr};

    QTimer *m_plotTimer{nullptr};
    bool m_isRunning{false};
};

#endif // MAINWINDOW_H