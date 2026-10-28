#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QRandomGenerator>
#include <QPixmap>
#include <QColorDialog>
#include <QSettings>
#include <QEvent>
#include <QSplitter>
#include <QRegularExpression>
#include <QDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QSignalBlocker>
#include <QEventLoop>
#include <QTimer>
#include <algorithm>
#include "src/ExportDialog/exportdialog.h"
#include "src/TraceDialog/tracedialog.h"
#include "src/Report/pdfreport.h"
#include "src/Report/pdfattachments.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QDesktopServices>
#include <QUrl>
#include <cmath>
#include <limits>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle(QString("polarSA - ") + POLARSA_VERSION);
    setWindowIcon(QIcon(":/img/polarsa_notxt.ico"));

    // Setup rbw combobox
    for( QPair<QString, float> elt : rbwMap)
    {
        ui->cbRBW->addItem(elt.first);
    }

    connect( ui->pbSingleCapture, &QPushButton::pressed, this, &MainWindow::triggerSingleCapture );
    connect( ui->pbSequenceCapture, &QPushButton::pressed, this, &MainWindow::triggerSequenceCapture );
    connect( ui->pbLoopSingleCapture, &QPushButton::pressed, this, &MainWindow::triggerLoopSingleCapture );
    connect( ui->pbLoopSequenceCapture, &QPushButton::pressed, this, &MainWindow::triggerLoopSequenceCapture );
    connect( ui->pbStopCapture, &QPushButton::pressed, this, &MainWindow::triggerStopCapture );
    connect( ui->hsFreq, &QSlider::valueChanged, this, &MainWindow::freqSliderMoved);
    connect( ui->pbFreqCenter, &QPushButton::clicked, this, &MainWindow::goToCenterFrequency );
    connect( ui->pbFreqPeak, &QPushButton::clicked, this, &MainWindow::goToPeakFrequency );

    // Menus
    connect( ui->actionImportTrace, &QAction::triggered, this, &MainWindow::importData );
    connect( ui->actionExportTrace, &QAction::triggered, this, &MainWindow::exportData );
    connect( ui->actionExportPdf, &QAction::triggered, this, &MainWindow::exportPdf );
    connect( ui->actionImportConfig, &QAction::triggered, this, &MainWindow::loadConfig );
    connect( ui->actionExportConfig, &QAction::triggered, this, &MainWindow::saveConfig );
    connect( ui->actionImportCalibration, &QAction::triggered, this, &MainWindow::loadCalibrationFile );
    connect( ui->actionImportLimits, &QAction::triggered, this, &MainWindow::loadLimitsFile );
    connect( ui->actionQuit, &QAction::triggered, this, &QWidget::close );
    connect( ui->actionClearImports, &QAction::triggered, this, &MainWindow::clearImports );
    connect( ui->actionClearGraphs, &QAction::triggered, this, &MainWindow::clearGraphs );
    connect( ui->actionClearCalibration, &QAction::triggered, this, &MainWindow::clearCalibration );
    connect( ui->actionClearLimits, &QAction::triggered, this, &MainWindow::clearLimits );
    connect( ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout );
    connect( ui->actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt );

    // Status bar: config, calibration and limits in use
    configStatus = new QLabel;
    calibrationStatus = new QLabel;
    limitsStatus = new QLabel;
    for( QLabel *label : {configStatus, calibrationStatus, limitsStatus} )
    {
        label->setContentsMargins(8, 0, 8, 0);
        ui->statusbar->addPermanentWidget(label);
    }
    connect( ui->lwImports, &QListWidget::itemChanged, this, [this](QListWidgetItem *item){
        int row = ui->lwImports->row(item);
        bool checked = (item->checkState() == Qt::Checked);
        if ( row == 0 )
        {
            liveTrace.visible = checked;
        }
        else if ( row - 1 < importedMeasurements.length() )
        {
            importedMeasurements[row - 1].visible = checked;
        }
        updateRadiationPattern();
        updateSpectrumPlots();
    });
    connect( ui->lwImports, &QListWidget::itemDoubleClicked, this, &MainWindow::openTraceDialog );

    // Row 0 of the trace list is always the live capture
    liveTrace.name = "Live capture";
    liveTrace.color = qApp->palette().highlight().color();
    ui->powerPlot->setLiveColor(liveTrace.color);
    QListWidgetItem *liveItem = new QListWidgetItem(colorSwatch(liveTrace.color), liveTrace.name);
    liveItem->setFlags(liveItem->flags() | Qt::ItemIsUserCheckable);
    liveItem->setCheckState(Qt::Checked);
    liveItem->setToolTip("Current sequence capture");
    ui->lwImports->addItem(liveItem);

    // Splitter between the graphs and the control panel: visible handle, graphs get the extra space
    const QString handleStyle = "QSplitter::handle { background-color: palette(mid); margin: 2px; border-radius: 1px; }"
                                "QSplitter::handle:hover { background-color: palette(highlight); }";
    ui->mainSplitter->setStyleSheet(handleStyle);
    ui->mainSplitter->setHandleWidth(7);
    ui->mainSplitter->setStretchFactor(0, 1);
    ui->mainSplitter->setStretchFactor(1, 0);

    QSettings settings;
    if ( !ui->mainSplitter->restoreState(settings.value("mainSplitter").toByteArray()) )
    {
        ui->mainSplitter->setSizes({width() - 260, 260});
    }

    connect( &rotator, &Rotator::connected, this, [this](){ ui->lbRotatorStatus->setText("Connected"); });
    connect( &rotator, &Rotator::disconnected, this, [this](){ ui->lbRotatorStatus->setText("Disconnected"); });

    connect( &tinySA, &TinySA::connected, this, [this](){ ui->lbSAStatus->setText("Connected"); });
    connect( &tinySA, &TinySA::disconnected, this, [this](){ ui->lbSAStatus->setText("Disconnected"); });

    // Settings changed during a loop are re-sent to the tinySA before the next sweep / pass
    auto markSADirty = [this]() { saParamsDirty = true; };
    connect( ui->sbSACenterFreq, &QDoubleSpinBox::valueChanged, this, markSADirty );
    connect( ui->sbSASpan, &QDoubleSpinBox::valueChanged, this, markSADirty );
    connect( ui->sbSAAtt, &QDoubleSpinBox::valueChanged, this, markSADirty );
    connect( ui->cbRBW, &QComboBox::currentIndexChanged, this, markSADirty );
    connect( ui->sbSALNA, &QCheckBox::toggled, this, markSADirty );

    // Any setting stored in a session config marks the loaded config as modified
    auto markModified = [this]() { markConfigModified(); };
    connect( ui->sbSACenterFreq, &QDoubleSpinBox::valueChanged, this, markModified );
    connect( ui->sbSASpan, &QDoubleSpinBox::valueChanged, this, markModified );
    connect( ui->sbSAAtt, &QDoubleSpinBox::valueChanged, this, markModified );
    connect( ui->cbRBW, &QComboBox::currentIndexChanged, this, markModified );
    connect( ui->sbSALNA, &QCheckBox::toggled, this, markModified );
    connect( ui->sbRotatorStart, &QSpinBox::valueChanged, this, markModified );
    connect( ui->sbRotatorEnd, &QSpinBox::valueChanged, this, markModified );
    connect( ui->sbRotatorStep, &QSpinBox::valueChanged, this, markModified );

    // Levels: external gain (preamp / attenuator between the antenna and the tinySA) and a coarse,
    // flat unit conversion used without calibration file. Display corrections, remembered between sessions.
    const QString micro = QString("dB") + QChar(0x00B5);
    ui->cbUnit->addItem("dBm", "dBm");
    ui->cbUnit->addItem(micro + "V", "dBuV");
    ui->cbUnit->addItem(micro + "V/m", "dBuV/m");
    ui->cbUnit->addItem("dB (relative)", "dB");
    ui->cbUnit->setItemData(0, "tinySA level, no conversion", Qt::ToolTipRole);
    ui->cbUnit->setItemData(1, "dBm + 107 dB (50 " + QString(QChar(0x03A9)) + " system)", Qt::ToolTipRole);
    ui->cbUnit->setItemData(2, "dBm + 107 dB + antenna factor (flat, coarse): field strength", Qt::ToolTipRole);
    ui->cbUnit->setItemData(3, "No conversion, levels labelled dB", Qt::ToolTipRole);
    ui->sbExtGain->setValue(settings.value("corrections/extGainDb", 0.0).toDouble());
    ui->sbAntennaFactor->setValue(settings.value("levels/antennaFactorDbM", 0.0).toDouble());
    int unitIndex = ui->cbUnit->findData(settings.value("levels/unit", "dBm").toString());
    ui->cbUnit->setCurrentIndex(unitIndex >= 0 ? unitIndex : 0);
    connect( ui->sbExtGain, &QDoubleSpinBox::valueChanged, this, [this](double db) {
        QSettings().setValue("corrections/extGainDb", db);
        markConfigModified();
        updateUnits();
    });
    connect( ui->cbUnit, &QComboBox::currentIndexChanged, this, [this]() {
        QSettings().setValue("levels/unit", unitKey());
        markConfigModified();
        updateUnits();
    });
    connect( ui->sbAntennaFactor, &QDoubleSpinBox::valueChanged, this, [this](double af) {
        QSettings().setValue("levels/antennaFactorDbM", af);
        markConfigModified();
        updateUnits();
    });

    // Normalized display, remembered between sessions
    ui->cbNormalize->setChecked(settings.value("display/normalized", false).toBool());
    ui->powerPlot->setNormalize(ui->cbNormalize->isChecked());
    connect( ui->cbNormalize, &QCheckBox::toggled, this, [this](bool on) {
        QSettings().setValue("display/normalized", on);
        ui->powerPlot->setNormalize(on);
        markConfigModified();
        updateRadiationPattern();
    });

    setCaptureRunning(false);
    updateConfigLabel();
    updateUnits();
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("mainSplitter", ui->mainSplitter->saveState());
    delete ui;
}

