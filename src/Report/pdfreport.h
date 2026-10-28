#ifndef PDFREPORT_H
#define PDFREPORT_H

#include <QColor>
#include <QList>
#include <QString>
#include <QStringList>
#include "src/Analysis/patternanalysis.h"

class PowerPlot;
class SpectrumPlot;

struct ReportTrace
{
    QString name;
    QString source;                 // file path, or empty for the live capture
    QColor color;
    double angleOffsetDeg = 0;
    double gainOffsetDb = 0;
    PatternMetrics metrics;         // at the report frequency, adjustments applied
    QStringList metadata;           // "key,value" lines
    QString dataFileName;           // embedded CSV attachment name (unique in the report)
    QByteArray dataCsv;             // polarSA CSV of the trace, empty: not embedded
};

struct ReportData
{
    QString configName;             // empty: no config file loaded
    double freqHz = 0;
    bool normalized = false;
    QStringList notes;              // "key,value" lines from the notes dialog
    QStringList captureSettings;    // "key,value" lines of the live capture
    QList<ReportTrace> traces;      // visible traces
    PowerPlot *polarPlot = nullptr;
    SpectrumPlot *maxPlot = nullptr; // nullptr: no Max Emissions page
};

namespace PdfReport
{
    // A4 report: header with logo and date, info block, radiation pattern, traces table,
    // Max Emissions plot and per-trace details / metadata.
    // Traces with dataCsv are embedded as file attachments, read back by PdfAttachments::read.
    bool write(const QString &path, const ReportData &data, QString *error);
}

#endif
