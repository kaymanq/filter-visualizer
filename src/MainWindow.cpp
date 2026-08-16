#include "MainWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QMessageBox>
#include <QStatusBar>
#include <QIntValidator>
#include <QDebug>
#include <iostream>
#include <map>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    std::cout << "MainWindow: конструктор НАЧАЛО" << std::endl;
    std::cout.flush();

    setupUI();

    connect(this, &MainWindow::dataReceived,
            this, &MainWindow::onReceiveData, Qt::QueuedConnection);
    connect(this, &MainWindow::firFiltered,
            this, &MainWindow::onFIRFiltered, Qt::QueuedConnection);
    connect(this, &MainWindow::iirFiltered,
            this, &MainWindow::onIIRFiltered, Qt::QueuedConnection);

    m_udpReceiver.setDataCallback([this](const DataPoint &p)
                                  { emit dataReceived(p); });

    m_firFilter.setOutputCallback([this](const DataPoint &p)
                                  { emit firFiltered(p); });

    m_iirFilter.setOutputCallback([this](const DataPoint &p)
                                  { emit iirFiltered(p); });

    m_plotTimer = new QTimer(this);
    connect(m_plotTimer, &QTimer::timeout, this, &MainWindow::onUpdatePlot);
    m_plotTimer->start(33);

    m_statusLabel = new QLabel("Ready. Press 'Start' to begin.");
    statusBar()->addWidget(m_statusLabel);

    std::cout << "MainWindow: конструктор ЗАВЕРШЕН" << std::endl;
    std::cout.flush();
}

MainWindow::~MainWindow()
{
    std::cout << "MainWindow: деструктор" << std::endl;
    stopAll();
}

void MainWindow::setupUI()
{
    std::cout << "MainWindow::setupUI: НАЧАЛО" << std::endl;
    std::cout.flush();

    setWindowTitle("Filter Visualizer");
    resize(1300, 900);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);

    m_plotWidget = new PlotWidget(this);
    m_plotWidget->setMinimumHeight(400);
    m_plotWidget->setPlotTitle("Data from Model");
    m_plotWidget->setAxisLabels("Time (samples)", "Value");
    mainLayout->addWidget(m_plotWidget);

    QTabWidget *tabs = new QTabWidget(this);

    QWidget *netTab = new QWidget();
    QFormLayout *netLayout = new QFormLayout(netTab);

    m_receiveAddrEdit = new QLineEdit("127.0.0.1");
    m_receivePortEdit = new QLineEdit("50006");
    m_sendAddrEdit = new QLineEdit("127.0.0.1");
    m_sendPortEdit = new QLineEdit("50005");

    QIntValidator *portValidator = new QIntValidator(1, 65535, this);
    m_receivePortEdit->setValidator(portValidator);
    m_sendPortEdit->setValidator(portValidator);

    netLayout->addRow("Receive IP:", m_receiveAddrEdit);
    netLayout->addRow("Receive Port:", m_receivePortEdit);
    netLayout->addRow("Send IP:", m_sendAddrEdit);
    netLayout->addRow("Send Port:", m_sendPortEdit);

    QLabel *infoLabel = new QLabel(
        "<font color='gray'>Receive: from model (port 50006)<br>"
        "Send: to model (port 50005)</font>");
    netLayout->addRow(infoLabel);
    tabs->addTab(netTab, "Network");

    QWidget *firTab = new QWidget();
    QFormLayout *firLayout = new QFormLayout(firTab);

    m_firAlgorithmCombo = new QComboBox();
    m_firAlgorithmCombo->addItem("Boxcar (Moving Average)", 0);
    m_firAlgorithmCombo->addItem("Hamming", 1);
    m_firAlgorithmCombo->addItem("Blackman", 2);
    m_firAlgorithmCombo->addItem("Median (Non-linear)", 3);
    m_firAlgorithmCombo->addItem("Gaussian", 4);
    m_firAlgorithmCombo->addItem("Low-Pass (Sinc)", 5);
    firLayout->addRow("FIR Algorithm:", m_firAlgorithmCombo);

    m_windowSizeSpin = new QSpinBox();
    m_windowSizeSpin->setRange(3, 101);
    m_windowSizeSpin->setValue(11);
    m_windowSizeSpin->setSingleStep(2);
    firLayout->addRow("Window Size (odd):", m_windowSizeSpin);

    m_firCutoffSpin = new QDoubleSpinBox();
    m_firCutoffSpin->setRange(0.01, 0.99);
    m_firCutoffSpin->setSingleStep(0.05);
    m_firCutoffSpin->setValue(0.3);
    m_firCutoffSpin->setEnabled(false);
    firLayout->addRow("Cutoff Frequency (0-1):", m_firCutoffSpin);

    connect(m_firAlgorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAlgorithmChanged);
    connect(m_windowSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int v)
            {
                m_firFilter.setWindowSize(v);
            });
    connect(m_firCutoffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v)
            { m_firFilter.setCutoffFrequency(v); });

    tabs->addTab(firTab, "FIR Filters");

    QWidget *iirTab = new QWidget();
    QFormLayout *iirLayout = new QFormLayout(iirTab);

    m_alphaSpin = new QDoubleSpinBox();
    m_alphaSpin->setRange(0.01, 1.0);
    m_alphaSpin->setSingleStep(0.05);
    m_alphaSpin->setValue(0.3);
    iirLayout->addRow("Alpha (0-1):", m_alphaSpin);

    connect(m_alphaSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v)
            { m_iirFilter.setAlpha(static_cast<float>(v)); });

    tabs->addTab(iirTab, "IIR Filter");

    QWidget *displayTab = new QWidget();
    QFormLayout *displayLayout = new QFormLayout(displayTab);

    m_plotSizeSpin = new QSpinBox();
    m_plotSizeSpin->setRange(50, 1000);
    m_plotSizeSpin->setValue(200);
    m_plotSizeSpin->setSingleStep(10);
    displayLayout->addRow("Points on graph:", m_plotSizeSpin);

    connect(m_plotSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int v)
            {
                m_rawBuffer.setCapacity(v);
                m_firBuffer.setCapacity(v);
                m_iirBuffer.setCapacity(v);
                m_plotWidget->setPointCount(v);
                m_lastFirValue = 0.0f;
                m_lastIirValue = 0.0f;
            });

    tabs->addTab(displayTab, "Display");

    mainLayout->addWidget(tabs);

    QHBoxLayout *controlLayout = new QHBoxLayout();

    QLabel *targetLabel = new QLabel("Target Value:");
    controlLayout->addWidget(targetLabel);

    m_targetEdit = new QLineEdit("0.0");
    m_targetEdit->setFixedWidth(100);
    controlLayout->addWidget(m_targetEdit);

    m_sendButton = new QPushButton("Send Target");
    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendTarget);
    controlLayout->addWidget(m_sendButton);

    controlLayout->addStretch();

    m_startStopButton = new QPushButton("Start");
    m_startStopButton->setFixedWidth(100);
    connect(m_startStopButton, &QPushButton::clicked, this, &MainWindow::onStartStop);
    controlLayout->addWidget(m_startStopButton);

    mainLayout->addLayout(controlLayout);

    onAlgorithmChanged();

    std::cout << "MainWindow::setupUI: ЗАВЕРШЕНО" << std::endl;
    std::cout.flush();
}

