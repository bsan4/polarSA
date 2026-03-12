/********************************************************************************
** Form generated from reading UI file 'mainwindow.ui'
**
** Created by: Qt User Interface Compiler version 6.10.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <src/SpectrumPlot/spectrumplot.h>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QTabWidget *tabSpectrum;
    QWidget *tabSpectrum1;
    QGridLayout *gridLayout_2;
    SpectrumPlot *spectrumPlot;
    QWidget *tab_2;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(800, 600);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        tabSpectrum = new QTabWidget(centralwidget);
        tabSpectrum->setObjectName("tabSpectrum");
        tabSpectrum1 = new QWidget();
        tabSpectrum1->setObjectName("tabSpectrum1");
        gridLayout_2 = new QGridLayout(tabSpectrum1);
        gridLayout_2->setObjectName("gridLayout_2");
        spectrumPlot = new SpectrumPlot(tabSpectrum1);
        spectrumPlot->setObjectName("spectrumPlot");

        gridLayout_2->addWidget(spectrumPlot, 0, 0, 1, 1);

        tabSpectrum->addTab(tabSpectrum1, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName("tab_2");
        tabSpectrum->addTab(tab_2, QString());

        gridLayout->addWidget(tabSpectrum, 0, 0, 1, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 800, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        tabSpectrum->setTabText(tabSpectrum->indexOf(tabSpectrum1), QCoreApplication::translate("MainWindow", "Spectrum", nullptr));
        tabSpectrum->setTabText(tabSpectrum->indexOf(tab_2), QCoreApplication::translate("MainWindow", "Tab 2", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
