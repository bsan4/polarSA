#ifndef PATTERNANALYSIS_H
#define PATTERNANALYSIS_H

#include <QList>
#include <QVector>
#include "inc/defines.h"

// Figures of merit of one radiation pattern (one frequency, power vs angle)
struct PatternMetrics
{
    bool valid = false;
    int samples = 0;            // distinct angles
    bool fullCircle = false;    // angles cover 360 degrees without a gap larger than ~1.5 steps
    double stepDeg = 0;         // typical angular step

    double maxDb = 0;
    double maxAngleDeg = 0;
    double minDb = 0;
    double minAngleDeg = 0;
    double averageDb = 0;       // power average over the angles (linear mean, in dB)

    double beamwidth3dB = -1;   // degrees; -1 when it cannot be determined
    bool omni = false;          // never drops 3 dB below the max over the full circle
    double frontToBackDb = -1;  // max minus the value 180 degrees away; -1 when no sample there
    bool hasFrontToBack = false;
};

namespace PatternAnalysis
{
    // Pattern of a capture at one frequency (nearest bin). Empty if freqHz is outside of the capture.
    RadiationPattern patternAt(const QList<PowerSpectrum> &spectra, const QVector<float> &angles, double freqHz);

    // Rotates (degrees, result wrapped to [0, 360)) and offsets (dB) a pattern
    RadiationPattern adjusted(const RadiationPattern &pattern, double angleOffsetDeg, double gainOffsetDb);

    PatternMetrics compute(const RadiationPattern &pattern);

    double wrap360(double angleDeg);
}

#endif
