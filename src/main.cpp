#include <QApplication>
#include <QCommandLineParser>
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

        // Set application properties
        app.setApplicationName("RTPlotter");
        app.setApplicationVersion("1.0");
        app.setOrganizationName("RTPlotter");

        // Set application icon from bundled SVG resource
        app.setWindowIcon(QIcon(":/icons/icons/logo.svg"));

        // Command line parser
        QCommandLineParser parser;
        parser.setApplicationDescription("Real-time plotter for CSV data");
        parser.addHelpOption();
        parser.addVersionOption();

        // Add project file option
        QCommandLineOption projectOption(QStringList() << "p" << "project",
                                        QCoreApplication::translate("main", "Load project file <file>"),
                                        QCoreApplication::translate("main", "file"));
        parser.addOption(projectOption);

        // Process the actual command line arguments given by the user
        parser.process(app);

        MainWindow *w = new MainWindow();
        
        // If project file is specified, load it
        if (parser.isSet(projectOption)) {
            QString projectFile = parser.value(projectOption);
            if (!w->loadProjectFromPath(projectFile)) {
                qCritical() << "Failed to load project file:" << projectFile;
                return -1;
            }
            w->addRecentProject(projectFile);
        }

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
