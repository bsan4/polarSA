#include "spectrumplot.h"
#include <QApplication>
#include <QPalette>
#include <cmath>
#include <algorithm>
#include <limits>

SpectrumPlot::SpectrumPlot(QWidget *parent)
    : QCustomPlot(parent)
{
    initPlot();
}

SpectrumPlot::~SpectrumPlot()
{
}

void SpectrumPlot::initPlot()
{
    addGraph();

    graph(0)->setLineStyle(QCPGraph::lsLine);
    graph(0)->setScatterStyle(QCPScatterStyle::ssNone);
    graph(0)->removeFromLegend();

    // --- MHz axis ticker ---
    auto ticker = QSharedPointer<QCPAxisTickerFixed>::create();
    ticker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
    ticker->setTickStep(1); // 1 MHz grid, but data will be ploted in unit of MHz
    xAxis->setTicker(ticker);

    xAxis->setNumberFormat("f");   // fixed
    xAxis->setNumberPrecision(3);  // 3 decimals

    xAxis->setLabel("Frequency (MHz)");
    yAxis->setLabel("Power (dBm)");

    // Legend: only used for the limit lines
    legend->setVisible(false);
    QFont legendFont = font();
    legendFont.setPointSizeF(qMax(6.0, legendFont.pointSizeF() - 1));
    legend->setFont(legendFont);
    axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop | Qt::AlignRight);

    setInteraction(QCP::iRangeDrag, true);
    setInteraction(QCP::iRangeZoom, true);
    setInteraction(QCP::iSelectPlottables, true);

    axisRect()->setupFullAxesBox();
    xAxis2->setVisible(true);
    yAxis2->setVisible(true);
    xAxis2->setTicks(false);
    yAxis2->setTicks(false);

    yAxis->setRange(-120, 20);
    xAxis->setRange(0, 1);

    applyTheme();

    // setOpenGl(true);
}

void SpectrumPlot::setPrintTheme(bool on)
{
    mPrintTheme = on;
    applyTheme();
}

void SpectrumPlot::setTraceColor(const QColor &color)
{
    mTraceColor = color;
    applyTheme();
    replot();
}

void SpectrumPlot::setUnit(const QString &unit)
{
    const bool power = Corrections::sameUnit(unit, "dBm");
    yAxis->setLabel(QString(power ? "Power" : "Level") + " (" + Corrections::displayUnit(unit) + ")");
    replot();
}

void SpectrumPlot::applyTheme()
{
    // --- use Qt theme colors (or light ones for printing) ---
    QPalette pal = mPrintTheme ? printPalette() : qApp->palette();

    QColor bg = pal.window().color();
    QColor fg = pal.windowText().color();
    QColor grid = pal.mid().color();

    setBackground(bg);

    for (QCPAxis *axis : {xAxis, yAxis, xAxis2, yAxis2})
    {
        axis->setBasePen(QPen(fg));
        axis->setTickPen(QPen(fg));
        axis->setSubTickPen(QPen(fg));
        axis->setTickLabelColor(fg);
        axis->setLabelColor(fg);
    }

    xAxis->grid()->setPen(QPen(grid, 1, Qt::DotLine));
    yAxis->grid()->setPen(QPen(grid, 1, Qt::DotLine));

    if (graphCount() > 0)
        graph(0)->setPen(QPen(mTraceColor.isValid() ? mTraceColor : pal.highlight().color(), 2));

    QColor legendBg = bg;
    legendBg.setAlpha(200);
    legend->setBrush(legendBg);
    legend->setBorderPen(QPen(grid));
    legend->setTextColor(fg);
}

void SpectrumPlot::clearSpectrum()
{
    if (graphCount() == 0)
        addGraph();

    graph(0)->data()->clear();
    replot();
}

static void fillGraph(QCPGraph *graph, const PowerSpectrum &spectrum)
{
    const int n = qMin(spectrum.freq.size(), spectrum.power.size());

    // Sparse spectra (several frequency windows): no line across a gap much larger than the typical spacing
    double gapLimit = 0;
    if (n > 2)
    {
        QVector<double> steps;
        for (int i = 1; i < n; ++i)
            steps.push_back(spectrum.freq[i] - spectrum.freq[i - 1]);
        std::sort(steps.begin(), steps.end());
        gapLimit = 3.0 * steps[steps.size() / 2];
    }

    QVector<double> x;
    QVector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        // convert Hz -> MHz
        if (i > 0 && gapLimit > 0 && spectrum.freq[i] - spectrum.freq[i - 1] > gapLimit)
        {
            x.push_back(0.5 * (spectrum.freq[i] + spectrum.freq[i - 1]) / 1e6);
            y.push_back(std::numeric_limits<double>::quiet_NaN());
        }
        x.push_back(static_cast<double>(spectrum.freq[i]) / 1e6);
        y.push_back(static_cast<double>(spectrum.power[i]));
    }
    graph->setData(x, y, true);
}

void SpectrumPlot::setPowerSpectrum(const PowerSpectrum &spectrum)
{
    if (graphCount() == 0)
        addGraph();

    fillGraph(graph(0), spectrum);
    rescaleToData(mAutoX, mAutoY);
    replot();
}

