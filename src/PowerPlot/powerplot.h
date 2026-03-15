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

    void setNormalize(bool en);
    void setRadialRange(double minVal, double maxVal);

private:
    void initPlot();
    void applyTheme();

private:
    QCPPolarAxisAngular *mAngularAxis = nullptr;
    QCPPolarAxisRadial  *mRadialAxis  = nullptr;
    QCPPolarGraph       *mGraph       = nullptr;

    bool mNormalize = true;
    double mManualMin = -40.0;
    double mManualMax = 0.0;
};

#endif
