#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    TinySA::Setup s;
    s.mode = "low";
    s.attenuationDb = 0;      // or -1 for auto
    s.rbwKHz = 30;            // or -1 for auto
    s.repeatCount = 1;
    s.spurReduction = false;
    s.extGainDb = 0.0f;
    s.centerHz = 433920000;
    s.spanHz = 2000000;
    s.points = 290;
    s.rawOffsetDb = 174.0f;

    tinySA.setSetup(s);
    tinySA.setDefaults();

    PowerSpectrum ps = tinySA.capture();
    qDebug() << ps.power;
    qDebug() << ps.freq;
    ui->spectrumPlot->setPowerSpectrum(ps);
}

MainWindow::~MainWindow()
{
    delete ui;
}
