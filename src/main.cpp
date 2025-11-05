#include <QApplication>
#include "ParserConfigDialog.h"
#include "CSVReader.h"
#include "PlotConfigDialog.h"
#include "MainWindow.h"
#include <stdexcept>
#include <QDebug>

int main(int argc, char *argv[])
{
    try {
        QApplication app(argc, argv);

        // Set application icon from bundled SVG resource
        app.setWindowIcon(QIcon(":/icons/icons/logo.svg"));

        MainWindow *w = new MainWindow();
        w->show();
        return app.exec();
    } catch (const std::exception& e) {
        qCritical() << "Fatal exception in main():" << e.what();
        return -1;
    } catch (...) {
        qCritical() << "Unknown exception in main()";
        return -1;
    }
}
