#include "tracedialog.h"

#include <QColorDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <cmath>
#include "src/Corrections/corrections.h"

static QString degrees(double a)
{
    return QString::number(a, 'f', std::abs(a - std::round(a)) < 0.05 ? 0 : 1) + QChar(0x00B0);
}

static QString level(double v, const QString &unit)
{
    return QString::number(v, 'f', 1) + " " + Corrections::displayUnit(unit);
}

static QString signedDb(double v)
{
    return (v >= 0 ? "+" : "") + QString::number(v, 'f', 1) + " dB";
}

TraceDialog::TraceDialog(const Measurement &trace, bool isLive, double freqHz,
                         const PatternMetrics &reference, double correctionDb, const QString &unit, QWidget *parent)
    : QDialog(parent)
    , mTrace(trace)
    , mFreqHz(freqHz)
    , mCorrectionDb(correctionDb)
    , mUnit(unit)
    , mReference(reference)
    , mColor(trace.color)
{
    setWindowTitle("Trace - " + trace.name);
    mRawPattern = PatternAnalysis::patternAt(trace.spectra, trace.angles, freqHz);

    // --- Header: color + name
    mColorButton = new QPushButton;
    mColorButton->setToolTip("Change color");
    mColorButton->setFixedWidth(40);
    connect(mColorButton, &QPushButton::clicked, this, [this]() {
        QColor c = QColorDialog::getColor(mColor, this, "Trace color - " + mTrace.name);
        if (c.isValid())
        {
            setColor(c);
            emit previewChanged(angleOffset(), gainOffset(), mColor);
        }
    });
    QLabel *name = new QLabel("<b>" + trace.name.toHtmlEscaped() + "</b>");
    name->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QHBoxLayout *header = new QHBoxLayout;
    header->addWidget(mColorButton);
    header->addWidget(name, 1);

    // --- Adjustments
    mRotation = new QDoubleSpinBox;
    mRotation->setRange(-180.0, 180.0);
    mRotation->setDecimals(1);
    mRotation->setSingleStep(1.0);
    mRotation->setWrapping(true);
    mRotation->setSuffix(QString(QChar(0x00B0)));
    mRotation->setValue(trace.angleOffsetDeg);
    mRotation->setToolTip("Rotates the whole pattern around the center (added to every angle)");

    mGain = new QDoubleSpinBox;
    mGain->setRange(-100.0, 100.0);
    mGain->setDecimals(1);
    mGain->setSingleStep(0.5);
    mGain->setSuffix(" dB");
    mGain->setValue(trace.gainOffsetDb);
    mGain->setToolTip("Added to every value, e.g. to compensate a cable, an attenuator or a reference level");

    QPushButton *alignMax = new QPushButton("Max " + QString(QChar(0x2192)) + " 0" + QChar(0x00B0));
    alignMax->setToolTip("Rotate so that the maximum (at the selected frequency) points to 0" + QString(QChar(0x00B0)));
    connect(alignMax, &QPushButton::clicked, this, [this]() {
        PatternMetrics raw = PatternAnalysis::compute(mRawPattern);
        if (raw.valid)
        {
            double offset = PatternAnalysis::wrap360(-raw.maxAngleDeg);
            if (offset > 180.0)
                offset -= 360.0;
            mRotation->setValue(offset);
        }
    });
    QPushButton *reset = new QPushButton("Reset");
    connect(reset, &QPushButton::clicked, this, [this]() {
        mRotation->setValue(0);
        mGain->setValue(0);
    });

    QHBoxLayout *rotationRow = new QHBoxLayout;
    rotationRow->addWidget(mRotation, 1);
    rotationRow->addWidget(alignMax);
    QHBoxLayout *gainRow = new QHBoxLayout;
    gainRow->addWidget(mGain, 1);
    gainRow->addWidget(reset);

    QGroupBox *adjustBox = new QGroupBox("Adjustments");
    QFormLayout *adjustForm = new QFormLayout(adjustBox);
    adjustForm->addRow("Rotation", rotationRow);
    adjustForm->addRow("Gain offset", gainRow);

    // --- Analysis at the selected frequency
    auto value = []() {
        QLabel *l = new QLabel("-");
        l->setTextInteractionFlags(Qt::TextSelectableByMouse);
        return l;
    };
    mMax = value();
    mMin = value();
    mRipple = value();
    mAverage = value();
    mBeamwidth = value();
    mFrontToBack = value();
    mCoverage = value();
    mVsLive = value();

    QGroupBox *analysisBox = new QGroupBox("Analysis at " + QString::number(freqHz * 1e-6, 'f', 3) + " MHz");
    QFormLayout *analysisForm = new QFormLayout(analysisBox);
    analysisForm->addRow("Maximum", mMax);
    analysisForm->addRow("Minimum", mMin);
    analysisForm->addRow("Max " + QString(QChar(0x2212)) + " min", mRipple);
    analysisForm->addRow("Average (power)", mAverage);
    analysisForm->addRow(QString(QChar(0x2212)) + "3 dB beamwidth", mBeamwidth);
    analysisForm->addRow("Front-to-back", mFrontToBack);
    analysisForm->addRow("Coverage", mCoverage);
    if (!isLive)
        analysisForm->addRow("vs live capture", mVsLive);

    QWidget *analysisTab = new QWidget;
    QVBoxLayout *analysisLayout = new QVBoxLayout(analysisTab);
    analysisLayout->addWidget(adjustBox);
    analysisLayout->addWidget(analysisBox);
    analysisLayout->addStretch();

    // --- Info & metadata
    QFormLayout *infoForm = new QFormLayout;
    QString source = isLive ? QString("Live capture (not saved)") : trace.sourcePath;
    QLabel *sourceLabel = new QLabel(source.toHtmlEscaped());
    sourceLabel->setWordWrap(true);
    sourceLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoForm->addRow("Source", sourceLabel);
    if (!trace.spectra.isEmpty() && !trace.spectra.first().freq.isEmpty())
    {
        const QVector<float> &f = trace.spectra.first().freq;
        infoForm->addRow("Frequency range", new QLabel(QString::number(f.first() * 1e-6, 'f', 3) + " - "
                                                       + QString::number(f.last() * 1e-6, 'f', 3) + " MHz"));
        infoForm->addRow("Points", new QLabel(QString::number(f.length())));
    }
    infoForm->addRow("Angles", new QLabel(QString::number(trace.angles.length())
                                          + (trace.angles.isEmpty() ? QString()
                                             : "  (" + degrees(trace.angles.first()) + " " + QChar(0x2192) + " "
                                               + degrees(trace.angles.last()) + ")")));

    QPlainTextEdit *metadata = new QPlainTextEdit;
    metadata->setReadOnly(true);
    metadata->setPlainText(trace.metadata.isEmpty() ? QString("(no metadata)") : trace.metadata.join("\n"));

    QWidget *infoTab = new QWidget;
    QVBoxLayout *infoLayout = new QVBoxLayout(infoTab);
    infoLayout->addLayout(infoForm);
    infoLayout->addWidget(new QLabel("Metadata"));
    infoLayout->addWidget(metadata, 1);

    QTabWidget *tabs = new QTabWidget;
    tabs->addTab(analysisTab, "Analysis && adjustments");
    tabs->addTab(infoTab, "Info && metadata");

    // --- Buttons
    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (!isLive)
    {
        QPushButton *remove = buttons->addButton("Remove trace", QDialogButtonBox::DestructiveRole);
        connect(remove, &QPushButton::clicked, this, [this]() {
            mRemove = true;
            accept();
        });
    }

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(header);
    layout->addWidget(tabs, 1);
    layout->addWidget(buttons);

    setColor(mColor);
    auto onAdjust = [this]() {
        updateAnalysis();
        emit previewChanged(angleOffset(), gainOffset(), mColor);
    };
    connect(mRotation, &QDoubleSpinBox::valueChanged, this, onAdjust);
    connect(mGain, &QDoubleSpinBox::valueChanged, this, onAdjust);
    updateAnalysis();
    resize(440, 480);
}

