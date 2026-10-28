#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QSet>
#include <QTextStream>
#include "src/tinySA/tinySA.h"
#include "src/Rotator/rotator.h"
#include "src/Analysis/patternanalysis.h"
#include "src/Report/pdfreport.h"

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
    bool moveRotator(int deg);
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
    QColor nextImportColor();
    Measurement liveMeasurement() const;
    static RadiationPattern tracePattern(const Measurement &m, double freqHz);
    Q_SLOT void openTraceDialog(QListWidgetItem *item);
    void applyTraceSettings(int row, double angleOffsetDeg, double gainOffsetDb, const QColor &color);
    void removeImport(int index);
    Q_SLOT void exportPdf();
    ReportData buildReportData(const QStringList &notes, bool embedData = false) const;
    void embedTraceData(ReportTrace &t, const Measurement &m, bool isLive,
                        const QStringList &notes, QSet<QString> &usedNames) const;
    Q_SLOT void saveConfig();
    Q_SLOT void loadConfig();
    bool writeConfigFile(const QString &path, QString *error);
    bool readConfigFile(const QString &path, QStringList *warnings, QString *error);
    void markConfigModified();
    void updateConfigLabel();
    QString configDisplayName() const;
    QString lastDir(const QString &key) const;
    static void rememberDir(const QString &key, const QString &filePath);
    static QIcon colorSwatch(const QColor &color);
    static void applyAdjustments(Measurement &m);
    static bool writeMeasurementCsv(const QString &path, const Measurement &m, QString *error);
    static QByteArray measurementCsv(const Measurement &m);
    static QList<Measurement> readMeasurementFile(const QString &path, QStringList *errors);
    static bool parseMeasurementCsv(QTextStream &in, Measurement &m, QString *error);
    Ui::MainWindow *ui;
    TinySA tinySA;
    Rotator rotator;
    bool stopCapture = false;
    QString captureError;                   // why the last capture stopped early, if not by the user
    bool captureRunning = false;
    bool saParamsDirty = false;             // SA settings edited since they were last sent
    QList<PowerSpectrum> powerSpectra;
    QVector<float> captureAngles;           // one per entry of powerSpectra
    QStringList captureMetadata;            // settings snapshot of the last sequence
    QVector<double> sliderFreqs;            // merged, sorted frequency grid of all traces (Hz)

    // ui->lwImports: row 0 is the live capture, row i+1 is importedMeasurements[i]
    Measurement liveTrace;                   // live capture display settings (data: powerSpectra / captureAngles)
    QList<Measurement> importedMeasurements;
    int importCount = 0;                     // drives the import color sequence

    QString currentConfigPath;               // session config file loaded / saved last
    bool configModified = false;
    bool loadingConfig = false;
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
