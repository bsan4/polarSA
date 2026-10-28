#ifndef QUASIPEAKDIALOG_H
#define QUASIPEAKDIALOG_H

#include <QDialog>
#include <QStringList>

class QDoubleSpinBox;
class QSpinBox;

// Frequency window of a quasi-peak measurement
struct QuasiPeakWindow
{
    double startHz = 0;
    double stopHz = 0;
    int points = 51;
    int sweeps = 20;                // sweeps per angle, the last one is kept
};

// Asks for the frequency window (center / span or start / stop), points and sweeps of a
// quasi-peak measurement. Last values are remembered (QSettings), the center defaults to centerHz.
class QuasiPeakDialog : public QDialog
{
    Q_OBJECT

public:
    QuasiPeakDialog(double centerHz, const QString &anglesText, const QStringList &measuredWindows,
                    QWidget *parent = nullptr);

    QuasiPeakWindow window() const;

private:
    void fromCenterSpan();
    void fromStartStop();
    void saveFields() const;

    QDoubleSpinBox *mCenter = nullptr;
    QDoubleSpinBox *mSpan = nullptr;
    QDoubleSpinBox *mStart = nullptr;
    QDoubleSpinBox *mStop = nullptr;
    QSpinBox *mPoints = nullptr;
    QSpinBox *mSweeps = nullptr;
    bool mUpdating = false;
};

#endif
