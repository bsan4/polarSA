#ifndef TRACEDIALOG_H
#define TRACEDIALOG_H

#include <QDialog>
#include "inc/defines.h"
#include "src/Analysis/patternanalysis.h"

class QDoubleSpinBox;
class QLabel;
class QPushButton;

// Per-trace dialog: analysis at the selected frequency, rotation / gain adjustments,
// color, source info and metadata. Changes are previewed live through previewChanged().
class TraceDialog : public QDialog
{
    Q_OBJECT

public:
    // trace: settings + data of the trace. reference: metrics of the live capture to compare with
    // (invalid when there is none, or when the trace is the live capture itself).
    // correctionDb: setup calibration at freqHz, added to every value; unit: unit of the corrected values.
    TraceDialog(const Measurement &trace, bool isLive, double freqHz,
                const PatternMetrics &reference, double correctionDb = 0, const QString &unit = "dBm",
                QWidget *parent = nullptr);

    double angleOffset() const;
    double gainOffset() const;
    QColor color() const;
    bool removeRequested() const { return mRemove; }

signals:
    void previewChanged(double angleOffsetDeg, double gainOffsetDb, const QColor &color);

private:
    void updateAnalysis();
    void setColor(const QColor &color);

    Measurement mTrace;
    double mFreqHz = 0;
    double mCorrectionDb = 0;
    QString mUnit;
    PatternMetrics mReference;
    RadiationPattern mRawPattern;   // pattern at mFreqHz, before adjustments
    QColor mColor;
    bool mRemove = false;

    QPushButton *mColorButton = nullptr;
    QDoubleSpinBox *mRotation = nullptr;
    QDoubleSpinBox *mGain = nullptr;
    QLabel *mMax = nullptr;
    QLabel *mMin = nullptr;
    QLabel *mRipple = nullptr;
    QLabel *mAverage = nullptr;
    QLabel *mBeamwidth = nullptr;
    QLabel *mFrontToBack = nullptr;
    QLabel *mCoverage = nullptr;
    QLabel *mVsLive = nullptr;
};

#endif
