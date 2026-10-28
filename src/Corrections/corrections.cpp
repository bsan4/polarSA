#include "corrections.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>
#include <cmath>

namespace
{
// Colors of the limit lines, in file order
const QList<QColor> kLimitColors = {
    QColor("#ff8c00"),  // orange
    QColor("#e0409f"),  // pink
    QColor("#d4b000"),  // gold
    QColor("#9a9a9a"),  // grey
    QColor("#2fa84f"),  // green
    QColor("#8a5cff"),  // violet
};

// Small CSV reader shared by both file types: '#' lines are "key,value" metadata,
// ',' ';' or tab separators, ',' decimal mark accepted with ';' / tab separators.
struct CsvRows
{
    QStringList metadataKeys;
    QStringList metadataValues;
    QStringList header;
    QList<QStringList> rows;
    QList<int> lineNumbers;
};

bool readCsv(const QString &path, CsvRows &csv, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        *error = file.errorString();
        return false;
    }
    QTextStream in(&file);
    QChar sep;
    int lineNo = 0;
    while (!in.atEnd())
    {
        QString line = in.readLine().trimmed();
        lineNo++;
        if (line.isEmpty())
            continue;
        if (line.startsWith('#'))
        {
            QString meta = line.mid(1).trimmed();
            int comma = meta.indexOf(',');
            if (comma > 0)
            {
                csv.metadataKeys << meta.left(comma).trimmed().toLower();
                csv.metadataValues << meta.mid(comma + 1).trimmed();
            }
            continue;
        }
        if (sep.isNull())
            sep = line.contains(';') ? QChar(';') : line.contains('\t') ? QChar('\t') : QChar(',');
        QStringList fields = line.split(sep);
        for (QString &f : fields)
        {
            f = f.trimmed();
            if (sep != ',')
                f.replace(',', '.');
        }
        while (fields.size() > 1 && fields.last().isEmpty())
            fields.removeLast();

        if (csv.header.isEmpty())
        {
            csv.header = fields;
            continue;
        }
        csv.rows << fields;
        csv.lineNumbers << lineNo;
    }
    if (csv.rows.isEmpty())
    {
        *error = "no data rows found";
        return false;
    }
    return true;
}

QString metadata(const CsvRows &csv, const QString &key)
{
    int i = csv.metadataKeys.indexOf(key);
    return i >= 0 ? csv.metadataValues.at(i) : QString();
}

// Frequency scale from a header cell: "start_MHz" -> 1e6, "frequency_Hz" -> 1
double frequencyScale(const QString &headerCell)
{
    QString h = headerCell.toLower();
    if (h.contains("ghz")) return 1e9;
    if (h.contains("mhz")) return 1e6;
    if (h.contains("khz")) return 1e3;
    return 1.0;
}

bool toNumber(const QString &text, double *value)
{
    bool ok = false;
    *value = text.toDouble(&ok);
    return ok && std::isfinite(*value);
}
} // namespace

double CalibrationTable::at(double hz) const
{
    if (freqHz.isEmpty())
        return 0.0;
    if (hz <= freqHz.first())
        return correctionDb.first();
    if (hz >= freqHz.last())
        return correctionDb.last();
    auto it = std::upper_bound(freqHz.begin(), freqHz.end(), hz);
    int i = static_cast<int>(it - freqHz.begin());
    double t = (hz - freqHz[i - 1]) / (freqHz[i] - freqHz[i - 1]);
    return correctionDb[i - 1] + t * (correctionDb[i] - correctionDb[i - 1]);
}

bool CalibrationTable::covers(double hz) const
{
    return !freqHz.isEmpty() && hz >= freqHz.first() && hz <= freqHz.last();
}

bool LimitLine::valueAt(double hz, double *level) const
{
    bool found = false;
    double best = 0;
    for (const LimitSegment &s : segments)
    {
        if (hz < s.startHz || hz > s.stopHz)
            continue;
        double v = s.stopHz > s.startHz
                       ? s.startLevel + (hz - s.startHz) / (s.stopHz - s.startHz) * (s.stopLevel - s.startLevel)
                       : qMin(s.startLevel, s.stopLevel);
        if (!found || v < best)
            best = v;
        found = true;
    }
    if (found)
        *level = best;
    return found;
}

/*
 * Calibration CSV:
 *   # unit,dBuV/m                       <- unit of the corrected levels (default: dB)
 *   # description,LPDA + 3 m RG58, 2026-11
 *   frequency_Hz,correction_dB          <- header (frequency_MHz / _kHz / _GHz also accepted)
 *   30000000,125.2
 *   ...
 * corrected level = measured level (dBm) + correction (linear interpolation in frequency)
 */