bool MainWindow::devicesReady() const
{
    return tinySA.isOpen() && rotator.isOpen();
}

void MainWindow::setCaptureRunning(bool running)
{
    captureRunning = running;
    ui->pbSingleCapture->setEnabled(!running);
    ui->pbLoopSingleCapture->setEnabled(!running);
    ui->pbSequenceCapture->setEnabled(!running);
    ui->pbLoopSequenceCapture->setEnabled(!running);
    ui->pbStopCapture->setEnabled(running);
    ui->actionClearGraphs->setEnabled(!running);
    ui->actionImportConfig->setEnabled(!running);
}

bool MainWindow::startCapture()
{
    if ( captureRunning )
    {
        return false;
    }
    if ( !devicesReady() )
    {
        ui->statusbar->showMessage("tinySA and rotator must both be connected.");
        return false;
    }

    stopCapture = false;
    captureError.clear();
    setCaptureRunning(true);
    sendSAParameters();
    sendRotatorParameters();
    return true;
}

void MainWindow::endCapture(const QString &message)
{
    setCaptureRunning(false);
    ui->statusbar->showMessage( !devicesReady() ? "Stopped: device disconnected." : !captureError.isEmpty() ? captureError : message );
}

bool MainWindow::moveRotator(int deg)
{
    // Waits for the firmware's position notification; the UI (and the Stop button) stays responsive
    QEventLoop loop;
    bool reached = false;
    connect( &rotator, &Rotator::positionReached, &loop, [&](int at){ if ( at == deg ) { reached = true; loop.quit(); } } );
    connect( &rotator, &Rotator::disconnected, &loop, &QEventLoop::quit );
    connect( ui->pbStopCapture, &QPushButton::clicked, &loop, &QEventLoop::quit );
    QTimer::singleShot( rotator.moveTimeoutMs(), &loop, &QEventLoop::quit );
    rotator.setAngle(deg);
    loop.exec();
    if ( !reached && !stopCapture && rotator.isOpen() )
    {
        captureError = "Stopped: rotator did not reach " + QString::number(deg) + " deg.";
        stopCapture = true;
    }
    return reached;
}

PowerSpectrum MainWindow::captureSpectrum()
{
    // Retry until a valid sweep is received, Stop is pressed or the tinySA goes away
    PowerSpectrum ps = tinySA.capture();
    while ( ps.power.isEmpty() && !stopCapture && tinySA.isOpen() )
    {
        QApplication::processEvents();
        ps = tinySA.capture();
    }
    return ps;
}

void MainWindow::triggerSingleCapture()
{
    if ( !startCapture() )
    {
        return;
    }

    ui->statusbar->showMessage("Capturing...");
    PowerSpectrum ps = captureSpectrum();
    if ( !ps.freq.isEmpty() )
    {
        showSpectrum(ps);
    }
    endCapture( stopCapture ? "Stopped." : "Done." );
}

void MainWindow::triggerLoopSingleCapture()
{
    if ( !startCapture() )
    {
        return;
    }

    // Continuous sweeps; spectrum analyzer settings changed meanwhile are applied on the fly
    int count = 0;
    while ( !stopCapture && tinySA.isOpen() )
    {
        if ( saParamsDirty )
        {
            sendSAParameters();
        }
        PowerSpectrum ps = captureSpectrum();
        if ( !ps.freq.isEmpty() )
        {
            showSpectrum(ps);
            count++;
            ui->statusbar->showMessage("Loop single: sweep " + QString::number(count) + " (Stop to end)");
        }
        QApplication::processEvents();
    }
    endCapture("Stopped after " + QString::number(count) + " sweeps.");
}

void MainWindow::triggerSequenceCapture()
{
    if ( !startCapture() )
    {
        return;
    }

    bool completed = runSequence(false, QString(), 1);
    endCapture( completed ? "Done." : "Stopped." );
}

void MainWindow::triggerLoopSequenceCapture()
{
    if ( !startCapture() )
    {
        return;
    }

    // Each pass replaces the live data. The direction alternates (start->end, end->start...)
    // so the turntable never has to travel back across the whole range between passes.
    int pass = 0;
    while ( !stopCapture && devicesReady() )
    {
        pass++;
        runSequence( (pass % 2) == 0, "Pass " + QString::number(pass) + ": ", pass );
    }
    endCapture("Stopped during pass " + QString::number(pass) + ".");
}

QVector<int> MainWindow::sequenceAngles()
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

    QVector<int> angles;
    for( int a = startAngle; a <= endAngle; a += angleStep )
    {
        angles.append(a);
    }
    return angles;
}

bool MainWindow::runSequence(bool reverse, const QString &label, int pass)
{
    if ( saParamsDirty )
    {
        sendSAParameters(); // settings changed since the previous pass
    }

    QVector<int> angles = sequenceAngles();
    if ( reverse )
    {
        std::reverse(angles.begin(), angles.end());
    }

    powerSpectra.clear();
    captureAngles.clear();

    captureMetadata = QStringList{
        "date," + QDateTime::currentDateTime().toString(Qt::ISODate),
        "center_MHz," + QString::number(ui->sbSACenterFreq->value(), 'f', 4),
        "span_MHz," + QString::number(ui->sbSASpan->value(), 'f', 4),
        "rbw," + ui->cbRBW->currentText(),
        "attenuation_dB," + QString::number(ui->sbSAAtt->value()),
        "lna," + QString(ui->sbSALNA->isChecked() ? "on" : "off"),
        "ext_gain_dB," + QString::number(ui->sbExtGain->value(), 'f', 1),
        "data_unit,dBm (tinySA input level)",
        "angle_start_deg," + QString::number(ui->sbRotatorStart->value()),
        "angle_end_deg," + QString::number(ui->sbRotatorEnd->value()),
        "angle_step_deg," + QString::number(ui->sbRotatorStep->value())
    };
    if ( pass > 1 || reverse )
    {
        captureMetadata.append("loop_pass," + QString::number(pass));
        captureMetadata.append(QString("rotation,") + (reverse ? "end to start" : "start to end"));
    }

    int i = 0;
    while ( i < angles.length() && !stopCapture && devicesReady() )
    {
        int a = angles.at(i);
        ui->statusbar->showMessage(label + "Capturing " + QString::number(i + 1) + " out of " + QString::number(angles.length()));

        qDebug() << "capturing angle " << a;
        if ( !moveRotator(a) )
        {
            break;
        }
        PowerSpectrum ps = captureSpectrum();
        if (ps.power.length() > 0)
        {
            showSpectrum(ps);
            powerSpectra.append(ps);
            captureAngles.append(static_cast<float>(a));
            i++;
            if ( powerSpectra.length() == 1 )
            {
                // New capture: merge its grid, and jump to its center if the selected frequency is outside of it
                updateFrequencyRange();
                double selHz = selectedFrequency();
                if ( selHz < ps.freq.first() || selHz > ps.freq.last() )
                {
                    setSelectedFrequency(0.5 * (ps.freq.first() + ps.freq.last()));
                }
            }
            updateRadiationPattern();
            updateSpectrumPlots();
        }
        QApplication::processEvents();
    }
    return i == angles.length();
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
    saParamsDirty = false;
}

void MainWindow::sendRotatorParameters()
{
    sequenceAngles(); // validates start / end / step
    rotator.setAngle(ui->sbRotatorStart->value());
}

