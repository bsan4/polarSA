#ifndef SPECTRUMPLOT_H
#define SPECTRUMPLOT_H

#include <QObject>
#include "inc/defines.h"
#include "lib/QCustomPlot/qcustomplot.h"
#include "src/Corrections/corrections.h"

class SpectrumPlot : public QCustomPlot
{
    Q_OBJECT

public:
    SpectrumPlot(QWidget *parent = nullptr);
    ~SpectrumPlot();
    void setPowerSpectrum(const PowerSpectrum &spectrum);
    void clearSpectrum();
    void setTraceColor(const QColor &color);    // main trace; invalid = theme highlight

    // Extra spectra (e.g. max hold of imported measurements); empty spectra are not drawn.
    // markers: draw a dot on each point (sparse spectra such as quasi-peak windows).
    void setOverlaySpectra(const QList<PowerSpectrum> &spectra, const QList<QColor> &colors,
                           const QList<bool> &markers = QList<bool>());

    // EMC limit lines (dashed, with a legend). includeInScale: also fit the y axis on them.
    void setLimits(const LimitSet &limits, bool includeInScale);
    void setUnit(const QString &unit);          // y axis unit, e.g. "dBm" or "dBuV/m"

    // Autoscale on every data update (per axis); enabling an axis rescales it at once
    void setAutoscale(bool x, bool y);
    // One-time rescale of the chosen axes. Y fits the data inside the displayed X range.
    void rescale(bool x, bool y);
    void setPrintTheme(bool on);                // light colors for PDF / print rendering

private:
    void initPlot();
    void applyTheme();
    void rescaleToData(bool x, bool y);

    bool mPrintTheme = false;
    QColor mTraceColor;
    QList<QCPGraph*> mOverlayGraphs;
    QList<QCPGraph*> mLimitGraphs;
    LimitSet mLimits;
    bool mLimitsInScale = false;
    bool mAutoX = true;
    bool mAutoY = true;
};


#endif
