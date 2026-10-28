#include "quasipeakdialog.h"

#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

static QDoubleSpinBox *mhzBox()
{
    QDoubleSpinBox *box = new QDoubleSpinBox;
    box->setDecimals(4);
    box->setRange(0.0, 10000.0);
    box->setSuffix(" MHz");
    box->setSingleStep(0.1);
    return box;
}

QuasiPeakDialog::QuasiPeakDialog(double centerHz, const QString &anglesText, const QStringList &measuredWindows,
                                 QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Quasi-Peak Measurement");

    QSettings settings;
    settings.beginGroup("quasiPeak");

    mCenter = mhzBox();
    mSpan = mhzBox();
    mStart = mhzBox();
    mStop = mhzBox();
    mCenter->setValue(centerHz * 1e-6);
    mSpan->setValue(settings.value("spanMHz", 1.0).toDouble());
    mSpan->setMinimum(0.0001);

    mPoints = new QSpinBox;
    mPoints->setRange(2, 450);
    mPoints->setValue(settings.value("points", 51).toInt());
    mPoints->setToolTip("Points of the window (tinySA Ultra: 450 max)");

    mSweeps = new QSpinBox;
    mSweeps->setRange(1, 200);
    mSweeps->setValue(settings.value("sweeps", 20).toInt());
    mSweeps->setToolTip("Quasi-peak sweeps at each angle; the detector restarts from a raw sweep at each angle "
                        "and the last sweep is kept. More sweeps = closer to the settled quasi-peak value.");
    settings.endGroup();

    fromCenterSpan();
    connect(mCenter, &QDoubleSpinBox::valueChanged, this, &QuasiPeakDialog::fromCenterSpan);
    connect(mSpan, &QDoubleSpinBox::valueChanged, this, &QuasiPeakDialog::fromCenterSpan);
    connect(mStart, &QDoubleSpinBox::valueChanged, this, &QuasiPeakDialog::fromStartStop);
    connect(mStop, &QDoubleSpinBox::valueChanged, this, &QuasiPeakDialog::fromStartStop);

    QGroupBox *windowBox = new QGroupBox("Frequency window");
    QFormLayout *windowForm = new QFormLayout(windowBox);
    windowForm->addRow("Center", mCenter);
    windowForm->addRow("Span", mSpan);
    windowForm->addRow("Start", mStart);
    windowForm->addRow("Stop", mStop);
    windowForm->addRow("Points", mPoints);
    windowForm->addRow("Sweeps per angle", mSweeps);

    QLabel *info = new QLabel(
        "Measured at each rotator angle: " + anglesText + ".<br>"
        "RBW, attenuation and LNA come from the Spectrum Analyzer panel "
        "(CISPR 16 uses 120 kHz from 30 MHz to 1 GHz: the closest tinySA RBW is 100 kHz).<br>"
        "The result is added to the <b>Quasi-peak</b> trace; a window overlapping a measured one replaces its points.<br>"
        "<i>tinySA quasi-peak: trace calculation with instant charge and slow discharge over successive sweeps, "
        "an approximation of a CISPR 16 quasi-peak detector.</i>");
    info->setWordWrap(true);
    info->setTextFormat(Qt::RichText);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(windowBox);
    layout->addWidget(info);
    if (!measuredWindows.isEmpty())
    {
        QLabel *measured = new QLabel("<b>Already measured:</b><br>" + measuredWindows.join("<br>"));
        measured->setWordWrap(true);
        measured->setTextFormat(Qt::RichText);
        layout->addWidget(measured);
    }

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText("Start");
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() { saveFields(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    resize(460, sizeHint().height());
    mCenter->setFocus();
    mCenter->selectAll();
}

void QuasiPeakDialog::fromCenterSpan()
{
    if (mUpdating)
        return;
    mUpdating = true;
    mStart->setValue(qMax(0.0, mCenter->value() - mSpan->value() / 2));
    mStop->setValue(mCenter->value() + mSpan->value() / 2);
    mUpdating = false;
}

void QuasiPeakDialog::fromStartStop()
{
    if (mUpdating)
        return;
    mUpdating = true;
    if (mStop->value() <= mStart->value())
        mStop->setValue(mStart->value() + 0.0001);
    mCenter->setValue((mStart->value() + mStop->value()) / 2);
    mSpan->setValue(mStop->value() - mStart->value());
    mUpdating = false;
}

QuasiPeakWindow QuasiPeakDialog::window() const
{
    QuasiPeakWindow w;
    w.startHz = mStart->value() * 1e6;
    w.stopHz = mStop->value() * 1e6;
    w.points = mPoints->value();
    w.sweeps = mSweeps->value();
    return w;
}

void QuasiPeakDialog::saveFields() const
{
    QSettings settings;
    settings.beginGroup("quasiPeak");
    settings.setValue("spanMHz", mSpan->value());
    settings.setValue("points", mPoints->value());
    settings.setValue("sweeps", mSweeps->value());
    settings.endGroup();
}