bool Corrections::loadCalibration(const QString &path, CalibrationTable &table, QString *error)
{
    CsvRows csv;
    if (!readCsv(path, csv, error))
        return false;
    if (csv.header.size() < 2)
    {
        *error = "header must have 2 columns: frequency, correction";
        return false;
    }
    const double scale = frequencyScale(csv.header.at(0));

    QVector<QPair<double, double>> points;
    for (int r = 0; r < csv.rows.size(); ++r)
    {
        const QStringList &f = csv.rows.at(r);
        double hz, db;
        if (f.size() < 2 || !toNumber(f.at(0), &hz) || !toNumber(f.at(1), &db))
        {
            *error = QString("line %1: expected frequency,correction").arg(csv.lineNumbers.at(r));
            return false;
        }
        points.append({hz * scale, db});
    }
    std::sort(points.begin(), points.end(), [](const QPair<double, double> &a, const QPair<double, double> &b) {
        return a.first < b.first;
    });

    table = CalibrationTable();
    for (const auto &p : points)
    {
        if (!table.freqHz.isEmpty() && p.first <= table.freqHz.last())
        {
            *error = "duplicate frequency " + QString::number(p.first, 'f', 0) + " Hz";
            return false;
        }
        table.freqHz.append(p.first);
        table.correctionDb.append(p.second);
    }
    table.path = QFileInfo(path).absoluteFilePath();
    table.name = QFileInfo(path).fileName();
    QString unit = metadata(csv, "unit");
    table.unit = unit.isEmpty() ? QString("dB") : unit;
    table.description = metadata(csv, "description");
    return true;
}

/*
 * Limits CSV:
 *   # name,EN 55032 Class B, 3 m
 *   # unit,dBuV/m
 *   # description,...
 *   limit,start_MHz,stop_MHz,start_level,stop_level      <- frequencies in Hz / kHz / MHz / GHz (from the header)
 *   Quasi-peak,30,230,40,40
 *   Quasi-peak,230,1000,47,47
 *   Average,1000,3000,50,50
 * stop_level may be left empty (= start_level). Rows with the same limit name form one line.
 */
bool Corrections::loadLimits(const QString &path, LimitSet &limits, QString *error)
{
    CsvRows csv;
    if (!readCsv(path, csv, error))
        return false;
    if (csv.header.size() < 4)
    {
        *error = "header must be: limit,start,stop,start_level[,stop_level]";
        return false;
    }
    const double startScale = frequencyScale(csv.header.at(1));
    const double stopScale = frequencyScale(csv.header.at(2));

    limits = LimitSet();
    for (int r = 0; r < csv.rows.size(); ++r)
    {
        const QStringList &f = csv.rows.at(r);
        LimitSegment s;
        double start, stop;
        if (f.size() < 4 || f.at(0).isEmpty() || !toNumber(f.at(1), &start) || !toNumber(f.at(2), &stop)
            || !toNumber(f.at(3), &s.startLevel))
        {
            *error = QString("line %1: expected limit,start,stop,start_level[,stop_level]").arg(csv.lineNumbers.at(r));
            return false;
        }
        s.startHz = start * startScale;
        s.stopHz = stop * stopScale;
        s.stopLevel = s.startLevel;
        if (f.size() >= 5 && !f.at(4).isEmpty() && !toNumber(f.at(4), &s.stopLevel))
        {
            *error = QString("line %1: invalid stop level '%2'").arg(csv.lineNumbers.at(r)).arg(f.at(4));
            return false;
        }
        if (s.stopHz < s.startHz)
        {
            *error = QString("line %1: stop frequency below start frequency").arg(csv.lineNumbers.at(r));
            return false;
        }

        int index = -1;
        for (int i = 0; i < limits.lines.size(); ++i)
            if (limits.lines[i].name == f.at(0))
                index = i;
        if (index < 0)
        {
            LimitLine line;
            line.name = f.at(0);
            line.color = kLimitColors.at(limits.lines.size() % kLimitColors.size());
            limits.lines.append(line);
            index = limits.lines.size() - 1;
        }
        limits.lines[index].segments.append(s);
    }
    for (LimitLine &line : limits.lines)
    {
        std::sort(line.segments.begin(), line.segments.end(), [](const LimitSegment &a, const LimitSegment &b) {
            return a.startHz < b.startHz;
        });
    }

    limits.path = QFileInfo(path).absoluteFilePath();
    QString name = metadata(csv, "name");
    limits.name = name.isEmpty() ? QFileInfo(path).completeBaseName() : name;
    QString unit = metadata(csv, "unit");
    limits.unit = unit.isEmpty() ? QString("dBuV/m") : unit;
    limits.description = metadata(csv, "description");
    return true;
}

bool Corrections::sameUnit(const QString &a, const QString &b)
{
    auto norm = [](QString u) {
        u = u.trimmed().toLower();
        u.replace(QChar(0x00B5), 'u'); // micro sign
        u.replace(QChar(0x03BC), 'u'); // greek mu
        u.remove(' ');
        return u;
    };
    return norm(a) == norm(b);
}

QString Corrections::displayUnit(const QString &unit)
{
    QString u = unit;
    u.replace("dBuV", QString("dB") + QChar(0x00B5) + "V");
    u.replace("dBuA", QString("dB") + QChar(0x00B5) + "A");
    return u;
}