void MainWindow::updateRadiationPattern()
{
    // The slider indexes sliderFreqs: the merged frequency grid of every trace (live + imports)
    if ( sliderFreqs.isEmpty() )
    {
        ui->powerPlot->setRadiationPattern(RadiationPattern());
        ui->powerPlot->setOverlayPatterns({}, {});
        ui->powerPlot->setLimitCircles({}, {});
        ui->lbRPLimits->setText(QString());
        ui->lbRPMax->setText("-");
        ui->lbRPMin->setText("-");
        return;
    }

    double freqHz = selectedFrequency();
    ui->lbRPFreq->setText( QString::number(freqHz * 1e-6, 'f', 3) + " MHz");

    // Live pattern, with its rotation / gain adjustments
    RadiationPattern rp = tracePattern(liveMeasurement(), freqHz);

    // Max / min of the live pattern, with the angle where they occur
    PatternMetrics lm = PatternAnalysis::compute(rp);
    if ( lm.valid )
    {
        ui->lbRPMax->setText( QString::number(lm.maxDb, 'f', 1) + " " + Corrections::displayUnit(dataUnit()) + " @ " + QString::number(lm.maxAngleDeg, 'f', 0) + QChar(0x00B0) );
        ui->lbRPMin->setText( QString::number(lm.minDb, 'f', 1) + " " + Corrections::displayUnit(dataUnit()) + " @ " + QString::number(lm.minAngleDeg, 'f', 0) + QChar(0x00B0) );
    }
    else
    {
        ui->lbRPMax->setText("-");
        ui->lbRPMin->setText("-");
    }

    ui->powerPlot->setRadiationPattern( liveTrace.visible ? rp : RadiationPattern() );

    QList<RadiationPattern> overlays;
    QList<QColor> colors;
    for( const Measurement &m : importedMeasurements )
    {
        overlays.append( m.visible ? tracePattern(m, freqHz) : RadiationPattern() );
        colors.append(m.color);
    }
    ui->powerPlot->setOverlayPatterns(overlays, colors);

    // EMC limits at this frequency: one circle per limit line (interpolated on sloped segments)
    QList<double> limitLevels;
    QList<QColor> limitColors;
    QStringList limitTexts;
    if ( limitsComparable() && !ui->cbNormalize->isChecked() )
    {
        for( const LimitLine &line : limits.lines )
        {
            double level;
            if ( line.valueAt(freqHz, &level) )
            {
                limitLevels.append(level);
                limitColors.append(line.color);
                limitTexts.append(line.name + " " + QString::number(level, 'f', 1));
            }
        }
    }
    ui->powerPlot->setLimitCircles(limitLevels, limitColors);
    ui->lbRPLimits->setText( limitTexts.isEmpty() ? QString()
                             : "Limit: " + limitTexts.join(", ") + " " + Corrections::displayUnit(dataUnit()) );
}

void MainWindow::updateFrequencyRange()
{
    // Rebuild the slider grid from all traces, keeping the selected frequency
    double previousHz = sliderFreqs.isEmpty() ? -1.0 : selectedFrequency();

    QVector<double> freqs;
    auto addGrid = [&freqs](const QList<PowerSpectrum> &spectra) {
        if ( !spectra.isEmpty() )
        {
            for( float f : spectra.first().freq ) freqs.append(f);
        }
    };
    addGrid(powerSpectra);
    for( const Measurement &m : importedMeasurements )
    {
        addGrid(m.spectra);
    }
    std::sort(freqs.begin(), freqs.end());

    sliderFreqs.clear();
    for( double f : freqs )
    {
        if ( sliderFreqs.isEmpty() || f - sliderFreqs.last() > 1.0 ) // identical grids collapse into one
        {
            sliderFreqs.append(f);
        }
    }

    QSignalBlocker blocker(ui->hsFreq);
    if ( sliderFreqs.isEmpty() )
    {
        ui->hsFreq->setRange(0, 0);
        ui->lbMinFreq->setText("-");
        ui->lbMaxFreq->setText("-");
        ui->lbRPFreq->setText("-");
        return;
    }

    // Keep the selected frequency if it is still in range, else start from the middle
    bool keep = previousHz >= sliderFreqs.first() && previousHz <= sliderFreqs.last();
    ui->hsFreq->setRange(0, sliderFreqs.length() - 1);
    ui->hsFreq->setValue( nearestSliderIndex(keep ? previousHz : 0.5 * (sliderFreqs.first() + sliderFreqs.last())) );
    ui->lbMinFreq->setText( QString::number(1e-6 * sliderFreqs.first(), 'f', 2) + " MHz");
    ui->lbMaxFreq->setText( QString::number(1e-6 * sliderFreqs.last(), 'f', 2) + " MHz");
}

int MainWindow::nearestSliderIndex(double freqHz) const
{
    auto it = std::lower_bound(sliderFreqs.begin(), sliderFreqs.end(), freqHz);
    int i = static_cast<int>(it - sliderFreqs.begin());
    if ( i >= sliderFreqs.length() )
    {
        return sliderFreqs.length() - 1;
    }
    if ( i > 0 && freqHz - sliderFreqs.at(i - 1) < sliderFreqs.at(i) - freqHz )
    {
        return i - 1;
    }
    return i;
}

double MainWindow::selectedFrequency() const
{
    if ( sliderFreqs.isEmpty() )
    {
        return 0.0;
    }
    return sliderFreqs.at(qBound(0, ui->hsFreq->value(), static_cast<int>(sliderFreqs.length()) - 1));
}

void MainWindow::setSelectedFrequency(double freqHz)
{
    if ( !sliderFreqs.isEmpty() )
    {
        ui->hsFreq->setValue(nearestSliderIndex(freqHz)); // valueChanged -> updateRadiationPattern
    }
}

void MainWindow::goToCenterFrequency()
{
    // Middle of the live capture, else of the first visible import
    // (the middle of the merged grid can fall in a gap between bands)
    const QVector<float> *freq = nullptr;
    if ( !powerSpectra.isEmpty() && !powerSpectra.first().freq.isEmpty() )
    {
        freq = &powerSpectra.first().freq;
    }
    for( int i = 0; !freq && i < importedMeasurements.length(); i++ )
    {
        const Measurement &m = importedMeasurements.at(i);
        if ( m.visible && !m.spectra.isEmpty() && !m.spectra.first().freq.isEmpty() )
        {
            freq = &m.spectra.first().freq;
        }
    }
    if ( freq )
    {
        setSelectedFrequency(0.5 * (freq->first() + freq->last()));
    }
}

void MainWindow::goToPeakFrequency()
{
    // Highest power over all angles and frequencies of the visible traces
    bool found = false;
    float peakPower = 0;
    double peakFreq = 0;
    float peakAngle = 0;
    QString peakTrace;
    auto search = [&](const Measurement &m) {
        for( int s = 0; s < m.spectra.length() && s < m.angles.length(); s++ )
        {
            const PowerSpectrum &ps = m.spectra.at(s);
            for( int i = 0; i < ps.power.length() && i < ps.freq.length(); i++ )
            {
                float p = ps.power.at(i) + static_cast<float>(m.gainOffsetDb + correctionAt(ps.freq.at(i)));
                if ( !found || p > peakPower )
                {
                    found = true;
                    peakPower = p;
                    peakFreq = ps.freq.at(i);
                    peakAngle = static_cast<float>(PatternAnalysis::wrap360(m.angles.at(s) + m.angleOffsetDeg));
                    peakTrace = m.name;
                }
            }
        }
    };
    if ( liveTrace.visible )
    {
        search(liveMeasurement());
    }
    for( const Measurement &m : importedMeasurements )
    {
        if ( m.visible )
        {
            search(m);
        }
    }

    if ( !found )
    {
        ui->statusbar->showMessage("No visible trace to search.");
        return;
    }
    setSelectedFrequency(peakFreq);
    ui->statusbar->showMessage("Peak: " + QString::number(peakPower, 'f', 1) + " " + Corrections::displayUnit(dataUnit()) + " at "
                               + QString::number(peakFreq * 1e-6, 'f', 3) + " MHz, "
                               + QString::number(peakAngle) + QChar(0x00B0) + " (" + peakTrace + ")");
}

QColor MainWindow::nextImportColor()
{
    // Red, yellow, cyan first, then hand-picked colors that stay distinct from each other, then random
    static const QList<QColor> importPalette = {
        QColor(Qt::red),
        QColor(Qt::yellow),
        QColor(Qt::cyan),
        QColor("#00d000"),  // green
        QColor("#ff8c00"),  // orange
        QColor("#3c7dff"),  // blue
        QColor("#ff00ff"),  // magenta
        QColor("#c08a50"),  // tan
        QColor("#ff8fc8"),  // pink
        QColor("#a0a0a0"),  // grey
        QColor("#b0ff60"),  // light lime
        QColor("#8a5cff"),  // violet
        QColor("#008b8b"),  // teal
        QColor("#a52a2a"),  // brown
    };

    int id = importCount++;
    if ( id < importPalette.length() )
    {
        return importPalette.at(id);
    }
    return QColor::fromHsv(QRandomGenerator::global()->bounded(360),
                           QRandomGenerator::global()->bounded(160, 256),
                           QRandomGenerator::global()->bounded(180, 256));
}

QIcon MainWindow::colorSwatch(const QColor &color)
{
    QPixmap swatch(12, 12);
    swatch.fill(color);
    return QIcon(swatch);
}

Measurement MainWindow::liveMeasurement() const
{
    Measurement m = liveTrace;
    m.spectra = powerSpectra;
    m.angles = captureAngles;
    m.metadata = captureMetadata;
    return m;
}

