#include "spectrumplot.h"
#include <QApplication>
#include <QPalette>
#include <cmath>
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
    QVector<double> x;
    QVector<double> y;
    x.reserve(n);
    y.reserve(n);
    for (int i = 0; i < n; ++i)
    {
        // convert Hz -> MHz
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
    rescaleToData();
    replot();
}

void SpectrumPlot::setOverlaySpectra(const QList<PowerSpectrum> &spectra, const QList<QColor> &colors)
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
        mOverlayGraphs[i]->setPen(QPen(i < colors.size() ? colors[i] : QColor(Qt::gray), 1.5));
        fillGraph(mOverlayGraphs[i], spectra[i]);
    }
    rescaleToData();
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
    rescaleToData();
    replot();
}

void SpectrumPlot::rescaleToData()
{
    // x: span of the measured data only (limit lines cover whole regulation bands)
    double xMin = 0, xMax = 0, yMin = 0, yMax = 0;
    bool found = false;
    QList<QCPGraph*> dataGraphs = mOverlayGraphs;
    if (graphCount() > 0)
        dataGraphs.prepend(graph(0));
    for (QCPGraph *g : dataGraphs)
    {
        for (auto it = g->data()->constBegin(); it != g->data()->constEnd(); ++it)
        {
            if (std::isnan(it->value))
                continue;
            if (!found)
            {
                xMin = xMax = it->key;
                yMin = yMax = it->value;
                found = true;
            }
            xMin = qMin(xMin, it->key);
            xMax = qMax(xMax, it->key);
            yMin = qMin(yMin, it->value);
            yMax = qMax(yMax, it->value);
        }
    }
    if (!found)
        return;

    // y: also the limit levels inside the displayed frequency span
    if (mLimitsInScale)
    {
        for (const LimitLine &line : mLimits.lines)
        {
            for (double xMHz : {xMin, 0.5 * (xMin + xMax), xMax})
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
                    if (p.first / 1e6 >= xMin && p.first / 1e6 <= xMax)
                    {
                        yMin = qMin(yMin, p.second);
                        yMax = qMax(yMax, p.second);
                    }
                }
            }
        }
    }

    if (xMax - xMin < 1e-9)
    {
        xMin -= 0.5;
        xMax += 0.5;
    }
    if (yMax - yMin < 1e-6)
    {
        yMin -= 1;
        yMax += 1;
    }
    xAxis->setRange(xMin, xMax);
    yAxis->setRange(yMin, yMax);
    yAxis->scaleRange(1.1, yAxis->range().center());
    xAxis->scaleRange(1.02, xAxis->range().center());
}
