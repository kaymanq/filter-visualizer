cmake_minimum_required(VERSION 3.16)
project(FilterVisualizer)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


add_definitions(-D_GLIBCXX_INCLUDE_NEXT_C_HEADERS)


find_package(Qt5 REQUIRED COMPONENTS Core Widgets PrintSupport)

message(STATUS "Using Qt5 version: ${Qt5_VERSION}")


set(QCUSTOMPLOT_ROOT "${CMAKE_SOURCE_DIR}/external/qcustomplot")

# Проверяем наличие файлов
if(EXISTS "${QCUSTOMPLOT_ROOT}/qcustomplot.h" AND 
   EXISTS "${QCUSTOMPLOT_ROOT}/qcustomplot.cpp")
    
    message(STATUS "QCustomPlot found at: ${QCUSTOMPLOT_ROOT}")
    set(QCUSTOMPLOT_INCLUDE_DIR ${QCUSTOMPLOT_ROOT})
    set(QCUSTOMPLOT_SRC_DIR ${QCUSTOMPLOT_ROOT})
    set(QCUSTOMPLOT_FOUND TRUE)
    
elseif(EXISTS "${QCUSTOMPLOT_ROOT}/src/qcustomplot.h" AND 
       EXISTS "${QCUSTOMPLOT_ROOT}/src/qcustomplot.cpp")
    
    message(STATUS "QCustomPlot found at: ${QCUSTOMPLOT_ROOT}/src")
    set(QCUSTOMPLOT_INCLUDE_DIR ${QCUSTOMPLOT_ROOT}/src)
    set(QCUSTOMPLOT_SRC_DIR ${QCUSTOMPLOT_ROOT}/src)
    set(QCUSTOMPLOT_FOUND TRUE)
    
elseif(EXISTS "${QCUSTOMPLOT_ROOT}/include/qcustomplot.h" AND 
       EXISTS "${QCUSTOMPLOT_ROOT}/src/qcustomplot.cpp")
    
    message(STATUS "QCustomPlot found with include/ and src/")
    set(QCUSTOMPLOT_INCLUDE_DIR ${QCUSTOMPLOT_ROOT}/include)
    set(QCUSTOMPLOT_SRC_DIR ${QCUSTOMPLOT_ROOT}/src)
    set(QCUSTOMPLOT_FOUND TRUE)
    
else()
    message(FATAL_ERROR 
        "QCustomPlot not found!\n"
        "Please download from: https://www.qcustomplot.com/index.php/download\n"
        "Or run:\n"
        "  mkdir -p external\n"
        "  cd external\n"
        "  wget https://www.qcustomplot.com/release/2.1.0/QCustomPlot.tar.gz\n"
        "  tar -xzf QCustomPlot.tar.gz\n"
        "  mv qcustomplot-2.1.0 qcustomplot\n"
        "  cd ..\n"
    )
endif()


if(QCUSTOMPLOT_FOUND)
    
    message(STATUS "QCustomPlot include: ${QCUSTOMPLOT_INCLUDE_DIR}")
    message(STATUS "QCustomPlot sources: ${QCUSTOMPLOT_SRC_DIR}")
    
    add_library(qcustomplot STATIC
        ${QCUSTOMPLOT_SRC_DIR}/qcustomplot.cpp
    )
    
    target_include_directories(qcustomplot SYSTEM PUBLIC
        ${QCUSTOMPLOT_INCLUDE_DIR}
    )
    
    target_link_libraries(qcustomplot
        Qt5::Core
        Qt5::Widgets
        Qt5::PrintSupport
    )
    
    target_compile_definitions(qcustomplot PRIVATE
        QCUSTOMPLOT_COMPILE_LIBRARY
    )
    
    target_compile_definitions(qcustomplot PRIVATE
        _USE_MATH_DEFINES
        _GLIBCXX_INCLUDE_NEXT_C_HEADERS
    )
    
    target_compile_features(qcustomplot PUBLIC cxx_std_17)
    
    message(STATUS "QCustomPlot library configured successfully!")
endif()

# ==========================================
# ИСХОДНЫЕ ФАЙЛЫ ПРОЕКТА
# ==========================================
set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/UDPReceiver.cpp
    src/UDPSender.cpp
    src/DataBuffer.cpp
    src/FIRFilter.cpp
    src/IIRFilter.cpp
    src/PlotWidget.cpp
)

set(HEADERS
    src/MainWindow.h
    src/UDPReceiver.h
    src/UDPSender.h
    src/DataBuffer.h
    src/FIRFilter.h
    src/IIRFilter.h
    src/PlotWidget.h
)


add_executable(filter_visualizer ${SOURCES} ${HEADERS})


target_link_libraries(filter_visualizer
    Qt5::Widgets
    Qt5::PrintSupport
    qcustomplot
    pthread
)


target_include_directories(filter_visualizer SYSTEM PRIVATE
    ${QCUSTOMPLOT_INCLUDE_DIR}
)

target_include_directories(filter_visualizer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)


target_compile_definitions(filter_visualizer PRIVATE
    _USE_MATH_DEFINES
    _GLIBCXX_INCLUDE_NEXT_C_HEADERS
)

target_compile_features(filter_visualizer PUBLIC cxx_std_17)


if(UNIX AND NOT APPLE)
    target_link_libraries(filter_visualizer
        ${CMAKE_DL_LIBS}
        m  
    )
endif()


if(WIN32)
    target_link_libraries(filter_visualizer ws2_32)
endif()


option(USE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)

if(USE_SANITIZERS AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(filter_visualizer PRIVATE -fsanitize=address,undefined -g -O0)
    target_link_options(filter_visualizer PRIVATE -fsanitize=address,undefined)
    message(STATUS "Sanitizers enabled: address,undefined")
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

message(STATUS "")
message(STATUS "========================================")
message(STATUS "=== BUILD CONFIGURATION ===")
message(STATUS "Qt version: ${Qt5_VERSION}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "QCustomPlot include: ${QCUSTOMPLOT_INCLUDE_DIR}")
message(STATUS "QCustomPlot sources: ${QCUSTOMPLOT_SRC_DIR}")
message(STATUS "Sanitizers: ${USE_SANITIZERS}")
message(STATUS "========================================")
message(STATUS "")