RadiationPattern MainWindow::tracePattern(const Measurement &m, double freqHz) const
{
    return PatternAnalysis::adjusted(PatternAnalysis::patternAt(m.spectra, m.angles, freqHz),
                                     m.angleOffsetDeg, m.gainOffsetDb + correctionAt(freqHz));
}

void MainWindow::openTraceDialog(QListWidgetItem *item)
{
    int row = ui->lwImports->row(item);
    if ( row < 0 || row - 1 >= importedMeasurements.length() )
    {
        return;
    }
    if ( captureRunning )
    {
        ui->statusbar->showMessage("Stop the capture to edit a trace.");
        return;
    }

    bool isLive = (row == 0);
    double freqHz = selectedFrequency();
    Measurement trace = isLive ? liveMeasurement() : importedMeasurements.at(row - 1);

    // Imports are compared to the live capture (when it has data at this frequency)
    PatternMetrics reference;
    if ( !isLive )
    {
        reference = PatternAnalysis::compute(tracePattern(liveMeasurement(), freqHz));
    }

    TraceDialog dialog(trace, isLive, freqHz, reference, correctionAt(freqHz), dataUnit(), this);
    connect( &dialog, &TraceDialog::previewChanged, this, [this, row](double angle, double gain, const QColor &color) {
        applyTraceSettings(row, angle, gain, color);
    });

    if ( dialog.exec() != QDialog::Accepted )
    {
        applyTraceSettings(row, trace.angleOffsetDeg, trace.gainOffsetDb, trace.color); // undo the preview
        return;
    }
    if ( dialog.removeRequested() )
    {
        removeImport(row - 1);
        return;
    }
    applyTraceSettings(row, dialog.angleOffset(), dialog.gainOffset(), dialog.color());
}

void MainWindow::applyTraceSettings(int row, double angleOffsetDeg, double gainOffsetDb, const QColor &color)
{
    Measurement &t = (row == 0) ? liveTrace : importedMeasurements[row - 1];
    t.angleOffsetDeg = angleOffsetDeg;
    t.gainOffsetDb = gainOffsetDb;
    t.color = color;

    // The list shows the adjustments next to the name
    QString text = t.name;
    if ( angleOffsetDeg != 0 || gainOffsetDb != 0 )
    {
        text += "  [" + QString::number(angleOffsetDeg, 'f', 1) + QChar(0x00B0) + ", "
                + (gainOffsetDb >= 0 ? "+" : "") + QString::number(gainOffsetDb, 'f', 1) + " dB]";
    }
    QListWidgetItem *item = ui->lwImports->item(row);
    {
        QSignalBlocker blocker(ui->lwImports);
        item->setIcon(colorSwatch(color));
        item->setText(text);
    }

    if ( row == 0 )
    {
        ui->powerPlot->setLiveColor(color);
    }
    updateRadiationPattern();
    updateSpectrumPlots();
}

void MainWindow::removeImport(int index)
{
    if ( index < 0 || index >= importedMeasurements.length() )
    {
        return;
    }
    importedMeasurements.removeAt(index);
    delete ui->lwImports->takeItem(index + 1);
    updateFrequencyRange();
    updateRadiationPattern();
    updateSpectrumPlots();
}

QString MainWindow::unitKey() const
{
    return ui->cbUnit->currentData().toString();
}

double MainWindow::unitOffsetDb() const
{
    // Flat conversions from dBm (50 ohm system), used when no calibration file is loaded
    const QString key = unitKey();
    if ( key == "dBuV" )
    {
        return 107.0;
    }
    if ( key == "dBuV/m" )
    {
        return 107.0 + ui->sbAntennaFactor->value();
    }
    return 0.0;
}

QString MainWindow::levelDescription() const
{
    QString d = "tinySA level (dBm)";
    const double ext = ui->sbExtGain->value();
    if ( ext != 0 )
    {
        d += QString(" %1 %2 dB external gain").arg(ext > 0 ? "-" : "+").arg(QString::number(std::abs(ext), 'f', 1));
    }
    if ( calibration.isValid() )
    {
        d += " + calibration " + calibration.name
             + (calibration.description.isEmpty() ? QString() : " (" + calibration.description + ")");
    }
    else if ( unitKey() == "dBuV" )
    {
        d += " + 107 dB (dBm to dB" + QString(QChar(0x00B5)) + "V, 50 " + QChar(0x03A9) + ")";
    }
    else if ( unitKey() == "dBuV/m" )
    {
        d += " + 107 dB + AF " + QString::number(ui->sbAntennaFactor->value(), 'f', 1) + " dB/m (flat, coarse)";
    }
    return d + " = " + Corrections::displayUnit(dataUnit());
}

QStringList MainWindow::levelsMetadata() const
{
    // Data files always hold tinySA levels in dBm: the conversion is recorded, not applied
    return QStringList{
        "calibration," + (calibration.isValid() ? calibration.name + " (" + calibration.unit + ")" : QString("none")),
        "levels," + levelDescription() + " (display only: the data are tinySA input levels in dBm)"
    };
}

QString MainWindow::dataUnit() const
{
    return calibration.isValid() ? calibration.unit : unitKey();
}

double MainWindow::correctionAt(double hz) const
{
    // level shown = tinySA level - external gain + calibration(f) or flat unit conversion
    const double conversion = calibration.isValid() ? calibration.at(hz) : unitOffsetDb();
    return conversion - ui->sbExtGain->value();
}

void MainWindow::updateLevelControls()
{
    const bool cal = calibration.isValid();
    ui->cbUnit->setEnabled(!cal);
    ui->cbUnit->setToolTip( cal ? "Set by the calibration file " + calibration.name + " ("
                                  + Corrections::displayUnit(calibration.unit) + ")"
                                : QString("Coarse conversion of the levels (flat in frequency). A calibration file, when loaded, sets the unit instead.") );
    const bool af = !cal && unitKey() == "dBuV/m";
    ui->label_af->setVisible(af);
    ui->sbAntennaFactor->setVisible(af);
}

void MainWindow::clearGraphs()
{
    if ( captureRunning )
    {
        return;
    }
    if ( !powerSpectra.isEmpty()
         && QMessageBox::question(this, "Clear Graphs", "Clear the live capture from all graphs?\nImported traces are kept.")
                != QMessageBox::Yes )
    {
        return;
    }
    powerSpectra.clear();
    captureAngles.clear();
    captureMetadata.clear();
    lastSpectrum = PowerSpectrum();
    ui->spectrumPlot->setPowerSpectrum(PowerSpectrum());
    updateFrequencyRange();
    updateRadiationPattern();
    updateSpectrumPlots();
    ui->statusbar->showMessage("Live capture cleared.");
}

