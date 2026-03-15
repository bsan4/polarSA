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
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>
#include <src/SpectrumPlot/spectrumplot.h>
#include "src/PowerPlot/powerplot.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QTabWidget *tabWidget;
    QWidget *tabSpectrum;
    QGridLayout *gridLayout_2;
    SpectrumPlot *spectrumPlot;
    QWidget *tabRadiationPattern;
    QGridLayout *gridLayout_3;
    PowerPlot *powerPlot;
    QGroupBox *groupBox;
    QGridLayout *gridLayout_6;
    QCheckBox *sbSALNA;
    QDoubleSpinBox *sbSARBW;
    QLabel *lbSAStatus;
    QLabel *label_10;
    QLabel *label_8;
    QLabel *label_7;
    QLabel *label_11;
    QDoubleSpinBox *sbSAAtt;
    QDoubleSpinBox *sbSASpan;
    QLabel *label_13;
    QSpacerItem *verticalSpacer_2;
    QDoubleSpinBox *sbSACenterFreq;
    QLabel *label_9;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_5;
    QLabel *label_3;
    QSpinBox *sbRotatorStep;
    QLabel *lbRotatorStatus;
    QLabel *label;
    QSpacerItem *verticalSpacer_3;
    QSpinBox *sbRotatorStart;
    QSpinBox *sbRotatorEnd;
    QLabel *label_2;
    QLabel *label_4;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_4;
    QSpacerItem *verticalSpacer;
    QSpacerItem *verticalSpacer_4;
    QPushButton *pbSequenceCapture;
    QPushButton *pbSingleCapture;
    QPushButton *pushButton_3;
    QPushButton *pushButton_4;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1190, 720);
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName("centralwidget");
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName("gridLayout");
        tabWidget = new QTabWidget(centralwidget);
        tabWidget->setObjectName("tabWidget");
        tabSpectrum = new QWidget();
        tabSpectrum->setObjectName("tabSpectrum");
        gridLayout_2 = new QGridLayout(tabSpectrum);
        gridLayout_2->setObjectName("gridLayout_2");
        spectrumPlot = new SpectrumPlot(tabSpectrum);
        spectrumPlot->setObjectName("spectrumPlot");

        gridLayout_2->addWidget(spectrumPlot, 0, 0, 1, 1);

        tabWidget->addTab(tabSpectrum, QString());
        tabRadiationPattern = new QWidget();
        tabRadiationPattern->setObjectName("tabRadiationPattern");
        gridLayout_3 = new QGridLayout(tabRadiationPattern);
        gridLayout_3->setObjectName("gridLayout_3");
        powerPlot = new PowerPlot(tabRadiationPattern);
        powerPlot->setObjectName("powerPlot");

        gridLayout_3->addWidget(powerPlot, 0, 0, 1, 1);

        tabWidget->addTab(tabRadiationPattern, QString());

        gridLayout->addWidget(tabWidget, 1, 1, 2, 1);

        groupBox = new QGroupBox(centralwidget);
        groupBox->setObjectName("groupBox");
        gridLayout_6 = new QGridLayout(groupBox);
        gridLayout_6->setObjectName("gridLayout_6");
        sbSALNA = new QCheckBox(groupBox);
        sbSALNA->setObjectName("sbSALNA");

        gridLayout_6->addWidget(sbSALNA, 5, 1, 1, 1);

        sbSARBW = new QDoubleSpinBox(groupBox);
        sbSARBW->setObjectName("sbSARBW");
        sbSARBW->setMaximum(10000.000000000000000);
        sbSARBW->setValue(10.000000000000000);

        gridLayout_6->addWidget(sbSARBW, 3, 1, 1, 1);

        lbSAStatus = new QLabel(groupBox);
        lbSAStatus->setObjectName("lbSAStatus");

        gridLayout_6->addWidget(lbSAStatus, 0, 1, 1, 1);

        label_10 = new QLabel(groupBox);
        label_10->setObjectName("label_10");

        gridLayout_6->addWidget(label_10, 4, 0, 1, 1);

        label_8 = new QLabel(groupBox);
        label_8->setObjectName("label_8");

        gridLayout_6->addWidget(label_8, 0, 0, 1, 1);

        label_7 = new QLabel(groupBox);
        label_7->setObjectName("label_7");

        gridLayout_6->addWidget(label_7, 1, 0, 1, 1);

        label_11 = new QLabel(groupBox);
        label_11->setObjectName("label_11");

        gridLayout_6->addWidget(label_11, 5, 0, 1, 1);

        sbSAAtt = new QDoubleSpinBox(groupBox);
        sbSAAtt->setObjectName("sbSAAtt");

        gridLayout_6->addWidget(sbSAAtt, 4, 1, 1, 1);

        sbSASpan = new QDoubleSpinBox(groupBox);
        sbSASpan->setObjectName("sbSASpan");
        sbSASpan->setDecimals(4);
        sbSASpan->setMaximum(10000.000000000000000);
        sbSASpan->setValue(10.000000000000000);

        gridLayout_6->addWidget(sbSASpan, 2, 1, 1, 1);

        label_13 = new QLabel(groupBox);
        label_13->setObjectName("label_13");

        gridLayout_6->addWidget(label_13, 3, 0, 1, 1);

        verticalSpacer_2 = new QSpacerItem(250, 19, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_6->addItem(verticalSpacer_2, 6, 0, 1, 2);

        sbSACenterFreq = new QDoubleSpinBox(groupBox);
        sbSACenterFreq->setObjectName("sbSACenterFreq");
        sbSACenterFreq->setDecimals(4);
        sbSACenterFreq->setMinimum(0.000000000000000);
        sbSACenterFreq->setMaximum(10000.000000000000000);
        sbSACenterFreq->setValue(100.000000000000000);

        gridLayout_6->addWidget(sbSACenterFreq, 1, 1, 1, 1);

        label_9 = new QLabel(groupBox);
        label_9->setObjectName("label_9");

        gridLayout_6->addWidget(label_9, 2, 0, 1, 1);


        gridLayout->addWidget(groupBox, 1, 0, 1, 1);

        groupBox_2 = new QGroupBox(centralwidget);
        groupBox_2->setObjectName("groupBox_2");
        gridLayout_5 = new QGridLayout(groupBox_2);
        gridLayout_5->setObjectName("gridLayout_5");
        label_3 = new QLabel(groupBox_2);
        label_3->setObjectName("label_3");

        gridLayout_5->addWidget(label_3, 3, 0, 1, 1);

        sbRotatorStep = new QSpinBox(groupBox_2);
        sbRotatorStep->setObjectName("sbRotatorStep");
        sbRotatorStep->setMaximum(360);
        sbRotatorStep->setValue(10);

        gridLayout_5->addWidget(sbRotatorStep, 3, 1, 1, 1);

        lbRotatorStatus = new QLabel(groupBox_2);
        lbRotatorStatus->setObjectName("lbRotatorStatus");

        gridLayout_5->addWidget(lbRotatorStatus, 0, 1, 1, 1);

        label = new QLabel(groupBox_2);
        label->setObjectName("label");

        gridLayout_5->addWidget(label, 1, 0, 1, 1);

        verticalSpacer_3 = new QSpacerItem(250, 109, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_5->addItem(verticalSpacer_3, 4, 0, 1, 2);

        sbRotatorStart = new QSpinBox(groupBox_2);
        sbRotatorStart->setObjectName("sbRotatorStart");
        sbRotatorStart->setMaximum(360);

        gridLayout_5->addWidget(sbRotatorStart, 1, 1, 1, 1);

        sbRotatorEnd = new QSpinBox(groupBox_2);
        sbRotatorEnd->setObjectName("sbRotatorEnd");
        sbRotatorEnd->setMaximum(360);
        sbRotatorEnd->setValue(180);

        gridLayout_5->addWidget(sbRotatorEnd, 2, 1, 1, 1);

        label_2 = new QLabel(groupBox_2);
        label_2->setObjectName("label_2");

        gridLayout_5->addWidget(label_2, 2, 0, 1, 1);

        label_4 = new QLabel(groupBox_2);
        label_4->setObjectName("label_4");

        gridLayout_5->addWidget(label_4, 0, 0, 1, 1);


        gridLayout->addWidget(groupBox_2, 2, 0, 1, 1);

        groupBox_3 = new QGroupBox(centralwidget);
        groupBox_3->setObjectName("groupBox_3");
        gridLayout_4 = new QGridLayout(groupBox_3);
        gridLayout_4->setObjectName("gridLayout_4");
        verticalSpacer = new QSpacerItem(20, 403, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding);

        gridLayout_4->addItem(verticalSpacer, 5, 0, 1, 1);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        gridLayout_4->addItem(verticalSpacer_4, 2, 0, 1, 1);

        pbSequenceCapture = new QPushButton(groupBox_3);
        pbSequenceCapture->setObjectName("pbSequenceCapture");

        gridLayout_4->addWidget(pbSequenceCapture, 3, 0, 1, 1);

        pbSingleCapture = new QPushButton(groupBox_3);
        pbSingleCapture->setObjectName("pbSingleCapture");

        gridLayout_4->addWidget(pbSingleCapture, 0, 0, 1, 1);

        pushButton_3 = new QPushButton(groupBox_3);
        pushButton_3->setObjectName("pushButton_3");
        pushButton_3->setEnabled(false);

        gridLayout_4->addWidget(pushButton_3, 1, 0, 1, 1);

        pushButton_4 = new QPushButton(groupBox_3);
        pushButton_4->setObjectName("pushButton_4");
        pushButton_4->setEnabled(false);

        gridLayout_4->addWidget(pushButton_4, 4, 0, 1, 1);


        gridLayout->addWidget(groupBox_3, 1, 2, 2, 1);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName("menubar");
        menubar->setGeometry(QRect(0, 0, 1190, 25));
        MainWindow->setMenuBar(menubar);
        statusbar = new QStatusBar(MainWindow);
        statusbar->setObjectName("statusbar");
        MainWindow->setStatusBar(statusbar);

        retranslateUi(MainWindow);

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "MainWindow", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabSpectrum), QCoreApplication::translate("MainWindow", "Spectrum", nullptr));
        tabWidget->setTabText(tabWidget->indexOf(tabRadiationPattern), QCoreApplication::translate("MainWindow", "Radiation Pattern", nullptr));
        groupBox->setTitle(QCoreApplication::translate("MainWindow", "Spectrum Analyzer", nullptr));
        sbSALNA->setText(QString());
        lbSAStatus->setText(QCoreApplication::translate("MainWindow", "Disconnected", nullptr));
        label_10->setText(QCoreApplication::translate("MainWindow", "Attenuation (dB)", nullptr));
        label_8->setText(QCoreApplication::translate("MainWindow", "Status", nullptr));
        label_7->setText(QCoreApplication::translate("MainWindow", "Center Freq (MHz)", nullptr));
        label_11->setText(QCoreApplication::translate("MainWindow", "LNA", nullptr));
        label_13->setText(QCoreApplication::translate("MainWindow", "RBW (kHz)", nullptr));
        label_9->setText(QCoreApplication::translate("MainWindow", "Span (MHz)", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("MainWindow", "Rotator", nullptr));
        label_3->setText(QCoreApplication::translate("MainWindow", "Angular step", nullptr));
        lbRotatorStatus->setText(QCoreApplication::translate("MainWindow", "Disconnected", nullptr));
        label->setText(QCoreApplication::translate("MainWindow", "Start angle", nullptr));
        label_2->setText(QCoreApplication::translate("MainWindow", "End angle", nullptr));
        label_4->setText(QCoreApplication::translate("MainWindow", "Status", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("MainWindow", "Capture", nullptr));
        pbSequenceCapture->setText(QCoreApplication::translate("MainWindow", "Capture Sequence", nullptr));
        pbSingleCapture->setText(QCoreApplication::translate("MainWindow", "Capture Single", nullptr));
        pushButton_3->setText(QCoreApplication::translate("MainWindow", "Loop Single Capture", nullptr));
        pushButton_4->setText(QCoreApplication::translate("MainWindow", "Loop Sequence", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
