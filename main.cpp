#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("Quantor Innovation");
    QCoreApplication::setApplicationName("polarSA");
    MainWindow w;
    w.show();
    return a.exec();
}
