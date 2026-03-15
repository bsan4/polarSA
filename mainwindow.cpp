#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("polarSA - Beta 1.0.0");

    connect( ui->pbSingleCapture, &QPushButton::pressed, this, &MainWindow::triggerSingleCapture );
    connect( ui->pbSequenceCapture, &QPushButton::pressed, this, &MainWindow::triggerSequenceCapture );

    connect( &rotator, &Rotator::connected, this, [this](){ ui->lbRotatorStatus->setText("Connected"); });
    connect( &rotator, &Rotator::disconnected, this, [this](){ ui->lbRotatorStatus->setText("Disconnected"); });

    connect( &tinySA, &TinySA::connected, this, [this](){ ui->lbSAStatus->setText("Connected"); });
    connect( &tinySA, &TinySA::disconnected, this, [this](){ ui->lbSAStatus->setText("Disconnected"); });

    ui->powerPlot->setNormalize(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::triggerSingleCapture()
{
    if ( !tinySA.isOpen() || !rotator.isOpen() )
    {
        return;
    }

    sendSAParameters();
    sendRotatorParameters();

    ui->statusbar->showMessage("Capturing...");

    bool success = false;

    PowerSpectrum ps = tinySA.capture();
    while ( ps.power.isEmpty())
    {
        ps = tinySA.capture();
    }
    ui->spectrumPlot->setPowerSpectrum(ps);
    ui->statusbar->showMessage("Done.");
}

void MainWindow::triggerSequenceCapture()
{
    if ( !tinySA.isOpen() || !rotator.isOpen() )
    {
        return;
    }

    sendSAParameters();
    sendRotatorParameters();

    RadiationPattern rp;
    int startAngle = ui->sbRotatorStart->value();
    int endAngle = ui->sbRotatorEnd->value();
    int angleStep = ui->sbRotatorStep->value();

    int a = startAngle;
    while ( a <= endAngle )
    {
        int nbTotalCaptures = (endAngle - startAngle)/angleStep + 1;
        int currentCapture = (a - startAngle)/angleStep + 1;
        ui->statusbar->showMessage("Capturing " + QString::number(currentCapture) + " out of " + QString::number(nbTotalCaptures));

        qDebug() << "capturing angle " << a;
        rotator.setAngle(a);
        Sleep(100);
        PowerSpectrum ps = tinySA.capture();
        if (ps.power.length() > 0)
        {
            rp.angles.push_back(static_cast<float>(a));
            rp.power.push_back( ps.power.at(0) );
            a += angleStep;
            ui->spectrumPlot->setPowerSpectrum(ps);
            ui->powerPlot->setRadiationPattern(rp);
        }
        QApplication::processEvents();
    }
}

void MainWindow::sendSAParameters()
{
    TinySA::Setup s;
    s.mode = "low";
    s.attenuationDb = ui->sbSAAtt->value();      // or -1 for auto
    s.rbwKHz = ui->sbSARBW->value();            // or -1 for auto
    s.repeatCount = 1;
    s.spurReduction = false;
    s.extGainDb = 0.0f;
    s.centerHz = ui->sbSACenterFreq->value() * 1e6;
    s.spanHz = ui->sbSASpan->value() * 1e6;
    s.points = 290;
    s.rawOffsetDb = 174.0f;
    tinySA.setSetup(s);
    tinySA.setDefaults();
}

void MainWindow::sendRotatorParameters()
{
    int startAngle = ui->sbRotatorStart->value();
    int endAngle = ui->sbRotatorEnd->value();
    int angleStep = ui->sbRotatorStep->value();
    if ( endAngle < startAngle )
    {
        endAngle = startAngle;
        ui->sbRotatorEnd->setValue(endAngle);
    }
    if ( angleStep < 1 )
    {
        angleStep = 1;
        ui->sbRotatorStep->setValue(angleStep);
    }

    rotator.setAngle(ui->sbRotatorStart->value());
}