void MainWindow::onAlgorithmChanged()
{
    int firIdx = m_firAlgorithmCombo->currentIndex();
    if (firIdx >= 0)
    {
        FIRFilter::Algorithm algo;
        switch (firIdx)
        {
        case 0:
            algo = FIRFilter::Algorithm::Boxcar;
            break;
        case 1:
            algo = FIRFilter::Algorithm::Hamming;
            break;
        case 2:
            algo = FIRFilter::Algorithm::Blackman;
            break;
        case 3:
            algo = FIRFilter::Algorithm::Median;
            break;
        case 4:
            algo = FIRFilter::Algorithm::Gaussian;
            break;
        case 5:
            algo = FIRFilter::Algorithm::LowPass;
            break;
        default:
            algo = FIRFilter::Algorithm::Boxcar;
            break;
        }
        m_firFilter.setAlgorithm(algo);
        m_firCutoffSpin->setEnabled(algo == FIRFilter::Algorithm::LowPass);
    }
}

void MainWindow::startAll()
{
    std::cout << "=== startAll: НАЧАЛО ===" << std::endl;
    std::cout.flush();

    if (m_isRunning)
    {
        std::cout << "startAll: уже запущен, выходим" << std::endl;
        return;
    }

    QString receiveAddr = m_receiveAddrEdit->text();
    int receivePort = m_receivePortEdit->text().toInt();
    QString sendAddr = m_sendAddrEdit->text();
    int sendPort = m_sendPortEdit->text().toInt();

    std::cout << "startAll: Receive = " << receiveAddr.toStdString() << " : " << receivePort << std::endl;
    std::cout << "startAll: Send = " << sendAddr.toStdString() << " : " << sendPort << std::endl;
    std::cout.flush();

    if (!m_udpReceiver.start(receiveAddr.toStdString(), receivePort))
    {
        QMessageBox::critical(this, "Error", "Failed to start UDP receiver");
        std::cout << "startAll: UDP Receiver НЕ запущен!" << std::endl;
        return;
    }
    std::cout << "startAll: UDP Receiver запущен" << std::endl;
    std::cout.flush();

    if (!m_udpSender.init(sendAddr.toStdString(), sendPort))
    {
        QMessageBox::critical(this, "Error", "Failed to init UDP sender");
        m_udpReceiver.stop();
        std::cout << "startAll: UDP Sender НЕ инициализирован!" << std::endl;
        return;
    }
    std::cout << "startAll: UDP Sender инициализирован" << std::endl;
    std::cout.flush();

    int plotSize = m_plotSizeSpin->value();
    std::cout << "startAll: размер буфера = " << plotSize << std::endl;
    std::cout.flush();

    m_rawBuffer.setCapacity(plotSize);
    m_firBuffer.setCapacity(plotSize);
    m_iirBuffer.setCapacity(plotSize);
    m_lastFirValue = 0.0f;
    m_lastIirValue = 0.0f;
    m_firMap.clear();
    m_iirMap.clear();

    std::cout << "startAll: буферы настроены" << std::endl;
    std::cout.flush();

    m_firFilter.start(&m_rawBuffer);
    m_iirFilter.start(&m_rawBuffer);

    m_isRunning = true;
    m_startStopButton->setText("Stop");
    m_statusLabel->setText("Running. Receiving data from model...");

    std::cout << "=== startAll: ЗАВЕРШЕНО ===" << std::endl;
    std::cout.flush();
}

