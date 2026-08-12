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
            { m_firFilter.setWindowSize(v); });
    connect(m_firCutoffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v)
            { m_firFilter.setCutoffFrequency(v); });

    tabs->addTab(firTab, "FIR Filters");

    QWidget *iirTab = new QWidget();
    QFormLayout *iirLayout = new QFormLayout(iirTab);

    m_iirAlgorithmCombo = new QComboBox();
    m_iirAlgorithmCombo->addItem("Exponential Smoothing", 0);
    m_iirAlgorithmCombo->addItem("Butterworth 2nd Order", 1);
    m_iirAlgorithmCombo->addItem("Chebyshev 2nd Order", 2);
    m_iirAlgorithmCombo->addItem("Bessel 2nd Order", 3);
    iirLayout->addRow("IIR Algorithm:", m_iirAlgorithmCombo);

    m_alphaSpin = new QDoubleSpinBox();
    m_alphaSpin->setRange(0.01, 1.0);
    m_alphaSpin->setSingleStep(0.05);
    m_alphaSpin->setValue(0.3);
    iirLayout->addRow("Alpha (0-1):", m_alphaSpin);

    m_iirOrderSpin = new QSpinBox();
    m_iirOrderSpin->setRange(1, 4);
    m_iirOrderSpin->setValue(2);
    m_iirOrderSpin->setEnabled(false);
    iirLayout->addRow("Filter Order:", m_iirOrderSpin);

    m_iirCutoffSpin = new QDoubleSpinBox();
    m_iirCutoffSpin->setRange(0.01, 0.99);
    m_iirCutoffSpin->setSingleStep(0.05);
    m_iirCutoffSpin->setValue(0.3);
    m_iirCutoffSpin->setEnabled(false);
    iirLayout->addRow("Cutoff Frequency (0-1):", m_iirCutoffSpin);

    connect(m_iirAlgorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onAlgorithmChanged);
    connect(m_alphaSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v)
            { m_iirFilter.setAlpha(static_cast<float>(v)); });
    connect(m_iirOrderSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            [this](int v)
            { m_iirFilter.setOrder(v); });
    connect(m_iirCutoffSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            [this](double v)
            { m_iirFilter.setCutoffFrequency(static_cast<float>(v)); });

    tabs->addTab(iirTab, "IIR Filters");

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
    // FIR
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

    // IIR
    int iirIdx = m_iirAlgorithmCombo->currentIndex();
    if (iirIdx >= 0)
    {
        IIRFilter::Algorithm algo;
        switch (iirIdx)
        {
        case 0:
            algo = IIRFilter::Algorithm::Exponential;
            break;
        case 1:
            algo = IIRFilter::Algorithm::Butterworth;
            break;
        case 2:
            algo = IIRFilter::Algorithm::Chebyshev;
            break;
        case 3:
            algo = IIRFilter::Algorithm::Bessel;
            break;
        default:
            algo = IIRFilter::Algorithm::Exponential;
            break;
        }
        m_iirFilter.setAlgorithm(algo);
        bool isExp = (algo == IIRFilter::Algorithm::Exponential);
        bool isOther = (algo != IIRFilter::Algorithm::Exponential);
        m_alphaSpin->setEnabled(isExp);
        m_iirOrderSpin->setEnabled(isOther);
        m_iirCutoffSpin->setEnabled(isOther);
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

    std::cout << "startAll: запускаем UDP Receiver..." << std::endl;
    std::cout.flush();

    if (!m_udpReceiver.start(receiveAddr.toStdString(), receivePort))
    {
        QMessageBox::critical(this, "Error", "Failed to start UDP receiver");
        std::cout << "startAll: UDP Receiver НЕ запущен!" << std::endl;
        return;
    }
    std::cout << "startAll: UDP Receiver запущен" << std::endl;
    std::cout.flush();

    std::cout << "startAll: инициализируем UDP Sender..." << std::endl;
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

    std::cout << "startAll: буферы настроены" << std::endl;
    std::cout.flush();

    std::cout << "startAll: ПРОВЕРКА - вызываем FIR фильтр..." << std::endl;
    std::cout.flush();

    std::cout << "startAll: запускаем FIR фильтр..." << std::endl;
    std::cout.flush();

    m_firFilter.start(&m_rawBuffer);

    std::cout << "startAll: FIR фильтр запущен (возврат из start)" << std::endl;
    std::cout.flush();

    std::cout << "startAll: ПРОВЕРКА - вызываем IIR фильтр..." << std::endl;
    std::cout.flush();

    std::cout << "startAll: запускаем IIR фильтр..." << std::endl;
    std::cout.flush();

    m_iirFilter.start(&m_rawBuffer);

    std::cout << "startAll: IIR фильтр запущен (возврат из start)" << std::endl;
    std::cout.flush();

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

    std::cout << "stopAll: останавливаем UDP Receiver..." << std::endl;
    m_udpReceiver.stop();
    std::cout << "stopAll: UDP Receiver остановлен" << std::endl;

    std::cout << "stopAll: закрываем UDP Sender..." << std::endl;
    m_udpSender.close();
    std::cout << "stopAll: UDP Sender закрыт" << std::endl;

    std::cout << "stopAll: останавливаем FIR фильтр..." << std::endl;
    m_firFilter.stop();
    std::cout << "stopAll: FIR фильтр остановлен" << std::endl;

    std::cout << "stopAll: останавливаем IIR фильтр..." << std::endl;
    m_iirFilter.stop();
    std::cout << "stopAll: IIR фильтр остановлен" << std::endl;

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
}

void MainWindow::onIIRFiltered(const DataPoint &point)
{
    m_iirBuffer.push(point);
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
    auto fir = m_firBuffer.getAll();
    auto iir = m_iirBuffer.getAll();

    if (raw.empty())
        return;

    std::vector<float> rawVals, firVals, iirVals;
    rawVals.reserve(raw.size());
    firVals.reserve(fir.size());
    iirVals.reserve(iir.size());

    for (const auto &p : raw)
        rawVals.push_back(p.value);
    for (const auto &p : fir)
        firVals.push_back(p.value);
    for (const auto &p : iir)
        iirVals.push_back(p.value);

    m_plotWidget->updateData(rawVals, firVals, iirVals);
}