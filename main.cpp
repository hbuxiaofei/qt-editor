#include <QApplication>

#include "mainwindow.h"
#include "config.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    static QSharedMemory *shareMem = new QSharedMemory("Q-Text-Editor");
    if (!shareMem->create(1)) {
        qApp->quit();
        return -1;
    }

    Config config;
    if (argc > 1 && QFile::exists(argv[1])) {
        QString inputFile = QFileInfo(argv[1]).filePath();
        if (!config.recentFiles.contains(inputFile)) {
            config.recentFiles.append(inputFile);
        }
    }
    MainWindow mainWin(&config);
    mainWin.show();
    return app.exec();
}
