# Filter Visualizer

Приложение для фильтрации и визуализации данных в реальном времени.

## 📊 Описание

Приложение принимает данные по UDP от Python-модели, применяет различные фильтры (FIR и IIR) и отображает результаты на графике.

## 🚀 Возможности

- **Приём данных по UDP** от модели
- **Отправка целевых значений** в модель
- **6 алгоритмов FIR-фильтров**:
  - Boxcar (Moving Average)
  - Hamming
  - Blackman
  - Median (Non-linear)
  - Gaussian
  - Low-Pass (Sinc)
- **4 алгоритма IIR-фильтров**:
  - Exponential Smoothing
  - Butterworth 2nd Order
- **Визуализация в реальном времени** с помощью QCustomPlot
- **Многопоточная обработка** данных

## 🛠️ Технологии

- **C++17**
- **Qt 5.12.8** (Core, Widgets, PrintSupport)
- **QCustomPlot** (графики)
- **CMake** (сборка)
- **POSIX sockets** (UDP)

## 📁 Структура проекта
FilterVizualizer/
├── src/
│ ├── main.cpp
│ ├── MainWindow.h/cpp
│ ├── UDPReceiver.h/cpp
│ ├── UDPSender.h/cpp
│ ├── DataBuffer.h/cpp
│ ├── FIRFilter.h/cpp
│ ├── IIRFilter.h/cpp
│ └── PlotWidget.h/cpp
├── external/
│ └── qcustomplot/
├── CMakeLists.txt
├── .gitignore
└── README.md

## 🔧 Сборка

```bash
# Клонирование репозитория
git clone https://github.com/ваш-username/FilterVizualizer.git
cd FilterVizualizer

# Скачивание QCustomPlot
mkdir -p external
cd external
wget https://www.qcustomplot.com/release/2.1.0/QCustomPlot.tar.gz
tar -xzf QCustomPlot.tar.gz
mv qcustomplot-2.1.0 qcustomplot
cd ..

# Сборка
mkdir build
cd build
cmake ..
make -j4
./filter_visualizer
