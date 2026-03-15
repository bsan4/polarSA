#ifndef DEFINES_H
#define DEFINES_H

#include <QVector>

class PowerSpectrum
{
public:
    QVector<float> power;
    QVector<float> freq;
};


class RadiationPattern
{
public:
    QVector<float> power;   // dBm or relative dB
    QVector<float> angles;  // degrees
};


#endif
