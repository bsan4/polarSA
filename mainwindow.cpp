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
#include <QSignalBlocker>
#include <QEventLoop>
#include <QTimer>
#include <algorithm>
#include "src/ExportDialog/exportdialog.h"
#include <cmath>
#include <limits>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("polarSA - Beta 1.0.0");
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
    connect( ui->pbExportData, &QPushButton::clicked, this, &MainWindow::exportData );
    connect( ui->pbImportData, &QPushButton::clicked, this, &MainWindow::importData );
    connect( ui->pbClearImports, &QPushButton::clicked, this, &MainWindow::clearImports );
    connect( ui->lwImports, &QListWidget::itemChanged, this, [this](QListWidgetItem *item){
        int row = ui->lwImports->row(item);
        bool checked = (item->checkState() == Qt::Checked);
        if ( row == 0 )
        {
            liveVisible = checked;
        }
        else if ( row - 1 < importedMeasurements.length() )
        {
            importedMeasurements[row - 1].visible = checked;
        }
        updateRadiationPattern();
    });
    connect( ui->lwImports, &QListWidget::itemDoubleClicked, this, &MainWindow::changeTraceColor );

    // Row 0 of the trace list is always the live capture
    liveColor = qApp->palette().highlight().color();
    QListWidgetItem *liveItem = new QListWidgetItem(colorSwatch(liveColor), "Live capture");
    liveItem->setFlags(liveItem->flags() | Qt::ItemIsUserCheckable);
    liveItem->setCheckState(Qt::Checked);
    liveItem->setToolTip("Current sequence capture");
    ui->lwImports->addItem(liveItem);

    // Splitters: visible handles, plots get the extra space, double-click toggles Max Emissions
    const QString handleStyle = "QSplitter::handle { background-color: palette(mid); margin: 2px; border-radius: 1px; }"
                                "QSplitter::handle:hover { background-color: palette(highlight); }";
    ui->rpSplitter->setStyleSheet(handleStyle);
    ui->rpSplitter->setHandleWidth(7);
    ui->rpSplitter->handle(1)->installEventFilter(this);
    ui->rpSplitter->handle(1)->setToolTip(ui->rpSplitter->toolTip());
    ui->rpSplitter->setToolTip(QString());
    ui->mainSplitter->setStyleSheet(handleStyle);
    ui->mainSplitter->setHandleWidth(7);
    ui->mainSplitter->setStretchFactor(0, 1);
    ui->mainSplitter->setStretchFactor(1, 0);

    QSettings settings;
    lastDataDir = settings.value("lastDataDir").toString();
    if ( !ui->rpSplitter->restoreState(settings.value("rpSplitter").toByteArray()) )
    {
        ui->rpSplitter->setSizes({650, 350});
    }
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

    setCaptureRunning(false);
    ui->powerPlot->setNormalize(false);
}

