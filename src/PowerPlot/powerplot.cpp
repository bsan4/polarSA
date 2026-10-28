#include "powerplot.h"

#include <algorithm>
#include <cmath>

PowerPlot::PowerPlot(QWidget *parent)
    : QCustomPlot(parent)
{
    initPlot();
}

PowerPlot::~PowerPlot()
{
}

void PowerPlot::setLiveColor(const QColor &color)
{
    mLiveColor = color;
    applyTheme();
    replot();
}

void PowerPlot::setPrintTheme(bool on)
{
    mPrintTheme = on;
    applyTheme();
}

void PowerPlot::setNormalize(bool en)
{
    mNormalize = en;
}

void PowerPlot::setRadialRange(double minVal, double maxVal)
{
    mManualMin = minVal;
    mManualMax = maxVal;

    if (mRadialAxis)
        mRadialAxis->setRange(minVal, maxVal);

    replot();
}

void PowerPlot::applyTheme()
{
    if (!qApp)
        return;

    QPalette pal = mPrintTheme ? printPalette() : qApp->palette();

    const QColor bg   = pal.window().color();
    const QColor fg   = pal.windowText().color();
    const QColor mid  = pal.mid().color();
    const QColor high = pal.highlight().color();

    setBackground(bg);

    if (!mAngularAxis)
        return;

    if (mRadialAxis)
    {
        mRadialAxis->setBasePen(QPen(fg));
        mRadialAxis->setTickPen(QPen(fg));
        mRadialAxis->setSubTickPen(QPen(fg));
        mRadialAxis->setTickLabelColor(fg);
        mRadialAxis->setLabelColor(fg);
    }

    mAngularAxis->setBasePen(QPen(fg));
    mAngularAxis->setTickPen(QPen(fg));
    mAngularAxis->setSubTickPen(QPen(fg));
    mAngularAxis->setTickLabelColor(fg);
    mAngularAxis->setLabelColor(fg);

    if (mGraph)
    {
        mGraph->setPen(QPen(mLiveColor.isValid() ? mLiveColor : high, 2));
        mGraph->setBrush(Qt::NoBrush);
    }

    if (mAngularAxis->grid())
    {
        mAngularAxis->grid()->setAngularPen(QPen(mid, 1, Qt::DotLine));
        mAngularAxis->grid()->setRadialPen(QPen(mid, 1, Qt::DotLine));
        mAngularAxis->grid()->setAngularSubGridPen(QPen(mid, 1, Qt::DotLine));
        mAngularAxis->grid()->setRadialSubGridPen(QPen(mid, 1, Qt::DotLine));
        mAngularAxis->grid()->setRadialZeroLinePen(QPen(mid, 1, Qt::DotLine));
    }
}

void PowerPlot::initPlot()
{
    plotLayout()->clear();

    mAngularAxis = new QCPPolarAxisAngular(this);
    plotLayout()->addElement(0, 0, mAngularAxis);

    mRadialAxis = mAngularAxis->radialAxis();
    mGraph = new QCPPolarGraph(mAngularAxis, mRadialAxis);

    mGraph->setLineStyle(QCPPolarGraph::lsLine);
    mGraph->setScatterStyle(QCPScatterStyle::ssNone);
    mGraph->setPeriodic(true);

    mAngularAxis->setRange(0, 360);
    mAngularAxis->setAngle(0.0);
    mAngularAxis->setTickLabels(true);
    mAngularAxis->setTicks(true);
    mAngularAxis->setSubTicks(false);
    mAngularAxis->setNumberFormat("f");
    mAngularAxis->setNumberPrecision(0);
    mAngularAxis->setLabel("Angle (deg)");

    if (mRadialAxis)
    {
        mRadialAxis->setRange(mManualMin, mManualMax);
        mRadialAxis->setTicks(true);
        mRadialAxis->setSubTicks(true);
        mRadialAxis->setNumberFormat("f");
        mRadialAxis->setNumberPrecision(1);
        mRadialAxis->setLabel("Power (dB)");
    }

    if (mAngularAxis->grid())
    {
        mAngularAxis->grid()->setType(QCPPolarGrid::gtAll);
        mAngularAxis->grid()->setSubGridType(QCPPolarGrid::gtAll);
    }

    if (legend)
        legend->setVisible(false);

    applyTheme();
    replot();
}