double TraceDialog::angleOffset() const
{
    return mRotation->value();
}

double TraceDialog::gainOffset() const
{
    return mGain->value();
}

QColor TraceDialog::color() const
{
    return mColor;
}

void TraceDialog::setColor(const QColor &color)
{
    mColor = color;
    QPixmap swatch(24, 14);
    swatch.fill(color);
    mColorButton->setIcon(QIcon(swatch));
    mColorButton->setIconSize(swatch.size());
}

void TraceDialog::updateAnalysis()
{
    PatternMetrics m = PatternAnalysis::compute(PatternAnalysis::adjusted(mRawPattern, angleOffset(), gainOffset() + mCorrectionDb));
    if (!m.valid)
    {
        const QString na = "no data at this frequency";
        mMax->setText(na);
        for (QLabel *l : {mMin, mRipple, mAverage, mBeamwidth, mFrontToBack, mCoverage, mVsLive})
            l->setText("-");
        return;
    }

    mMax->setText(level(m.maxDb, mUnit) + " @ " + degrees(m.maxAngleDeg));
    mMin->setText(level(m.minDb, mUnit) + " @ " + degrees(m.minAngleDeg));
    mRipple->setText(QString::number(m.maxDb - m.minDb, 'f', 1) + " dB");
    mAverage->setText(level(m.averageDb, mUnit));

    if (m.omni)
        mBeamwidth->setText("omnidirectional (within 3 dB)");
    else if (m.beamwidth3dB >= 0)
        mBeamwidth->setText(degrees(m.beamwidth3dB));
    else
        mBeamwidth->setText("not determined (pattern does not drop 3 dB within the measured range)");
    mBeamwidth->setWordWrap(true);

    mFrontToBack->setText(m.hasFrontToBack ? QString::number(m.frontToBackDb, 'f', 1) + " dB"
                                           : QString("- (no sample 180") + QChar(0x00B0) + " from the max)");
    mCoverage->setText(QString::number(m.samples) + " angles, step " + degrees(m.stepDeg)
                       + (m.fullCircle ? ", full circle" : ", partial"));

    if (mReference.valid)
    {
        mVsLive->setText("max " + signedDb(m.maxDb - mReference.maxDb)
                         + ", average " + signedDb(m.averageDb - mReference.averageDb));
    }
    else
    {
        mVsLive->setText("- (no live capture at this frequency)");
    }
}
