
#ifndef SPECTRUMPLOT_H
#define SPECTRUMPLOT_H

#include <QObject>
#include "inc/defines.h"
#include "lib/QCustomPlot/qcustomplot.h"

class SpectrumPlot : public QCustomPlot
{
    Q_OBJECT

public:
    SpectrumPlot(QWidget *parent = nullptr);
    ~SpectrumPlot();
    void setPowerSpectrum(const PowerSpectrum &spectrum);
    void clearSpectrum();

private:
    void initPlot();
};


#endif
