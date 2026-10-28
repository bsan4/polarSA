#ifndef POWERPLOT_H
#define POWERPLOT_H

#include <QWidget>
#include <QVector>
#include <QSharedPointer>
#include <QApplication>
#include <QPalette>

#include "inc/defines.h"
#include "lib/QCustomPlot/qcustomplot.h"

class PowerPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit PowerPlot(QWidget *parent = nullptr);
    ~PowerPlot();

    void setRadiationPattern(const RadiationPattern &pattern);
    void clearPattern();

    // Extra patterns (e.g. imported measurements) drawn on top of the live one.
    // An empty pattern in the list is simply not drawn.
    void setOverlayPatterns(const QList<RadiationPattern> &patterns, const QList<QColor> &colors);

    // EMC limits at the displayed frequency: one dashed circle per level
    void setLimitCircles(const QList<double> &levels, const QList<QColor> &colors);
    void setUnit(const QString &unit);  // radial axis unit in absolute mode, e.g. "dBm" or "dBuV/m"

    void setLiveColor(const QColor &color);
    void setPrintTheme(bool on);    // light colors for PDF / print rendering
    void setNormalize(bool en);
    void setRadialRange(double minVal, double maxVal);

private:
    void initPlot();
    void applyTheme();
    void fillGraph(QCPPolarGraph *graph, const RadiationPattern &pattern);
    void updateRadialRange();

private:
    QCPPolarAxisAngular *mAngularAxis = nullptr;
    QCPPolarAxisRadial  *mRadialAxis  = nullptr;
    QCPPolarGraph       *mGraph       = nullptr;
    QList<QCPPolarGraph*> mOverlayGraphs;
    QList<QCPPolarGraph*> mLimitGraphs;
    QString mUnit = "dBm";

    QColor mLiveColor;      // invalid = theme highlight color
    bool mPrintTheme = false;
    bool mNormalize = true;
    double mManualMin = -40.0;
    double mManualMax = 0.0;
};

#endif