void MainWindow::stopAll()
{
    std::cout << "=== stopAll: НАЧАЛО ===" << std::endl;
    std::cout.flush();

    if (!m_isRunning)
    {
        std::cout << "stopAll: уже остановлен, выходим" << std::endl;
        return;
    }

    m_udpReceiver.stop();
    m_udpSender.closeSocket();
    m_firFilter.stop();
    m_iirFilter.stop();

    m_isRunning = false;
    m_startStopButton->setText("Start");
    m_statusLabel->setText("Stopped.");

    std::cout << "=== stopAll: ЗАВЕРШЕНО ===" << std::endl;
    std::cout.flush();
}

void MainWindow::onStartStop()
{
    std::cout << "onStartStop: нажата кнопка, isRunning = " << m_isRunning << std::endl;
    std::cout.flush();

    if (m_isRunning)
    {
        stopAll();
    }
    else
    {
        startAll();
    }
}

void MainWindow::onReceiveData(const DataPoint &point)
{
    m_rawBuffer.push(point);
}

void MainWindow::onFIRFiltered(const DataPoint &point)
{
    m_firBuffer.push(point);
    m_firMap[point.timestamp] = point.value;
}

void MainWindow::onIIRFiltered(const DataPoint &point)
{
    m_iirBuffer.push(point);
    m_iirMap[point.timestamp] = point.value;
}

void MainWindow::onSendTarget()
{
    std::cout << "onSendTarget: вызов" << std::endl;
    std::cout.flush();

    if (!m_isRunning)
    {
        QMessageBox::warning(this, "Warning", "Please start the system first.");
        std::cout << "onSendTarget: система не запущена" << std::endl;
        return;
    }

    bool ok;
    float target = m_targetEdit->text().toFloat(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, "Error", "Invalid target value. Please enter a number.");
        std::cout << "onSendTarget: неверное значение" << std::endl;
        return;
    }

    std::cout << "onSendTarget: отправка = " << target << std::endl;
    m_udpSender.sendTarget(target);
    m_statusLabel->setText("Sent target: " + QString::number(target));
}

void MainWindow::onUpdatePlot()
{
    if (!m_isRunning)
        return;

    auto raw = m_rawBuffer.getAll();

    if (raw.empty())
        return;

    size_t size = raw.size();

    QVector<double> keys(size);
    QVector<double> rawData(size);
    QVector<double> firData(size);
    QVector<double> iirData(size);

    for (size_t i = 0; i < size; ++i)
    {
        keys[i] = static_cast<double>(i);
        rawData[i] = static_cast<double>(raw[i].value);

        auto itFir = m_firMap.find(raw[i].timestamp);
        if (itFir != m_firMap.end())
        {
            firData[i] = static_cast<double>(itFir->second);
            m_lastFirValue = itFir->second;
        }
        else
        {
            firData[i] = static_cast<double>(m_lastFirValue);
        }

        // Ищем IIR по timestamp
        auto itIir = m_iirMap.find(raw[i].timestamp);
        if (itIir != m_iirMap.end())
        {
            iirData[i] = static_cast<double>(itIir->second);
            m_lastIirValue = itIir->second;
        }
        else
        {
            iirData[i] = static_cast<double>(m_lastIirValue);
        }
    }

    static int counter = 0;
    if (++counter % 50 == 0)
    {
        std::cout << "Sync: raw=" << raw.size()
                  << ", firMap=" << m_firMap.size()
                  << ", iirMap=" << m_iirMap.size()
                  << ", size=" << size
                  << ", lastFir=" << m_lastFirValue
                  << ", lastIir=" << m_lastIirValue << std::endl;
    }

    m_plotWidget->updateData(keys, rawData, firData, iirData);
}