#ifndef CORRECTIONS_H
#define CORRECTIONS_H

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVector>

// Setup calibration: correction (dB) added to the measured level (dBm), function of frequency.
// Typically 107 + antenna factor + cable loss, which turns dBm into dBuV/m.
struct CalibrationTable
{
    QString name;               // file name
    QString path;
    QString unit = "dB";        // unit of the corrected levels, e.g. dBuV/m
    QString description;
    QVector<double> freqHz;     // sorted, increasing
    QVector<double> correctionDb;

    bool isValid() const { return !freqHz.isEmpty(); }
    double at(double hz) const;         // linear interpolation, held constant outside of the table
    bool covers(double hz) const;
};

// One EMC limit: straight segments between (start frequency, start level) and (stop frequency, stop level)
struct LimitSegment
{
    double startHz = 0;
    double stopHz = 0;
    double startLevel = 0;
    double stopLevel = 0;
};

struct LimitLine
{
    QString name;               // e.g. "Quasi-peak", "Average"
    QColor color;
    QList<LimitSegment> segments;

    // Limit at a frequency (interpolated on sloped segments). At a boundary between two
    // segments the lower (stricter) value applies. False when no segment covers the frequency.
    bool valueAt(double hz, double *level) const;
};

// A limits file: one regulation, possibly several limit lines (detectors, classes...)
struct LimitSet
{
    QString name;               // regulation name, from the file ("# name,...") or the file name
    QString path;
    QString unit = "dBuV/m";
    QString description;
    QList<LimitLine> lines;

    bool isEmpty() const { return lines.isEmpty(); }
};

namespace Corrections
{
    bool loadCalibration(const QString &path, CalibrationTable &table, QString *error);
    bool loadLimits(const QString &path, LimitSet &limits, QString *error);

    // Unit comparison tolerant to "dBuV/m" / "dBµV/m" and case
    bool sameUnit(const QString &a, const QString &b);
    QString displayUnit(const QString &unit);   // "dBuV/m" -> "dBµV/m"
}

#endif
