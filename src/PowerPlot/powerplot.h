
#ifndef POWERPLOT_H
#define POWERPLOT_H

#include <QObject>
#include "inc/defines.h"
#include "lib/QCustomPlot/qcustomplot.h"

class PowerPlot : public QCustomPlot
{
    Q_OBJECT

public:
    PowerPlot();
    ~PowerPlot();
    void setPowerSpectrum(PowerSpectrum &spectrum);

private:

};


#endif