MainWindow::~MainWindow()
{
    QSettings settings;
    settings.setValue("lastDataDir", lastDataDir);
    settings.setValue("rpSplitter", ui->rpSplitter->saveState());
    settings.setValue("mainSplitter", ui->mainSplitter->saveState());
    delete ui;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    // Double-click on the radiation pattern / max emissions handle: hide or show Max Emissions
    if ( watched == ui->rpSplitter->handle(1) && event->type() == QEvent::MouseButtonDblClick )
    {
        QList<int> sizes = ui->rpSplitter->sizes();
        if ( sizes.at(0) == 0 || sizes.at(1) == 0 )
        {
            if ( rpSplitterSizes.length() != 2 || rpSplitterSizes.contains(0) )
            {
                int total = sizes.at(0) + sizes.at(1);
                rpSplitterSizes = { total * 65 / 100, total * 35 / 100 };
            }
            ui->rpSplitter->setSizes(rpSplitterSizes);
        }
        else
        {
            rpSplitterSizes = sizes;
            ui->rpSplitter->setSizes({ sizes.at(0) + sizes.at(1), 0 });
        }
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
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
    setCaptureRunning(true);
    sendSAParameters();
    sendRotatorParameters();
    return true;
}

void MainWindow::endCapture(const QString &message)
{
    setCaptureRunning(false);
    ui->statusbar->showMessage( devicesReady() ? message : "Stopped: device disconnected." );
}

void MainWindow::waitMs(int ms)
{
    // Keeps the UI (and the Stop button) responsive while waiting
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
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
        ui->spectrumPlot->setPowerSpectrum(ps);
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
            ui->spectrumPlot->setPowerSpectrum(ps);
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

    PowerSpectrum maxPs;
    powerSpectra.clear();
    captureAngles.clear();

    captureMetadata = QStringList{
        "date," + QDateTime::currentDateTime().toString(Qt::ISODate),
        "center_MHz," + QString::number(ui->sbSACenterFreq->value(), 'f', 4),
        "span_MHz," + QString::number(ui->sbSASpan->value(), 'f', 4),
        "rbw," + ui->cbRBW->currentText(),
        "attenuation_dB," + QString::number(ui->sbSAAtt->value()),
        "lna," + QString(ui->sbSALNA->isChecked() ? "on" : "off"),
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
        rotator.setAngle(a);
        waitMs(100);
        PowerSpectrum ps = captureSpectrum();
        if (ps.power.length() > 0)
        {
            ui->spectrumPlot->setPowerSpectrum(ps);
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
            if ( maxPs.power.isEmpty() ) maxPs = ps;
            for( int k = 0; k < ps.power.count(); k++ )
            {
                if ( maxPs.power.at(k) < ps.power.at(k) )
                {
                    maxPs.power[k] = ps.power.at(k);
                }
            }
            ui->maxSpectrumPlot->setPowerSpectrum(maxPs);
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
        return;
    }

    double freqHz = selectedFrequency();
    ui->lbRPFreq->setText( QString::number(freqHz * 1e-6, 'f', 3) + " MHz");

    RadiationPattern rp = patternAtFrequency(powerSpectra, captureAngles, freqHz);

    // Max / min of the live pattern, with the angle where they occur
    if ( !rp.power.isEmpty() )
    {
        int iMax = 0;
        int iMin = 0;
        for( int i = 1; i < rp.power.length(); i++ )
        {
            if ( rp.power.at(i) > rp.power.at(iMax) ) iMax = i;
            if ( rp.power.at(i) < rp.power.at(iMin) ) iMin = i;
        }
        ui->lbRPMax->setText( QString::number(rp.power.at(iMax), 'f', 1) + " dBm @ " + QString::number(rp.angles.at(iMax)) + QChar(0x00B0) );
        ui->lbRPMin->setText( QString::number(rp.power.at(iMin), 'f', 1) + " dBm @ " + QString::number(rp.angles.at(iMin)) + QChar(0x00B0) );
    }
    else
    {
        ui->lbRPMax->setText("-");
        ui->lbRPMin->setText("-");
    }

    ui->powerPlot->setRadiationPattern( liveVisible ? rp : RadiationPattern() );

    QList<RadiationPattern> overlays;
    QList<QColor> colors;
    for( const Measurement &m : importedMeasurements )
    {
        overlays.append( m.visible ? patternAtFrequency(m.spectra, m.angles, freqHz) : RadiationPattern() );
        colors.append(m.color);
    }
    ui->powerPlot->setOverlayPatterns(overlays, colors);
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
    auto search = [&](const QList<PowerSpectrum> &spectra, const QVector<float> &angles, const QString &name) {
        for( int s = 0; s < spectra.length() && s < angles.length(); s++ )
        {
            const PowerSpectrum &ps = spectra.at(s);
            for( int i = 0; i < ps.power.length() && i < ps.freq.length(); i++ )
            {
                if ( !found || ps.power.at(i) > peakPower )
                {
                    found = true;
                    peakPower = ps.power.at(i);
                    peakFreq = ps.freq.at(i);
                    peakAngle = angles.at(s);
                    peakTrace = name;
                }
            }
        }
    };
    if ( liveVisible )
    {
        search(powerSpectra, captureAngles, "live capture");
    }
    for( const Measurement &m : importedMeasurements )
    {
        if ( m.visible )
        {
            search(m.spectra, m.angles, m.name);
        }
    }

    if ( !found )
    {
        ui->statusbar->showMessage("No visible trace to search.");
        return;
    }
    setSelectedFrequency(peakFreq);
    ui->statusbar->showMessage("Peak: " + QString::number(peakPower, 'f', 1) + " dBm at "
                               + QString::number(peakFreq * 1e-6, 'f', 3) + " MHz, "
                               + QString::number(peakAngle) + QChar(0x00B0) + " (" + peakTrace + ")");
}

RadiationPattern MainWindow::patternAtFrequency(const QList<PowerSpectrum> &spectra, const QVector<float> &angles, double freqHz)
{
    RadiationPattern rp;
    if ( spectra.isEmpty() || spectra.first().freq.isEmpty() )
    {
        return rp;
    }
    const QVector<float> &freq = spectra.first().freq;

    // Nearest frequency bin. A single-frequency measurement is always shown.
    int bestId = 0;
    double bestDist = (std::numeric_limits<double>::max)(); // parentheses: avoid the windows.h max() macro
    for( int i = 0; i < freq.length(); i++ )
    {
        double d = std::abs(freq.at(i) - freqHz);
        if ( d < bestDist )
        {
            bestDist = d;
            bestId = i;
        }
    }
    if ( freq.length() > 1 )
    {
        double halfBin = 0.5 * std::abs(freq.at(1) - freq.at(0));
        if ( bestDist > halfBin )
        {
            return rp; // requested frequency is outside of this trace
        }
    }

    for( int specId = 0; specId < spectra.length() && specId < angles.length(); specId++ )
    {
        if ( bestId < spectra.at(specId).power.length() )
        {
            rp.angles.push_back(angles.at(specId));
            rp.power.push_back(spectra.at(specId).power.at(bestId));
        }
    }
    return rp;
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

void MainWindow::changeTraceColor(QListWidgetItem *item)
{
    int row = ui->lwImports->row(item);
    if ( row < 0 || row - 1 >= importedMeasurements.length() )
    {
        return;
    }

    QColor current = (row == 0) ? liveColor : importedMeasurements.at(row - 1).color;
    QColor color = QColorDialog::getColor(current, this, "Trace color - " + item->text());
    if ( !color.isValid() )
    {
        return; // cancelled
    }

    item->setIcon(colorSwatch(color));
    if ( row == 0 )
    {
        liveColor = color;
        ui->powerPlot->setLiveColor(color);
    }
    else
    {
        importedMeasurements[row - 1].color = color;
        updateRadiationPattern();
    }
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
                                                QDir(lastDataDir).filePath(defaultName),
                                                "CSV files (*.csv);;All files (*)");
    if ( path.isEmpty() )
    {
        return;
    }
    lastDataDir = QFileInfo(path).absolutePath();

    Measurement m;
    m.metadata = dialog.metadata() + captureMetadata;
    m.angles = captureAngles;
    m.spectra = powerSpectra;

    QString error;
    if ( !writeMeasurementCsv(path, m, &error) )
    {
        QMessageBox::warning(this, "Export Data", "Export failed:\n" + error);
        return;
    }
    ui->statusbar->showMessage("Exported " + QString::number(m.spectra.length()) + " spectra to " + path);
}

void MainWindow::importData()
{
    QStringList paths = QFileDialog::getOpenFileNames(this, "Import Data", lastDataDir,
                                                      "CSV files (*.csv);;All files (*)");
    if ( paths.isEmpty() )
    {
        return;
    }
    lastDataDir = QFileInfo(paths.first()).absolutePath();

    QStringList errors;
    int nbImported = 0;
    for( const QString &path : paths )
    {
        Measurement m;
        QString error;
        if ( !readMeasurementCsv(path, m, &error) )
        {
            errors.append(QFileInfo(path).fileName() + ": " + error);
            continue;
        }
        m.name = QFileInfo(path).fileName();
        m.color = nextImportColor();
        importedMeasurements.append(m);
        nbImported++;

        const QVector<float> &freq = m.spectra.first().freq;
        QListWidgetItem *item = new QListWidgetItem(colorSwatch(m.color), m.name);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setToolTip(QFileInfo(path).absoluteFilePath() + "\n"
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
    ui->statusbar->showMessage("Imported " + QString::number(nbImported) + " file(s).");

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
}

/*
 * CSV layout (comma separated, '.' decimal point):
 *   # polarSA measurement export
 *   # key,value                    <- optional metadata lines
 *   angle_deg/freq_Hz,f0,f1,...    <- header: frequencies in Hz
 *   0,p0,p1,...                    <- one row per angle: power in dBm
 *   10,p0,p1,...
 * Import also accepts ';' or tab separators (',' decimal mark is then allowed).
 */
bool MainWindow::writeMeasurementCsv(const QString &path, const Measurement &m, QString *error)
{
    QFile file(path);
    if ( !file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate) )
    {
        *error = file.errorString();
        return false;
    }

    QTextStream out(&file);
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
    if ( out.status() != QTextStream::Ok )
    {
        *error = "write error";
        return false;
    }
    return true;
}

bool MainWindow::readMeasurementCsv(const QString &path, Measurement &m, QString *error)
{
    QFile file(path);
    if ( !file.open(QIODevice::ReadOnly | QIODevice::Text) )
    {
        *error = file.errorString();
        return false;
    }

    QTextStream in(&file);
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