void SpectrumPlot::setOverlaySpectra(const QList<PowerSpectrum> &spectra, const QList<QColor> &colors,
                                     const QList<bool> &markers)
{
    while (mOverlayGraphs.size() > spectra.size())
        removeGraph(mOverlayGraphs.takeLast());
    while (mOverlayGraphs.size() < spectra.size())
    {
        QCPGraph *g = addGraph();
        g->setLineStyle(QCPGraph::lsLine);
        g->setScatterStyle(QCPScatterStyle::ssNone);
        g->removeFromLegend();
        mOverlayGraphs.append(g);
    }
    for (int i = 0; i < spectra.size(); ++i)
    {
        const QColor color = i < colors.size() ? colors[i] : QColor(Qt::gray);
        mOverlayGraphs[i]->setPen(QPen(color, 1.5));
        mOverlayGraphs[i]->setScatterStyle( markers.value(i) ? QCPScatterStyle(QCPScatterStyle::ssDisc, color, 5)
                                                             : QCPScatterStyle(QCPScatterStyle::ssNone) );
        fillGraph(mOverlayGraphs[i], spectra[i]);
    }
    rescaleToData(mAutoX, mAutoY);
    replot();
}

void SpectrumPlot::setLimits(const LimitSet &limits, bool includeInScale)
{
    for (QCPGraph *g : mLimitGraphs)
        removeGraph(g);
    mLimitGraphs.clear();
    mLimits = limits;
    mLimitsInScale = includeInScale;

    const QString unit = Corrections::displayUnit(limits.unit);
    for (const LimitLine &line : limits.lines)
    {
        // Segments joined in frequency order; a NaN point breaks the line where segments do not touch
        QVector<double> x;
        QVector<double> y;
        for (int i = 0; i < line.segments.size(); ++i)
        {
            const LimitSegment &s = line.segments[i];
            if (i > 0 && s.startHz > line.segments[i - 1].stopHz)
            {
                x << 0.5 * (s.startHz + line.segments[i - 1].stopHz) / 1e6;
                y << std::numeric_limits<double>::quiet_NaN();
            }
            x << s.startHz / 1e6 << s.stopHz / 1e6;
            y << s.startLevel << s.stopLevel;
        }
        QCPGraph *g = addGraph();
        g->setLineStyle(QCPGraph::lsLine);
        g->setScatterStyle(QCPScatterStyle::ssNone);
        g->setPen(QPen(line.color, 1.5, Qt::DashLine));
        g->setData(x, y, true);
        g->setName(line.name + " (" + unit + ")");
        mLimitGraphs.append(g);
    }
    legend->setVisible(!mLimitGraphs.isEmpty());
    rescaleToData(mAutoX, mAutoY);
    replot();
}

void SpectrumPlot::setAutoscale(bool x, bool y)
{
    const bool newX = x && !mAutoX;
    const bool newY = y && !mAutoY;
    mAutoX = x;
    mAutoY = y;
    if (newX || newY)
        rescale(newX, newY);
}

void SpectrumPlot::rescale(bool x, bool y)
{
    rescaleToData(x, y);
    replot();
}

void SpectrumPlot::rescaleToData(bool doX, bool doY)
{
    if (!doX && !doY)
        return;

    QList<QCPGraph*> dataGraphs = mOverlayGraphs;
    if (graphCount() > 0)
        dataGraphs.prepend(graph(0));

    // x: span of the measured data only (limit lines cover whole regulation bands)
    double xMin = 0, xMax = 0;
    bool found = false;
    for (QCPGraph *g : dataGraphs)
    {
        for (auto it = g->data()->constBegin(); it != g->data()->constEnd(); ++it)
        {
            if (std::isnan(it->value))
                continue;
            if (!found)
            {
                xMin = xMax = it->key;
                found = true;
            }
            xMin = qMin(xMin, it->key);
            xMax = qMax(xMax, it->key);
        }
    }
    if (!found)
        return;

    if (doX)
    {
        if (xMax - xMin < 1e-9)
        {
            xMin -= 0.5;
            xMax += 0.5;
        }
        xAxis->setRange(xMin, xMax);
        xAxis->scaleRange(1.02, xAxis->range().center());
    }
    if (!doY)
        return;

    // y: data inside the displayed x range (all data if none is visible), plus the limit levels there
    const double viewMin = xAxis->range().lower;
    const double viewMax = xAxis->range().upper;
    double yMin = 0, yMax = 0;
    bool foundY = false;
    for (int pass = 0; pass < 2 && !foundY; ++pass)
    {
        for (QCPGraph *g : dataGraphs)
        {
            for (auto it = g->data()->constBegin(); it != g->data()->constEnd(); ++it)
            {
                if (std::isnan(it->value) || (pass == 0 && (it->key < viewMin || it->key > viewMax)))
                    continue;
                if (!foundY)
                {
                    yMin = yMax = it->value;
                    foundY = true;
                }
                yMin = qMin(yMin, it->value);
                yMax = qMax(yMax, it->value);
            }
        }
    }

    if (mLimitsInScale)
    {
        for (const LimitLine &line : mLimits.lines)
        {
            for (double xMHz : {viewMin, 0.5 * (viewMin + viewMax), viewMax})
            {
                double level;
                if (line.valueAt(xMHz * 1e6, &level))
                {
                    yMin = qMin(yMin, level);
                    yMax = qMax(yMax, level);
                }
            }
            for (const LimitSegment &s : line.segments)
            {
                for (const QPair<double, double> &p : {qMakePair(s.startHz, s.startLevel), qMakePair(s.stopHz, s.stopLevel)})
                {
                    if (p.first / 1e6 >= viewMin && p.first / 1e6 <= viewMax)
                    {
                        yMin = qMin(yMin, p.second);
                        yMax = qMax(yMax, p.second);
                    }
                }
            }
        }
    }

    if (yMax - yMin < 1e-6)
    {
        yMin -= 1;
        yMax += 1;
    }
    yAxis->setRange(yMin, yMax);
    yAxis->scaleRange(1.1, yAxis->range().center());
}
