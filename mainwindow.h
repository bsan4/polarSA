#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include "src/tinySA/tinySA.h"
#include "src/Rotator/rotator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    Q_SLOT void triggerSingleCapture();
    Q_SLOT void triggerSequenceCapture();
    Q_SLOT void triggerLoopSingleCapture();
    Q_SLOT void triggerLoopSequenceCapture();
    Q_SLOT void triggerStopCapture();
    Q_SLOT void freqSliderMoved(int idChanged);
    Q_SLOT void exportData();
    Q_SLOT void importData();
    Q_SLOT void clearImports();
    bool devicesReady() const;
    void setCaptureRunning(bool running);
    bool startCapture();
    void endCapture(const QString &message);
    void waitMs(int ms);
    PowerSpectrum captureSpectrum();
    QVector<int> sequenceAngles();
    bool runSequence(bool reverse, const QString &label, int pass);
    void sendSAParameters();
    void sendRotatorParameters();
    void updateRadiationPattern();
    void updateFrequencyRange();
    int nearestSliderIndex(double freqHz) const;
    double selectedFrequency() const;
    void setSelectedFrequency(double freqHz);
    Q_SLOT void goToCenterFrequency();
    Q_SLOT void goToPeakFrequency();
    static RadiationPattern patternAtFrequency(const QList<PowerSpectrum> &spectra, const QVector<float> &angles, double freqHz);
    QColor nextImportColor();
    void changeTraceColor(QListWidgetItem *item);
    static QIcon colorSwatch(const QColor &color);
    static bool writeMeasurementCsv(const QString &path, const Measurement &m, QString *error);
    static bool readMeasurementCsv(const QString &path, Measurement &m, QString *error);
    Ui::MainWindow *ui;
    TinySA tinySA;
    Rotator rotator;
    bool stopCapture = false;
    bool captureRunning = false;
    bool saParamsDirty = false;             // SA settings edited since they were last sent
    QList<PowerSpectrum> powerSpectra;
    QVector<float> captureAngles;           // one per entry of powerSpectra
    QStringList captureMetadata;            // settings snapshot of the last sequence
    QVector<double> sliderFreqs;            // merged, sorted frequency grid of all traces (Hz)

    // ui->lwImports: row 0 is the live capture, row i+1 is importedMeasurements[i]
    QColor liveColor;
    bool liveVisible = true;
    QList<Measurement> importedMeasurements;
    int importCount = 0;                     // drives the import color sequence
    QString lastDataDir;
    QList<int> rpSplitterSizes;              // sizes to restore when un-hiding a graph

    const QList<QPair<QString, float>> rbwMap = {
        QPair<QString, float>("Auto", -1),
        QPair<QString, float>("200Hz", 0.2),
        QPair<QString, float>("1kHz", 1),
        QPair<QString, float>("3kHz", 3),
        QPair<QString, float>("10kHz", 10),
        QPair<QString, float>("30kHz", 30),
        QPair<QString, float>("100kHz", 100),
        QPair<QString, float>("600kHz", 600),
        QPair<QString, float>("850kHz", 850)
    };
};
#endif // MAINWINDOW_H
