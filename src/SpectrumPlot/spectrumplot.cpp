#include "spectrumplot.h"
#include <QApplication>
#include <QPalette>

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

    applyTheme();

    // --- MHz axis ticker ---
    auto ticker = QSharedPointer<QCPAxisTickerFixed>::create();
    ticker->setScaleStrategy(QCPAxisTickerFixed::ssMultiples);
    ticker->setTickStep(1); // 1 MHz grid, but data will be ploted in unit of MHz
    xAxis->setTicker(ticker);

    xAxis->setNumberFormat("f");   // fixed
    xAxis->setNumberPrecision(3);  // 3 decimals

    xAxis->setLabel("Frequency (MHz)");
    yAxis->setLabel("Power (dBm)");

    legend->setVisible(false);

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

    // setOpenGl(true);
}

void SpectrumPlot::setPrintTheme(bool on)
{
    mPrintTheme = on;
    applyTheme();
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
        graph(0)->setPen(QPen(pal.highlight().color(), 2));
}

void SpectrumPlot::clearSpectrum()
{
    if (graphCount() == 0)
        addGraph();

    graph(0)->data()->clear();
    replot();
}

void SpectrumPlot::setPowerSpectrum(const PowerSpectrum &spectrum)
{
    if (graphCount() == 0)
        addGraph();

    const int n = qMin(spectrum.freq.size(), spectrum.power.size());

    if (n <= 0)
    {
        graph(0)->data()->clear();
        replot();
        return;
    }

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

    graph(0)->setData(x, y, true);

    xAxis->rescale(true);
    yAxis->rescale(true);

    yAxis->scaleRange(1.1, yAxis->range().center());
    xAxis->scaleRange(1.02, xAxis->range().center());

    replot();
}
