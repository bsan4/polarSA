#include "patternanalysis.h"

#include <QPair>
#include <algorithm>
#include <cmath>
#include <limits>

double PatternAnalysis::wrap360(double angleDeg)
{
    double a = std::fmod(angleDeg, 360.0);
    if (a < 0)
        a += 360.0;
    if (a >= 360.0 - 1e-9)
        a = 0.0;
    return a;
}

RadiationPattern PatternAnalysis::patternAt(const QList<PowerSpectrum> &spectra, const QVector<float> &angles, double freqHz)
{
    RadiationPattern rp;
    if (spectra.isEmpty() || spectra.first().freq.isEmpty())
        return rp;
    const QVector<float> &freq = spectra.first().freq;

    // Nearest frequency bin. A single-frequency measurement is always shown.
    int bestId = 0;
    double bestDist = (std::numeric_limits<double>::max)();
    for (int i = 0; i < freq.length(); ++i)
    {
        double d = std::abs(freq.at(i) - freqHz);
        if (d < bestDist)
        {
            bestDist = d;
            bestId = i;
        }
    }
    if (freq.length() > 1)
    {
        double halfBin = 0.5 * std::abs(freq.at(1) - freq.at(0));
        if (bestDist > halfBin)
            return rp; // requested frequency is outside of this trace
    }

    for (int specId = 0; specId < spectra.length() && specId < angles.length(); ++specId)
    {
        if (bestId < spectra.at(specId).power.length())
        {
            rp.angles.push_back(angles.at(specId));
            rp.power.push_back(spectra.at(specId).power.at(bestId));
        }
    }
    return rp;
}

RadiationPattern PatternAnalysis::adjusted(const RadiationPattern &pattern, double angleOffsetDeg, double gainOffsetDb)
{
    RadiationPattern rp;
    const int n = qMin(pattern.angles.size(), pattern.power.size());
    rp.angles.reserve(n);
    rp.power.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        rp.angles.push_back(static_cast<float>(wrap360(pattern.angles[i] + angleOffsetDeg)));
        rp.power.push_back(static_cast<float>(pattern.power[i] + gainOffsetDb));
    }
    return rp;
}

PatternMetrics PatternAnalysis::compute(const RadiationPattern &pattern)
{
    PatternMetrics m;

    // Sorted (angle, power) samples, one per distinct angle (0 and 360 are the same direction)
    QVector<QPair<double, double>> s;
    const int n0 = qMin(pattern.angles.size(), pattern.power.size());
    for (int i = 0; i < n0; ++i)
        s.append({wrap360(pattern.angles[i]), pattern.power[i]});
    std::stable_sort(s.begin(), s.end(), [](const QPair<double, double> &a, const QPair<double, double> &b) {
        return a.first < b.first;
    });
    QVector<QPair<double, double>> u;
    for (const auto &p : s)
        if (u.isEmpty() || p.first - u.last().first > 1e-6)
            u.append(p);
    const int n = u.size();
    if (n == 0)
        return m;

    m.valid = true;
    m.samples = n;

    // Max, min, average
    int iMax = 0;
    int iMin = 0;
    double linSum = 0;
    for (int i = 0; i < n; ++i)
    {
        if (u[i].second > u[iMax].second) iMax = i;
        if (u[i].second < u[iMin].second) iMin = i;
        linSum += std::pow(10.0, u[i].second / 10.0);
    }
    m.maxDb = u[iMax].second;
    m.maxAngleDeg = u[iMax].first;
    m.minDb = u[iMin].second;
    m.minAngleDeg = u[iMin].first;
    m.averageDb = 10.0 * std::log10(linSum / n);

    if (n < 2)
        return m;

    // Angular coverage: typical step (median) and largest gap, wrap-around included
    QVector<double> diffs;
    for (int i = 1; i < n; ++i)
        diffs.append(u[i].first - u[i - 1].first);
    std::sort(diffs.begin(), diffs.end());
    m.stepDeg = diffs[diffs.size() / 2];
    double wrapGap = u[0].first + 360.0 - u[n - 1].first;
    m.fullCircle = wrapGap <= 1.5 * m.stepDeg + 1e-6;

    // -3 dB beamwidth: walk away from the max on both sides until the pattern drops below max - 3 dB,
    // interpolating the crossing linearly. Returns the angular distance from the max, or -1.
    const double threshold = m.maxDb - 3.0;
    auto walk = [&](int dir) -> double {
        double dist = 0;
        int prev = iMax;
        for (int k = 1; k < n; ++k)
        {
            int cur = iMax + dir * k;
            if (cur < 0 || cur >= n)
            {
                if (!m.fullCircle)
                    return -1;
                cur = (cur + n) % n;
            }
            double da = std::abs(u[cur].first - u[prev].first);
            if (da > 180.0)
                da = 360.0 - da; // across 0 / 360
            if (u[cur].second < threshold)
            {
                double t = (u[prev].second - threshold) / (u[prev].second - u[cur].second);
                return dist + t * da;
            }
            dist += da;
            prev = cur;
        }
        return -2; // went all the way around
    };
    double right = walk(+1);
    double left = walk(-1);
    if (right == -2 || left == -2)
    {
        m.omni = true;
        m.beamwidth3dB = 360.0;
    }
    else if (right >= 0 && left >= 0)
    {
        m.beamwidth3dB = qMin(360.0, right + left);
    }

    // Front-to-back: value of the sample closest to max + 180 degrees, if close enough
    double target = wrap360(m.maxAngleDeg + 180.0);
    int iBack = -1;
    double bestDist = 1e9;
    for (int i = 0; i < n; ++i)
    {
        double d = std::abs(u[i].first - target);
        d = qMin(d, 360.0 - d);
        if (d < bestDist)
        {
            bestDist = d;
            iBack = i;
        }
    }
    if (iBack >= 0 && bestDist <= qMax(0.51 * m.stepDeg, 0.5))
    {
        m.hasFrontToBack = true;
        m.frontToBackDb = m.maxDb - u[iBack].second;
    }

    return m;
}
