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
  - Chebyshev 2nd Order
  - Bessel 2nd Order
- **Визуализация в реальном времени** с помощью QCustomPlot
- **Многопоточная обработка** данных

## 🛠️ Технологии

- **C++17**
- **Qt 5.12.8** (Core, Widgets, PrintSupport)
- **QCustomPlot** (графики)
- **CMake** (сборка)
- **POSIX sockets** (UDP)

## 📁 Структура проекта
