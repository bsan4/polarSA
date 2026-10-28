#ifndef DEFINES_H
#define DEFINES_H

#include <QVector>
#include <QList>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QPalette>

#define POLARSA_VERSION "Beta 1.0.0"

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


// Light palette used when plots are rendered for a printed / PDF report
inline QPalette printPalette()
{
    QPalette pal;
    pal.setColor(QPalette::Window, Qt::white);
    pal.setColor(QPalette::WindowText, Qt::black);
    pal.setColor(QPalette::Mid, QColor(170, 170, 170));
    pal.setColor(QPalette::Highlight, QColor(31, 79, 191));
    return pal;
}


// A full sequence capture: one spectrum per angle (same frequency grid for all)
class Measurement
{
public:
    QString name;
    QString sourcePath;             // file it was imported from (empty for the live capture)
    QColor color;
    bool visible = true;
    double angleOffsetDeg = 0;      // display rotation added to every angle
    double gainOffsetDb = 0;        // display gain added to every power value
    QStringList metadata;           // free "key,value" lines, informative only
    QVector<float> angles;          // degrees, one per spectrum
    QList<PowerSpectrum> spectra;
};


#endif
