#include <QApplication>
#include "MainWindow.h"
#include "DataBuffer.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<DataPoint>("DataPoint");

    app.setApplicationName("Filter Visualizer");
    app.setOrganizationName("MyCompany");

    MainWindow window;
    window.show();

    return app.exec();
}