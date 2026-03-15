#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <windows.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("polarSA - Beta 1.0.0");

    // Setup rbw combobox
    for( QPair<QString, float> elt : rbwMap)
    {
        ui->cbRBW->addItem(elt.first);
    }

    connect( ui->pbSingleCapture, &QPushButton::pressed, this, &MainWindow::triggerSingleCapture );
    connect( ui->pbSequenceCapture, &QPushButton::pressed, this, &MainWindow::triggerSequenceCapture );
    connect( ui->pbStopCapture, &QPushButton::pressed, this, &MainWindow::triggerStopCapture );
    connect( ui->hsFreq, &QSlider::valueChanged, this, &MainWindow::freqSliderMoved);

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
    stopCapture = false;
    if ( !tinySA.isOpen() || !rotator.isOpen() )
    {
        return;
    }

    sendSAParameters();
    sendRotatorParameters();

    ui->statusbar->showMessage("Capturing...");

    bool success = false;

    PowerSpectrum ps = tinySA.capture();
    while ( ps.power.isEmpty() && !stopCapture )
    {
        ps = tinySA.capture();
        QApplication::processEvents();
    }
    if ( !ps.freq.isEmpty() )
    {
        ui->spectrumPlot->setPowerSpectrum(ps);
    }
    ui->statusbar->showMessage("Done.");
}

void MainWindow::triggerSequenceCapture()
{
    stopCapture = false;
    if ( !tinySA.isOpen() || !rotator.isOpen() )
    {
        return;
    }

    sendSAParameters();
    sendRotatorParameters();

    RadiationPattern rp;
    PowerSpectrum maxPs;
    powerSpectra.clear();
    int startAngle = ui->sbRotatorStart->value();
    int endAngle = ui->sbRotatorEnd->value();
    int angleStep = ui->sbRotatorStep->value();

    int a = startAngle;
    while ( (a <= endAngle) && !stopCapture  )
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
            powerSpectra.append(ps);
            updateRadiationPattern();
            if ( maxPs.power.isEmpty() ) maxPs = ps;
            for( int i = 0; i < ps.power.count(); i++ )
            {
                if ( maxPs.power.at(i) < ps.power.at(i) )
                {
                    maxPs.power[i] = ps.power.at(i);
                }
            }
            ui->maxSpectrumPlot->setPowerSpectrum(maxPs);
        }
        QApplication::processEvents();
    }
    ui->statusbar->showMessage("Done.");
}

void MainWindow::triggerStopCapture()
{
    stopCapture = true;
}

void MainWindow::freqSliderMoved(int idChanged)
{
    updateRadiationPattern();
}

void MainWindow::sendSAParameters()
{
    float rbwkHz = -1;
    for( QPair<QString, float> elt : rbwMap)
    {
        if ( elt.first == ui->cbRBW->currentText() )
        {
            rbwkHz = elt.second;
        }
    }

    TinySA::Setup s;
    s.mode = "low";
    s.attenuationDb = ui->sbSAAtt->value();      // or -1 for auto
    s.rbwKHz = rbwkHz;            // or -1 for auto
    s.repeatCount = 1;
    s.spurReduction = false;
    s.extGainDb = 0.0f;
    s.centerHz = ui->sbSACenterFreq->value() * 1e6;
    s.spanHz = ui->sbSASpan->value() * 1e6;
    s.points = 290;
    s.rawOffsetDb = 174.0f;
    s.lna = ui->sbSALNA->isChecked();
    tinySA.setSetup(s);
    tinySA.setDefaults();

    ui->lbMinFreq->setText( QString::number(1e-6*(s.centerHz - s.spanHz/2), 'f', 2) + " MHz");
    ui->lbMaxFreq->setText( QString::number(1e-6*(s.centerHz + s.spanHz/2), 'f', 2) + " MHz");

    ui->hsFreq->setRange(0, s.points);
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

void MainWindow::updateRadiationPattern()
{
    if (powerSpectra.length() > 0)
    {
        RadiationPattern rp;
        int startAngle = ui->sbRotatorStart->value();
        int endAngle = ui->sbRotatorEnd->value();
        int angleStep = ui->sbRotatorStep->value();

        int freqId = ui->hsFreq->value();   // index of the targte freq in the data

        for( int specId = 0; specId < powerSpectra.length(); specId ++ )
        {
            if ( freqId < powerSpectra.at(specId).power.length() )
            {
                float angle = startAngle + specId * angleStep;
                rp.angles.push_back(static_cast<float>(angle));
                rp.power.push_back( powerSpectra.at(specId).power.at(freqId) );
            }
        }
        ui->powerPlot->setRadiationPattern(rp);
    }
}
