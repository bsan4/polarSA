#ifndef DEFINES_H
#define DEFINES_H

#include <QVector>
#include <QList>
#include <QString>
#include <QStringList>
#include <QColor>

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


// A full sequence capture: one spectrum per angle (same frequency grid for all)
class Measurement
{
public:
    QString name;
    QColor color;
    bool visible = true;
    QStringList metadata;           // free "key,value" lines, informative only
    QVector<float> angles;          // degrees, one per spectrum
    QList<PowerSpectrum> spectra;
};


#endif