void MainWindow::showAbout()
{
    QDialog dialog(this);
    dialog.setWindowTitle("About polarSA");

    // Quantor Innovation logo (white artwork) on a dark banner, readable with any theme
    QLabel *logo = new QLabel;
    QPixmap pix(":/img/quantor_logo.png");
    logo->setPixmap(pix.scaledToWidth(260, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);
    logo->setStyleSheet("background-color: #15171c; padding: 18px; border-radius: 6px;");

    QLabel *text = new QLabel;
    text->setTextFormat(Qt::RichText);
    text->setOpenExternalLinks(true);
    text->setWordWrap(true);
    text->setText( QString("<h3>polarSA %1</h3>").arg(POLARSA_VERSION)
                   + "<p>Radiation pattern and EMC pre-compliance measurements with a tinySA Ultra and a rotating "
                     "DUT platform.</p>"
                     "<p>Designed and maintained by <b>Quantor Innovation</b><br>"
                     "Author: Maxime Bezanilla<br>"
                     "<a href=\"https://www.quantor.fr\">www.quantor.fr</a></p>"
                     "<p>License: GPL v3. Built with Qt and QCustomPlot.</p>" );

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect( buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept );

    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->addWidget(logo);
    layout->addWidget(text);
    layout->addWidget(buttons);
    dialog.resize(420, dialog.sizeHint().height());
    dialog.exec();
}

bool MainWindow::limitsComparable() const
{
    return !limits.isEmpty() && Corrections::sameUnit(limits.unit, dataUnit());
}

PowerSpectrum MainWindow::corrected(const PowerSpectrum &ps, double gainDb) const
{
    PowerSpectrum out = ps;
    for( int i = 0; i < out.power.length() && i < out.freq.length(); i++ )
    {
        out.power[i] += static_cast<float>(gainDb + correctionAt(out.freq.at(i)));
    }
    return out;
}

PowerSpectrum MainWindow::maxHold(const Measurement &m) const
{
    // Max over all angles, then the trace's gain offset and the setup calibration
    PowerSpectrum maxPs;
    for( const PowerSpectrum &ps : m.spectra )
    {
        if ( maxPs.power.isEmpty() )
        {
            maxPs = ps;
            continue;
        }
        for( int k = 0; k < ps.power.length() && k < maxPs.power.length(); k++ )
        {
            maxPs.power[k] = qMax(maxPs.power.at(k), ps.power.at(k));
        }
    }
    return corrected(maxPs, m.gainOffsetDb);
}

void MainWindow::showSpectrum(const PowerSpectrum &ps)
{
    lastSpectrum = ps;
    ui->spectrumPlot->setPowerSpectrum(corrected(ps, 0));
}

void MainWindow::updateSpectrumPlots()
{
    // Spectrum tab: last sweep
    if ( !lastSpectrum.freq.isEmpty() )
    {
        ui->spectrumPlot->setPowerSpectrum(corrected(lastSpectrum, 0));
    }

    // Max Emissions: max hold of the live capture and of every visible import
    ui->maxSpectrumPlot->setTraceColor(liveTrace.color);
    ui->maxSpectrumPlot->setPowerSpectrum( liveTrace.visible && !powerSpectra.isEmpty() ? maxHold(liveMeasurement()) : PowerSpectrum() );
    QList<PowerSpectrum> overlays;
    QList<QColor> colors;
    for( const Measurement &m : importedMeasurements )
    {
        overlays.append( m.visible ? maxHold(m) : PowerSpectrum() );
        colors.append(m.color);
    }
    ui->maxSpectrumPlot->setOverlaySpectra(overlays, colors);

    // Worst margin to the limits in the group title
    QString title = "Max hold over all angles of the live capture and of the visible imports";
    if ( limitsComparable() )
    {
        const LimitCheck *worst = nullptr;
        const QList<LimitCheck> checks = limitChecks();
        for( const LimitCheck &c : checks )
        {
            if ( c.covered && (!worst || c.marginDb < worst->marginDb) )
            {
                worst = &c;
            }
        }
        if ( worst )
        {
            title += QString("  %1  worst margin %2 dB to %3 at %4 MHz (%5)")
                         .arg(QChar(0x2014))
                         .arg((worst->marginDb >= 0 ? "+" : "") + QString::number(worst->marginDb, 'f', 1))
                         .arg(worst->limit)
                         .arg(QString::number(worst->freqHz * 1e-6, 'f', 3))
                         .arg(worst->trace);
        }
    }
    else if ( !limits.isEmpty() )
    {
        title += QString("  %1  limits in %2, levels in %3: choose that unit (Levels) or load a calibration to compare")
                     .arg(QChar(0x2014))
                     .arg(Corrections::displayUnit(limits.unit))
                     .arg(Corrections::displayUnit(dataUnit()));
    }
    ui->lbMaxInfo->setText(title);
}

QList<LimitCheck> MainWindow::limitChecks() const
{
    QList<LimitCheck> checks;
    QList<Measurement> traces;
    if ( !powerSpectra.isEmpty() && liveTrace.visible )
    {
        traces.append(liveMeasurement());
    }
    for( const Measurement &m : importedMeasurements )
    {
        if ( m.visible )
        {
            traces.append(m);
        }
    }

    for( const Measurement &m : traces )
    {
        const PowerSpectrum mh = maxHold(m);
        for( const LimitLine &line : limits.lines )
        {
            LimitCheck c;
            c.trace = m.name;
            c.traceColor = m.color;
            c.limit = line.name;
            c.limitColor = line.color;
            for( int i = 0; i < mh.power.length() && i < mh.freq.length(); i++ )
            {
                double limitDb;
                if ( !line.valueAt(mh.freq.at(i), &limitDb) )
                {
                    continue;
                }
                double margin = limitDb - mh.power.at(i);
                if ( !c.covered || margin < c.marginDb )
                {
                    c.covered = true;
                    c.marginDb = margin;
                    c.freqHz = mh.freq.at(i);
                    c.levelDb = mh.power.at(i);
                    c.limitDb = limitDb;
                }
            }
            checks.append(c);
        }
    }
    return checks;
}

void MainWindow::updateUnits()
{
    const QString unit = dataUnit();
    ui->powerPlot->setUnit(unit);
    ui->spectrumPlot->setUnit(unit);
    ui->maxSpectrumPlot->setUnit(unit);
    ui->spectrumPlot->setLimits(limits, limitsComparable());
    ui->maxSpectrumPlot->setLimits(limits, limitsComparable());
    updateLevelControls();
    updateCorrectionLabels();
    updateRadiationPattern();
    updateSpectrumPlots();
}

void MainWindow::updateCorrectionLabels()
{
    if ( calibration.isValid() )
    {
        calibrationStatus->setText("Calibration: " + calibration.name);
        calibrationStatus->setToolTip( QDir::toNativeSeparators(calibration.path)
                                       + "\nUnit: " + Corrections::displayUnit(calibration.unit)
                                       + "\nRange: " + QString::number(calibration.freqHz.first() * 1e-6, 'f', 3) + " - "
                                       + QString::number(calibration.freqHz.last() * 1e-6, 'f', 3) + " MHz ("
                                       + QString::number(calibration.freqHz.length()) + " points, edge values used outside)"
                                       + (calibration.description.isEmpty() ? QString() : "\n" + calibration.description)
                                       + "\n\n" + levelDescription() );
    }
    else
    {
        calibrationStatus->setText("Calibration: none");
        calibrationStatus->setToolTip(levelDescription());
    }

    if ( !limits.isEmpty() )
    {
        QStringList names;
        for( const LimitLine &line : limits.lines )
        {
            names << line.name;
        }
        limitsStatus->setText("Limits: " + limits.name);
        limitsStatus->setToolTip( limits.name + "\n" + QDir::toNativeSeparators(limits.path)
                                  + "\nUnit: " + Corrections::displayUnit(limits.unit) + "\nLimits: " + names.join(", ")
                                  + (limits.description.isEmpty() ? QString() : "\n" + limits.description)
                                  + (limitsComparable() ? QString()
                                     : "\n\nNot compared: the levels are in " + Corrections::displayUnit(dataUnit())
                                       + ". Choose the unit " + Corrections::displayUnit(limits.unit)
                                       + " (Levels) or load a calibration in that unit.") );
        limitsStatus->setStyleSheet( limitsComparable() ? QString() : QString("color: #d08000;") );
    }
    else
    {
        limitsStatus->setText("Limits: none");
        limitsStatus->setToolTip(QString());
        limitsStatus->setStyleSheet(QString());
    }
}

QString MainWindow::shippedDir(const QString &sub) const
{
    // limits/ and calibration/ are shipped next to the executable; in development, use the source tree
    QDir app(QCoreApplication::applicationDirPath());
    QStringList candidates = { app.filePath(sub), app.filePath("../" + sub) };
#ifdef POLARSA_SOURCE_DIR
    candidates << QString(POLARSA_SOURCE_DIR) + "/" + sub;
#endif
    for( const QString &candidate : candidates )
    {
        if ( QDir(candidate).exists() )
        {
            return QDir(candidate).absolutePath();
        }
    }
    return QString();
}

bool MainWindow::applyCalibrationFile(const QString &path, QString *error)
{
    CalibrationTable table;
    if ( !Corrections::loadCalibration(path, table, error) )
    {
        return false;
    }
    calibration = table;
    markConfigModified();
    updateUnits();
    return true;
}

bool MainWindow::applyLimitsFile(const QString &path, QString *error)
{
    LimitSet set;
    if ( !Corrections::loadLimits(path, set, error) )
    {
        return false;
    }
    limits = set;
    markConfigModified();
    updateUnits();
    return true;
}

void MainWindow::loadCalibrationFile()
{
    QString dir = QSettings().value("dirs/calibration").toString();
    if ( dir.isEmpty() )
    {
        dir = shippedDir("calibration");
    }
    QString path = QFileDialog::getOpenFileName(this, "Load Setup Calibration", dir, "Calibration CSV (*.csv);;All files (*)");
    if ( path.isEmpty() )
    {
        return;
    }
    rememberDir("calibration", path);
    QString error;
    if ( !applyCalibrationFile(path, &error) )
    {
        QMessageBox::warning(this, "Load Setup Calibration", QFileInfo(path).fileName() + ":\n" + error);
        return;
    }
    ui->statusbar->showMessage("Calibration loaded: " + calibration.name + ", levels now in " + Corrections::displayUnit(dataUnit()));
}

void MainWindow::clearCalibration()
{
    if ( !calibration.isValid() )
    {
        return;
    }
    calibration = CalibrationTable();
    markConfigModified();
    updateUnits();
}

void MainWindow::loadLimitsFile()
{
    QString dir = QSettings().value("dirs/limits").toString();
    if ( dir.isEmpty() )
    {
        dir = shippedDir("limits");
    }
    QString path = QFileDialog::getOpenFileName(this, "Load EMC Limits", dir, "Limits CSV (*.csv);;All files (*)");
    if ( path.isEmpty() )
    {
        return;
    }
    rememberDir("limits", path);
    QString error;
    if ( !applyLimitsFile(path, &error) )
    {
        QMessageBox::warning(this, "Load EMC Limits", QFileInfo(path).fileName() + ":\n" + error);
        return;
    }
    ui->statusbar->showMessage( limitsComparable()
                                ? "Limits loaded: " + limits.name
                                : "Limits loaded: " + limits.name + " (in " + Corrections::displayUnit(limits.unit)
                                  + ", levels in " + Corrections::displayUnit(dataUnit()) + ": load a calibration to compare)" );
}

void MainWindow::clearLimits()
{
    if ( limits.isEmpty() )
    {
        return;
    }
    limits = LimitSet();
    markConfigModified();
    updateUnits();
}

QString MainWindow::lastDir(const QString &key) const
{
    QSettings settings;
    QString dir = settings.value("dirs/" + key).toString();
    if ( dir.isEmpty() )
    {
        dir = settings.value("lastDataDir").toString(); // single folder used by earlier versions
    }
    return dir;
}

void MainWindow::rememberDir(const QString &key, const QString &filePath)
{
    QSettings().setValue("dirs/" + key, QFileInfo(filePath).absolutePath());
}

QString MainWindow::configDisplayName() const
{
    if ( currentConfigPath.isEmpty() )
    {
        return QString();
    }
    return QFileInfo(currentConfigPath).fileName() + (configModified ? " (modified)" : "");
}

void MainWindow::updateConfigLabel()
{
    QString title = QString("polarSA - ") + POLARSA_VERSION;
    if ( currentConfigPath.isEmpty() )
    {
        configStatus->setText("Config: none");
        configStatus->setToolTip("File > Import Config... / Export Config...");
        setWindowTitle(title);
        return;
    }
    QString name = QFileInfo(currentConfigPath).fileName();
    configStatus->setText("Config: " + (configModified ? name + " *" : name));
    configStatus->setToolTip( QDir::toNativeSeparators(currentConfigPath)
                              + (configModified ? "\nModified since imported / exported" : QString()) );
    setWindowTitle(title + "  -  " + name + (configModified ? " *" : ""));
}

void MainWindow::markConfigModified()
{
    if ( !configModified && !loadingConfig )
    {
        configModified = true;
        updateConfigLabel();
    }
}

/*
 * Session config (JSON): settings only, no measurement data.
 * {
 *   "polarSA_config": 1,
 *   "spectrum_analyzer": { "center_MHz", "span_MHz", "rbw", "attenuation_dB", "lna" },
 *   "rotator": { "start_deg", "end_deg", "step_deg" },
 *   "display": { "normalized" },
 *   "notes": { "dut", "antenna", "matching", "polarization", "operator", "comment" }
 * }
 */
void MainWindow::saveConfig()
{
    QString defaultName = currentConfigPath.isEmpty() ? QString("polarSA_config.json") : QFileInfo(currentConfigPath).fileName();
    QString path = QFileDialog::getSaveFileName(this, "Save Session Config",
                                                QDir(lastDir("config")).filePath(defaultName),
                                                "polarSA config (*.json);;All files (*)");
    if ( path.isEmpty() )
    {
        return;
    }
    rememberDir("config", path);

    QString error;
    if ( !writeConfigFile(path, &error) )
    {
        QMessageBox::warning(this, "Save Session Config", "Cannot write the file:\n" + error);
        return;
    }
    ui->statusbar->showMessage("Config saved to " + QDir::toNativeSeparators(path));
}

bool MainWindow::writeConfigFile(const QString &path, QString *error)
{
    QJsonObject sa;
    sa["center_MHz"] = ui->sbSACenterFreq->value();
    sa["span_MHz"] = ui->sbSASpan->value();
    sa["rbw"] = ui->cbRBW->currentText();
    sa["attenuation_dB"] = ui->sbSAAtt->value();
    sa["lna"] = ui->sbSALNA->isChecked();

    QJsonObject rot;
    rot["start_deg"] = ui->sbRotatorStart->value();
    rot["end_deg"] = ui->sbRotatorEnd->value();
    rot["step_deg"] = ui->sbRotatorStep->value();

    QJsonObject display;
    display["normalized"] = ui->cbNormalize->isChecked();

    QJsonObject corrections;
    corrections["ext_gain_dB"] = ui->sbExtGain->value();
    corrections["unit"] = unitKey();
    corrections["antenna_factor_dB_m"] = ui->sbAntennaFactor->value();
    corrections["calibration_file"] = calibration.isValid() ? calibration.path : QString();
    corrections["limits_file"] = limits.isEmpty() ? QString() : limits.path;

    QJsonObject notes;
    QSettings settings;
    settings.beginGroup("export");
    for( const QString &key : {"dut", "antenna", "matching", "polarization", "operator", "comment"} )
    {
        notes[key] = settings.value(key).toString();
    }
    settings.endGroup();

    QJsonObject root;
    root["polarSA_config"] = 1;
    root["app_version"] = POLARSA_VERSION;
    root["saved"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["spectrum_analyzer"] = sa;
    root["rotator"] = rot;
    root["display"] = display;
    root["corrections"] = corrections;
    root["notes"] = notes;

    QFile file(path);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 )
    {
        *error = file.errorString();
        return false;
    }

    currentConfigPath = path;
    configModified = false;
    updateConfigLabel();
    return true;
}

void MainWindow::loadConfig()
{
    QString path = QFileDialog::getOpenFileName(this, "Load Session Config", lastDir("config"),
                                                "polarSA config (*.json);;All files (*)");
    if ( path.isEmpty() )
    {
        return;
    }
    rememberDir("config", path);

    QString error;
    QStringList warnings;
    if ( !readConfigFile(path, &warnings, &error) )
    {
        QMessageBox::warning(this, "Load Session Config", error);
        return;
    }
    ui->statusbar->showMessage("Config loaded: " + QFileInfo(path).fileName());
    if ( !warnings.isEmpty() )
    {
        QMessageBox::warning(this, "Load Session Config", warnings.join("\n"));
    }
}

bool MainWindow::readConfigFile(const QString &path, QStringList *warnings, QString *error)
{
    QFile file(path);
    if ( !file.open(QIODevice::ReadOnly) )
    {
        *error = "Cannot read the file:\n" + file.errorString();
        return false;
    }
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if ( doc.isNull() || !doc.isObject() || !doc.object().contains("polarSA_config") )
    {
        *error = "Not a polarSA config file" + (doc.isNull() ? ":\n" + parseError.errorString() : QString("."));
        return false;
    }
    QJsonObject root = doc.object();

    loadingConfig = true;
    QJsonObject sa = root["spectrum_analyzer"].toObject();
    if ( sa.contains("center_MHz") ) ui->sbSACenterFreq->setValue(sa["center_MHz"].toDouble());
    if ( sa.contains("span_MHz") ) ui->sbSASpan->setValue(sa["span_MHz"].toDouble());
    if ( sa.contains("attenuation_dB") ) ui->sbSAAtt->setValue(sa["attenuation_dB"].toDouble());
    if ( sa.contains("lna") ) ui->sbSALNA->setChecked(sa["lna"].toBool());
    if ( sa.contains("rbw") )
    {
        int index = ui->cbRBW->findText(sa["rbw"].toString());
        if ( index >= 0 )
        {
            ui->cbRBW->setCurrentIndex(index);
        }
        else
        {
            warnings->append("unknown RBW '" + sa["rbw"].toString() + "', kept " + ui->cbRBW->currentText());
        }
    }

    QJsonObject rot = root["rotator"].toObject();
    if ( rot.contains("start_deg") ) ui->sbRotatorStart->setValue(rot["start_deg"].toInt());
    if ( rot.contains("end_deg") ) ui->sbRotatorEnd->setValue(rot["end_deg"].toInt());
    if ( rot.contains("step_deg") ) ui->sbRotatorStep->setValue(rot["step_deg"].toInt());

    QJsonObject display = root["display"].toObject();
    if ( display.contains("normalized") ) ui->cbNormalize->setChecked(display["normalized"].toBool());

    // Corrections: external gain, calibration and limits files (empty name = none)
    if ( root.contains("corrections") )
    {
        QJsonObject corr = root["corrections"].toObject();
        if ( corr.contains("ext_gain_dB") ) ui->sbExtGain->setValue(corr["ext_gain_dB"].toDouble());
        if ( corr.contains("antenna_factor_dB_m") ) ui->sbAntennaFactor->setValue(corr["antenna_factor_dB_m"].toDouble());
        if ( corr.contains("unit") )
        {
            int index = ui->cbUnit->findData(corr["unit"].toString());
            if ( index >= 0 )
            {
                ui->cbUnit->setCurrentIndex(index);
            }
            else
            {
                warnings->append("unknown unit '" + corr["unit"].toString() + "', kept " + unitKey());
            }
        }

        QString calPath = corr["calibration_file"].toString();
        QString err;
        if ( calPath.isEmpty() )
        {
            clearCalibration();
        }
        else if ( !applyCalibrationFile(calPath, &err) )
        {
            warnings->append("calibration " + QDir::toNativeSeparators(calPath) + ": " + err);
        }

        QString limPath = corr["limits_file"].toString();
        if ( limPath.isEmpty() )
        {
            clearLimits();
        }
        else if ( !applyLimitsFile(limPath, &err) )
        {
            warnings->append("limits " + QDir::toNativeSeparators(limPath) + ": " + err);
        }
    }

    // Notes pre-fill the next export / report dialog
    if ( root.contains("notes") )
    {
        QJsonObject notes = root["notes"].toObject();
        QSettings settings;
        settings.beginGroup("export");
        for( const QString &key : notes.keys() )
        {
            settings.setValue(key, notes[key].toString());
        }
        settings.endGroup();
    }
    loadingConfig = false;

    currentConfigPath = path;
    configModified = false;
    updateConfigLabel();
    return true;
}

ReportData MainWindow::buildReportData(const QStringList &notes, bool embedData) const
{
    ReportData data;
    double freqHz = selectedFrequency();
    data.configName = configDisplayName();
    data.freqHz = freqHz;
    data.normalized = ui->cbNormalize->isChecked();
    data.notes = notes;
    data.captureSettings = powerSpectra.isEmpty() ? QStringList() : captureMetadata;
    data.polarPlot = ui->powerPlot;
    data.maxPlot = ui->maxSpectrumPlot;
    data.unit = dataUnit();
    data.calibration = levelDescription();
    if ( calibration.isValid() && !calibration.covers(freqHz) )
    {
        data.calibration += " - the selected frequency is outside of the calibration range, edge value used";
    }
    if ( !limits.isEmpty() )
    {
        data.limits = limits.name + " (" + QFileInfo(limits.path).fileName() + ", " + Corrections::displayUnit(limits.unit) + ")";
        if ( limitsComparable() )
        {
            data.limitChecks = limitChecks();
        }
        else
        {
            data.limitsNote = "Limits are in " + Corrections::displayUnit(limits.unit) + " while the levels are in "
                              + Corrections::displayUnit(dataUnit()) + ": no comparison.";
        }
    }

    // Visible traces: live capture first, then imports in list order
    QList<QPair<Measurement, bool>> traces; // (trace, isLive)
    if ( !powerSpectra.isEmpty() && liveTrace.visible )
    {
        traces.append({liveMeasurement(), true});
    }
    for( const Measurement &m : importedMeasurements )
    {
        if ( m.visible )
        {
            traces.append({m, false});
        }
    }
    QSet<QString> usedNames; // embedded file names, lower case
    for( const auto &entry : traces )
    {
        const Measurement &m = entry.first;
        ReportTrace t;
        t.name = m.name;
        t.source = entry.second ? QString() : QDir::toNativeSeparators(m.sourcePath);
        t.color = m.color;
        t.angleOffsetDeg = m.angleOffsetDeg;
        t.gainOffsetDb = m.gainOffsetDb;
        t.metrics = PatternAnalysis::compute(tracePattern(m, freqHz));
        t.metadata = m.metadata;
        if ( embedData )
        {
            embedTraceData(t, m, entry.second, notes, usedNames);
        }
        data.traces.append(t);
    }
    return data;
}

// Fills t.dataCsv / t.dataFileName: the CSV Export Data would write for this trace
// (notes and config for the live capture, display adjustments applied)
void MainWindow::embedTraceData(ReportTrace &t, const Measurement &m, bool isLive,
                                const QStringList &notes, QSet<QString> &usedNames) const
{
    Measurement csv = m;
    QString name = m.name;
    if ( isLive )
    {
        csv.metadata = notes;
        if ( !currentConfigPath.isEmpty() )
        {
            csv.metadata.append("config," + configDisplayName());
        }
        csv.metadata += levelsMetadata();
        csv.metadata += m.metadata;

        // Named like an Export Data file: polarSA_<dut>_<capture date>.csv
        QString dut, date;
        for( const QString &line : csv.metadata )
        {
            if ( line.startsWith("dut,") && dut.isEmpty() ) dut = line.mid(4).trimmed();
            if ( line.startsWith("date,") && date.isEmpty() ) date = line.mid(5).trimmed();
        }
        dut.replace(QRegularExpression("[^A-Za-z0-9_.-]+"), "_");
        QDateTime captured = QDateTime::fromString(date, Qt::ISODate);
        name = "polarSA_" + (dut.isEmpty() ? QString() : dut + "_")
               + (captured.isValid() ? captured.toString("yyyyMMdd_HHmmss") : QString("live")) + ".csv";
    }
    applyAdjustments(csv);

    if ( !name.endsWith(".csv", Qt::CaseInsensitive) )
    {
        name += ".csv";
    }
    QString base = name.left(name.length() - 4);
    for( int n = 2; usedNames.contains(name.toLower()); n++ )
    {
        name = base + "_" + QString::number(n) + ".csv";
    }
    usedNames.insert(name.toLower());

    t.dataFileName = name;
    t.dataCsv = measurementCsv(csv);
}

void MainWindow::exportPdf()
{
    if ( sliderFreqs.isEmpty() )
    {
        QMessageBox::information(this, "PDF Report", "Nothing to report: capture or import data first.");
        return;
    }

    ReportData preview = buildReportData(QStringList());
    QString summary = "Report at " + QString::number(preview.freqHz * 1e-6, 'f', 3) + " MHz, "
                      + QString::number(preview.traces.length()) + " visible trace(s)";
    ExportDialog dialog(summary, this, "PDF Report - Notes", "Create PDF...");
    dialog.addEmbedDataOption();
    if ( dialog.exec() != QDialog::Accepted )
    {
        return;
    }

    QString dut = dialog.dutName();
    dut.replace(QRegularExpression("[^A-Za-z0-9_.-]+"), "_");
    QString defaultName = "polarSA_report_" + (dut.isEmpty() ? QString() : dut + "_")
                          + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".pdf";
    QString path = QFileDialog::getSaveFileName(this, "PDF Report", QDir(lastDir("report")).filePath(defaultName),
                                                "PDF files (*.pdf)");
    if ( path.isEmpty() )
    {
        return;
    }
    rememberDir("report", path);

    ReportData data = buildReportData(dialog.metadata(), dialog.embedData());
    QString error;
    if ( !PdfReport::write(path, data, &error) )
    {
        QMessageBox::warning(this, "PDF Report", "Report failed:\n" + error);
        return;
    }
    ui->statusbar->showMessage("Report saved to " + QDir::toNativeSeparators(path));
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void MainWindow::exportData()
{
    if ( powerSpectra.isEmpty() )
    {
        QMessageBox::information(this, "Export Data", "Nothing to export: run a sequence capture first.");
        return;
    }

    const QVector<float> &freq = powerSpectra.first().freq;
    QString summary = QString::number(powerSpectra.length()) + " angles, "
                      + QString::number(freq.length()) + " points, "
                      + QString::number(1e-6 * freq.first(), 'f', 3) + " - "
                      + QString::number(1e-6 * freq.last(), 'f', 3) + " MHz";
    ExportDialog dialog(summary, this);
    if ( dialog.exec() != QDialog::Accepted )
    {
        return;
    }

    // DUT name (if any) in the file name, keeping only file-name-safe characters
    QString dut = dialog.dutName();
    dut.replace(QRegularExpression("[^A-Za-z0-9_.-]+"), "_");
    QString defaultName = "polarSA_" + (dut.isEmpty() ? QString() : dut + "_")
                          + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".csv";
    QString path = QFileDialog::getSaveFileName(this, "Export Data",
                                                QDir(lastDir("export")).filePath(defaultName),
                                                "CSV files (*.csv);;All files (*)");
    if ( path.isEmpty() )
    {
        return;
    }
    rememberDir("export", path);

    Measurement m;
    m.metadata = dialog.metadata();
    if ( !currentConfigPath.isEmpty() )
    {
        m.metadata.append("config," + configDisplayName());
    }
    m.metadata += levelsMetadata();
    m.metadata += captureMetadata;
    m.angles = captureAngles;
    m.spectra = powerSpectra;
    m.angleOffsetDeg = liveTrace.angleOffsetDeg;
    m.gainOffsetDb = liveTrace.gainOffsetDb;
    applyAdjustments(m);

    QString error;
    if ( !writeMeasurementCsv(path, m, &error) )
    {
        QMessageBox::warning(this, "Export Data", "Export failed:\n" + error);
        return;
    }
    ui->statusbar->showMessage("Exported " + QString::number(m.spectra.length()) + " spectra to " + path);
}

void MainWindow::restoreTinySALevels(Measurement &m)
{
    // Files written by earlier polarSA builds stored the levels with the external gain already removed.
    // The external gain is now a display correction: add it back so the data are tinySA input levels.
    if ( !m.metadata.contains("data_unit,dBm (analyzer input level minus external gain)") )
    {
        return;
    }
    double extGain = 0;
    for( const QString &line : m.metadata )
    {
        if ( line.startsWith("ext_gain_dB,") )
        {
            extGain = line.mid(12).toDouble();
        }
    }
    if ( extGain == 0 )
    {
        return;
    }
    for( PowerSpectrum &ps : m.spectra )
    {
        for( float &p : ps.power )
        {
            p += static_cast<float>(extGain);
        }
    }
    m.metadata.append("import_note,external gain " + QString::number(extGain, 'f', 1)
                      + " dB added back: levels are tinySA input levels (set Ext. gain to apply it)");
}

void MainWindow::importData()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, "Import Data", lastDir("import"),
                                                      "polarSA data (*.csv *.pdf);;CSV files (*.csv);;"
                                                      "PDF reports with embedded data (*.pdf);;All files (*)");
    if ( paths.isEmpty() )
    {
        return;
    }
    rememberDir("import", paths.first());

    QStringList errors;
    QList<Measurement> loaded;
    for( const QString &path : paths )
    {
        loaded += readMeasurementFile(path, &errors);
    }

    int nbImported = 0;
    for( Measurement &m : loaded )
    {
        restoreTinySALevels(m);
        m.color = nextImportColor();
        importedMeasurements.append(m);
        nbImported++;

        const QVector<float> &freq = m.spectra.first().freq;
        QListWidgetItem *item = new QListWidgetItem(colorSwatch(m.color), m.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setToolTip(m.sourcePath + "\n"
                         + QString::number(1e-6 * freq.first(), 'f', 3) + " - "
                         + QString::number(1e-6 * freq.last(), 'f', 3) + " MHz, "
                         + QString::number(freq.length()) + " points, "
                         + QString::number(m.angles.length()) + " angles"
                         + (m.metadata.isEmpty() ? QString() : "\n" + m.metadata.join("\n")));
        ui->lwImports->blockSignals(true);
        item->setCheckState(Qt::Checked);
        ui->lwImports->addItem(item);
        ui->lwImports->blockSignals(false);
    }

    updateFrequencyRange();
    updateRadiationPattern();
    updateSpectrumPlots();
    ui->statusbar->showMessage("Imported " + QString::number(nbImported) + " trace(s).");

    if ( !errors.isEmpty() )
    {
        QMessageBox::warning(this, "Import Data", "Some files could not be imported:\n\n" + errors.join("\n"));
    }
}

void MainWindow::clearImports()
{
    importedMeasurements.clear();
    importCount = 0;
    while ( ui->lwImports->count() > 1 )
    {
        delete ui->lwImports->takeItem(1); // keep row 0, the live capture
    }
    ui->powerPlot->setOverlayPatterns({}, {});
    updateFrequencyRange();
    updateRadiationPattern();
    updateSpectrumPlots();
}

/*
 * CSV layout (comma separated, '.' decimal point):
 *   # polarSA measurement export
 *   # key,value                    <- optional metadata lines
 *   angle_deg/freq_Hz,f0,f1,...    <- header: frequencies in Hz
 *   0,p0,p1,...                    <- one row per angle: power in dBm
 *   10,p0,p1,...
 * Import also accepts ';' or tab separators (',' decimal mark is then allowed).
 * PDF reports can embed the same CSV, one attachment per trace.
 */
void MainWindow::applyAdjustments(Measurement &m)
{
    // The file holds what is displayed: rotation / gain adjustments of the trace are applied
    if ( m.angleOffsetDeg == 0 && m.gainOffsetDb == 0 )
    {
        return;
    }
    for( float &a : m.angles )
    {
        a += static_cast<float>(m.angleOffsetDeg);
    }
    for( PowerSpectrum &ps : m.spectra )
    {
        for( float &p : ps.power )
        {
            p += static_cast<float>(m.gainOffsetDb);
        }
    }
    m.metadata.append("applied_rotation_deg," + QString::number(m.angleOffsetDeg, 'f', 1));
    m.metadata.append("applied_gain_offset_dB," + QString::number(m.gainOffsetDb, 'f', 1));
    m.angleOffsetDeg = 0;
    m.gainOffsetDb = 0;
}

bool MainWindow::writeMeasurementCsv(const QString &path, const Measurement &m, QString *error)
{
    QFile file(path);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
    {
        *error = file.errorString();
        return false;
    }
    QByteArray csv = measurementCsv(m);
    if ( file.write(csv) != csv.size() )
    {
        *error = file.errorString();
        return false;
    }
    return true;
}

QByteArray MainWindow::measurementCsv(const Measurement &m)
{
    QByteArray csv;
    QTextStream out(&csv);
    out << "# polarSA measurement export\n";
    for( const QString &line : m.metadata )
    {
        out << "# " << line << "\n";
    }

    out << "angle_deg/freq_Hz";
    for( float f : m.spectra.first().freq )
    {
        out << "," << QString::number(static_cast<double>(f), 'f', 0);
    }
    out << "\n";

    for( int specId = 0; specId < m.spectra.length() && specId < m.angles.length(); specId++ )
    {
        out << QString::number(m.angles.at(specId));
        for( float p : m.spectra.at(specId).power )
        {
            out << "," << QString::number(p, 'f', 2);
        }
        out << "\n";
    }

    out.flush();
    return csv;
}

QList<Measurement> MainWindow::readMeasurementFile(const QString &path, QStringList *errors)
{
    QFileInfo info(path);
    QList<Measurement> list;

    if ( info.suffix().compare("pdf", Qt::CaseInsensitive) == 0 )
    {
        // PDF report: one trace per embedded CSV
        QList<PdfAttachment> attachments;
        QString error;
        if ( !PdfAttachments::read(path, &attachments, &error) )
        {
            errors->append(info.fileName() + ": " + error);
            return list;
        }
        for( const PdfAttachment &a : attachments )
        {
            Measurement m;
            QTextStream in(a.data);
            if ( !parseMeasurementCsv(in, m, &error) )
            {
                errors->append(info.fileName() + " / " + a.name + ": " + error);
                continue;
            }
            m.name = a.name;
            m.sourcePath = QDir::toNativeSeparators(info.absoluteFilePath()) + " (embedded " + a.name + ")";
            list.append(m);
        }
        return list;
    }

    QFile file(path);
    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        errors->append(info.fileName() + ": " + file.errorString());
        return list;
    }
    Measurement m;
    QString error;
    QTextStream in(&file);
    if ( !parseMeasurementCsv(in, m, &error) )
    {
        errors->append(info.fileName() + ": " + error);
        return list;
    }
    m.name = info.fileName();
    m.sourcePath = info.absoluteFilePath();
    list.append(m);
    return list;
}