void PowerPlot::clearPattern()
{
    if (mGraph)
        mGraph->data()->clear();

    updateRadialRange();
    replot();
}

void PowerPlot::setRadiationPattern(const RadiationPattern &pattern)
{
    if (!mGraph || !mAngularAxis || !mRadialAxis)
        return;

    fillGraph(mGraph, pattern);

    updateRadialRange();
    applyTheme();
    replot();
}

void PowerPlot::setOverlayPatterns(const QList<RadiationPattern> &patterns, const QList<QColor> &colors)
{
    if (!mAngularAxis || !mRadialAxis)
        return;

    while (mOverlayGraphs.size() > patterns.size())
        mAngularAxis->removeGraph(mOverlayGraphs.takeLast());

    while (mOverlayGraphs.size() < patterns.size())
    {
        QCPPolarGraph *g = new QCPPolarGraph(mAngularAxis, mRadialAxis);
        g->setLineStyle(QCPPolarGraph::lsLine);
        g->setScatterStyle(QCPScatterStyle::ssNone);
        g->setPeriodic(true);
        g->setBrush(Qt::NoBrush);
        mOverlayGraphs.append(g);
    }

    for (int i = 0; i < patterns.size(); ++i)
    {
        const QColor c = (i < colors.size()) ? colors[i] : QColor(Qt::gray);
        mOverlayGraphs[i]->setPen(QPen(c, 2));
        fillGraph(mOverlayGraphs[i], patterns[i]);
    }

    updateRadialRange();
    applyTheme();
    replot();
}

void PowerPlot::fillGraph(QCPPolarGraph *graph, const RadiationPattern &pattern)
{
    const int n = qMin(pattern.power.size(), pattern.angles.size());
    if (n <= 0)
    {
        graph->data()->clear();
        return;
    }

    double maxPower = pattern.power[0];
    for (int i = 1; i < n; ++i)
        if (pattern.power[i] > maxPower) maxPower = pattern.power[i];

    QVector<double> keys;
    QVector<double> values;
    keys.reserve(n);
    values.reserve(n);

    for (int i = 0; i < n; ++i)
    {
        keys.push_back(static_cast<double>(pattern.angles[i]));
        values.push_back(mNormalize
                             ? static_cast<double>(pattern.power[i]) - maxPower
                             : static_cast<double>(pattern.power[i]));
    }

    graph->setData(keys, values, false);
}

void PowerPlot::updateRadialRange()
{
    if (!mRadialAxis)
        return;

    // Range covering every non-empty trace (live + overlays)
    QList<QCPPolarGraph*> graphs = mOverlayGraphs;
    if (mGraph)
        graphs.prepend(mGraph);

    bool found = false;
    double lo = 0.0;
    double hi = 0.0;
    for (QCPPolarGraph *g : graphs)
    {
        for (auto it = g->data()->constBegin(); it != g->data()->constEnd(); ++it)
        {
            if (!found)
            {
                lo = hi = it->value;
                found = true;
            }
            lo = qMin(lo, it->value);
            hi = qMax(hi, it->value);
        }
    }

    if (!found)
        return;

    if (mNormalize)
    {
        if (lo > -3.0)
            lo = -3.0;

        mRadialAxis->setRange(std::floor(lo / 5.0) * 5.0, 0.0);
        mRadialAxis->setLabel("Relative Power (dB)");
    }
    else
    {
        if (std::abs(hi - lo) < 1e-6)
        {
            lo -= 1.0;
            hi += 1.0;
        }

        mRadialAxis->setRange(lo - 30, hi);
        mRadialAxis->setLabel("Power (dBm)");
    }
}