bool MainWindow::parseMeasurementCsv(QTextStream &in, Measurement &m, QString *error)
{
    QVector<float> freq;
    QChar sep;
    int lineNo = 0;
    while ( !in.atEnd() )
    {
        QString line = in.readLine().trimmed();
        lineNo++;
        if ( line.isEmpty() )
        {
            continue;
        }
        if ( line.startsWith('#') )
        {
            QString meta = line.mid(1).trimmed();
            if ( !meta.isEmpty() && meta != "polarSA measurement export" )
            {
                m.metadata.append(meta);
            }
            continue;
        }

        if ( sep.isNull() )
        {
            sep = line.contains(';') ? QChar(';') : line.contains('\t') ? QChar('\t') : QChar(',');
        }
        QStringList fields = line.split(sep);
        if ( sep != ',' )
        {
            for( QString &f : fields ) f.replace(',', '.');
        }
        while ( fields.length() > 1 && fields.last().trimmed().isEmpty() )
        {
            fields.removeLast(); // trailing separators, e.g. after a spreadsheet re-save
        }

        if ( freq.isEmpty() )
        {
            // Header line: first cell is a label, the others are frequencies in Hz
            for( int i = 1; i < fields.length(); i++ )
            {
                bool ok = false;
                double f = fields.at(i).trimmed().toDouble(&ok);
                if ( !ok )
                {
                    *error = QString("line %1: invalid frequency '%2'").arg(lineNo).arg(fields.at(i));
                    return false;
                }
                freq.append(static_cast<float>(f));
            }
            if ( freq.isEmpty() )
            {
                *error = QString("line %1: header has no frequency column").arg(lineNo);
                return false;
            }
            continue;
        }

        if ( fields.length() != freq.length() + 1 )
        {
            *error = QString("line %1: expected %2 values, got %3").arg(lineNo).arg(freq.length() + 1).arg(fields.length());
            return false;
        }

        bool ok = false;
        float angle = fields.at(0).trimmed().toFloat(&ok);
        if ( !ok )
        {
            *error = QString("line %1: invalid angle '%2'").arg(lineNo).arg(fields.at(0));
            return false;
        }

        PowerSpectrum ps;
        ps.freq = freq;
        ps.power.reserve(freq.length());
        for( int i = 1; i < fields.length(); i++ )
        {
            float p = fields.at(i).trimmed().toFloat(&ok);
            if ( !ok )
            {
                *error = QString("line %1: invalid power '%2'").arg(lineNo).arg(fields.at(i));
                return false;
            }
            ps.power.append(p);
        }
        m.angles.append(angle);
        m.spectra.append(ps);
    }

    if ( m.spectra.isEmpty() )
    {
        *error = "no data rows found";
        return false;
    }
    return true;
}
