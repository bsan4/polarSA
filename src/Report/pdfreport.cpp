#include "pdfreport.h"

#include <QDateTime>
#include <QFileInfo>
#include <QFontMetrics>
#include <QHash>
#include <QPageLayout>
#include <QPageSize>
#include <QPdfWriter>
#include <QSvgRenderer>
#include <cmath>

#include "inc/defines.h"
#include "lib/QCustomPlot/qcustomplot.h"
#include "src/PowerPlot/powerplot.h"
#include "src/SpectrumPlot/spectrumplot.h"

namespace
{
const QColor kAccent(31, 79, 191);
const QColor kMuted(110, 110, 110);
const QColor kRule(200, 200, 200);

QString degrees(double a)
{
    return QString::number(a, 'f', std::abs(a - std::round(a)) < 0.05 ? 0 : 1) + QChar(0x00B0);
}

// "key,value" -> ("Key", "value"); keys use '_' as separator in the files
QPair<QString, QString> splitKeyValue(const QString &line)
{
    int comma = line.indexOf(',');
    if (comma < 0)
        return {QString(), line};
    static const QHash<QString, QString> labels = {
        {"dut", "DUT"}, {"rbw", "RBW"}, {"lna", "LNA"},
        {"center_MHz", "Center (MHz)"}, {"span_MHz", "Span (MHz)"}, {"attenuation_dB", "Attenuation (dB)"},
        {"angle_start_deg", "Start angle (deg)"}, {"angle_end_deg", "End angle (deg)"}, {"angle_step_deg", "Angle step (deg)"},
        {"applied_rotation_deg", "Applied rotation (deg)"}, {"applied_gain_offset_dB", "Applied gain (dB)"}};
    QString key = line.left(comma).trimmed();
    if (labels.contains(key))
    {
        key = labels.value(key);
    }
    else
    {
        key.replace('_', ' ');
        if (!key.isEmpty())
            key[0] = key[0].toUpper();
    }
    return {key, line.mid(comma + 1).trimmed()};
}

class Page
{
public:
    Page(QPdfWriter &writer, QCPPainter &painter, const ReportData &data)
        : mWriter(writer), mPainter(painter), mData(data)
    {
        QRect r = writer.pageLayout().paintRectPixels(writer.resolution());
        mWidth = r.width();
        mHeight = r.height();
        mDate = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm");
        drawHeader();
    }

    int width() const { return mWidth; }
    int y() const { return mY; }
    void advance(int dy) { mY += dy; }

    void ensureSpace(int h)
    {
        if (mY + h > mHeight - kFooterHeight)
            newPage();
    }

    void newPage()
    {
        drawFooter();
        mWriter.newPage();
        mPageNo++;
        drawHeader();
    }

    void finish()
    {
        drawFooter();
    }

    void sectionTitle(const QString &title)
    {
        ensureSpace(40);
        mY += 8;
        QFont f = baseFont(11, true);
        mPainter.setFont(f);
        mPainter.setPen(kAccent);
        mPainter.drawText(QRect(0, mY, mWidth, 20), Qt::AlignLeft | Qt::AlignVCenter, title);
        mY += 22;
        mPainter.setPen(QPen(kRule, 1));
        mPainter.drawLine(QLineF(0, mY, mWidth, mY));
        mY += 6;
    }

    // Key / value lines in two columns
    void keyValues(const QList<QPair<QString, QString>> &items)
    {
        QFont keyFont = baseFont(8.5, true);
        QFont valueFont = baseFont(8.5, false);
        const int colW = mWidth / 2;
        const int keyW = 105;
        for (int i = 0; i < items.size(); i += 2)
        {
            int rowH = 0;
            for (int c = 0; c < 2 && i + c < items.size(); ++c)
            {
                QFontMetrics fm(valueFont, &mWriter);
                QRect br = fm.boundingRect(QRect(0, 0, colW - keyW - 10, 1000), Qt::TextWordWrap, items[i + c].second);
                rowH = qMax(rowH, br.height());
            }
            rowH = qMax(rowH, 14) + 3;
            ensureSpace(rowH);
            for (int c = 0; c < 2 && i + c < items.size(); ++c)
            {
                int x = c * colW;
                mPainter.setFont(keyFont);
                mPainter.setPen(kMuted);
                mPainter.drawText(QRect(x, mY, keyW, rowH), Qt::AlignLeft | Qt::AlignTop, items[i + c].first);
                mPainter.setFont(valueFont);
                mPainter.setPen(Qt::black);
                mPainter.drawText(QRect(x + keyW, mY, colW - keyW - 10, rowH), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                                  items[i + c].second);
            }
            mY += rowH;
        }
    }

    void paragraph(const QString &text, double pointSize = 8.5, const QColor &color = Qt::black, int indent = 0)
    {
        QFont f = baseFont(pointSize, false);
        QFontMetrics fm(f, &mWriter);
        QRect br = fm.boundingRect(QRect(0, 0, mWidth - indent, 10000), Qt::TextWordWrap, text);
        ensureSpace(br.height() + 2);
        mPainter.setFont(f);
        mPainter.setPen(color);
        mPainter.drawText(QRect(indent, mY, mWidth - indent, br.height()), Qt::AlignLeft | Qt::TextWordWrap, text);
        mY += br.height() + 2;
    }

    void plot(QCustomPlot *plot, int w, int h)
    {
        ensureSpace(h + 6);
        int x = (mWidth - w) / 2;
        mPainter.save();
        mPainter.translate(x, mY);
        plot->toPainter(&mPainter, w, h);
        mPainter.restore();
        mY += h + 6;
    }

    QFont baseFont(double pointSize, bool bold) const
    {
        QFont f("Segoe UI");
        f.setPointSizeF(pointSize);
        f.setBold(bold);
        return f;
    }

    QCPPainter &painter() { return mPainter; }

private:
    static constexpr int kHeaderHeight = 64;
    static constexpr int kFooterHeight = 28;

    void drawHeader()
    {
        // Logo
        QSvgRenderer logo(QString(":/polarsa_notxt.svg"));
        if (logo.isValid())
            logo.render(&mPainter, QRectF(0, 0, 44, 44));

        mPainter.setPen(Qt::black);
        mPainter.setFont(baseFont(16, true));
        mPainter.drawText(QRect(56, 0, mWidth - 56, 26), Qt::AlignLeft | Qt::AlignVCenter, "polarSA");
        mPainter.setFont(baseFont(10, false));
        mPainter.setPen(kMuted);
        mPainter.drawText(QRect(56, 24, mWidth - 56, 20), Qt::AlignLeft | Qt::AlignVCenter, "Radiation pattern report");

        mPainter.setFont(baseFont(9, false));
        mPainter.drawText(QRect(0, 0, mWidth, 22), Qt::AlignRight | Qt::AlignVCenter, mDate);
        if (!mData.configName.isEmpty())
            mPainter.drawText(QRect(0, 22, mWidth, 20), Qt::AlignRight | Qt::AlignVCenter, "Config: " + mData.configName);

        mPainter.setPen(QPen(kAccent, 2));
        mPainter.drawLine(QLineF(0, 52, mWidth, 52));
        mY = kHeaderHeight;
    }

    void drawFooter()
    {
        int y = mHeight - 18;
        mPainter.setPen(QPen(kRule, 1));
        mPainter.drawLine(QLineF(0, y - 4, mWidth, y - 4));
        mPainter.setFont(baseFont(7.5, false));
        mPainter.setPen(kMuted);
        QString left = QString("polarSA ") + POLARSA_VERSION
                       + (mData.configName.isEmpty() ? QString() : "  " + QString(QChar(0x00B7)) + "  config: " + mData.configName);
        mPainter.drawText(QRect(0, y, mWidth, 16), Qt::AlignLeft | Qt::AlignVCenter, left);
        mPainter.drawText(QRect(0, y, mWidth, 16), Qt::AlignRight | Qt::AlignVCenter, "Page " + QString::number(mPageNo));
    }

    QPdfWriter &mWriter;
    QCPPainter &mPainter;
    const ReportData &mData;
    QString mDate;
    int mWidth = 0;
    int mHeight = 0;
    int mY = 0;
    int mPageNo = 1;
};

void tracesTable(Page &page, const ReportData &data)
{
    struct Col { QString title; int width; };
    const QList<Col> cols = {
        {"", 16}, {"Trace", 0}, {"Max", 120}, {"Average", 72}, {QString(QChar(0x2212)) + "3 dB BW", 70},
        {"F/B", 56}, {"Rot. / gain", 90}};
    int fixed = 0;
    for (const Col &c : cols) fixed += c.width;
    const int nameW = page.width() - fixed;

    QCPPainter &p = page.painter();
    const int rowH = 18;

    auto drawRow = [&](const QStringList &cells, const QColor &swatch, bool header) {
        page.ensureSpace(rowH);
        int x = 0;
        p.setFont(page.baseFont(8.5, header));
        for (int i = 0; i < cols.size(); ++i)
        {
            int w = cols[i].width ? cols[i].width : nameW;
            if (i == 0)
            {
                if (swatch.isValid())
                    p.fillRect(QRect(x + 2, page.y() + 5, 10, 8), swatch);
            }
            else
            {
                p.setPen(header ? kMuted : Qt::black);
                QString text = p.fontMetrics().elidedText(cells.value(i), Qt::ElideMiddle, w - 6);
                p.drawText(QRect(x, page.y(), w - 6, rowH), Qt::AlignLeft | Qt::AlignVCenter, text);
            }
            x += w;
        }
        page.advance(rowH);
        p.setPen(QPen(kRule, 0.5));
        p.drawLine(QLineF(0, page.y(), page.width(), page.y()));
    };

    QStringList titles;
    for (const Col &c : cols) titles << c.title;
    drawRow(titles, QColor(), true);

    for (const ReportTrace &t : data.traces)
    {
        const PatternMetrics &m = t.metrics;
        QStringList cells;
        cells << "" << t.name;
        if (m.valid)
        {
            cells << QString::number(m.maxDb, 'f', 1) + " dBm @ " + degrees(m.maxAngleDeg)
                  << QString::number(m.averageDb, 'f', 1) + " dBm"
                  << (m.omni ? QString("omni") : m.beamwidth3dB >= 0 ? degrees(m.beamwidth3dB) : QString("-"))
                  << (m.hasFrontToBack ? QString::number(m.frontToBackDb, 'f', 1) + " dB" : QString("-"));
        }
        else
        {
            cells << "no data at this frequency" << "" << "" << "";
        }
        cells << degrees(t.angleOffsetDeg) + " / " + (t.gainOffsetDb >= 0 ? "+" : "") + QString::number(t.gainOffsetDb, 'f', 1) + " dB";
        drawRow(cells, t.color, false);
    }
}
} // namespace

bool PdfReport::write(const QString &path, const ReportData &data, QString *error)
{
    QPdfWriter writer(path);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageMargins(QMarginsF(15, 12, 15, 12), QPageLayout::Millimeter);
    writer.setResolution(96);
    writer.setTitle("polarSA radiation pattern report");
    writer.setCreator(QString("polarSA ") + POLARSA_VERSION);

    QCPPainter painter;
    if (!painter.begin(&writer))
    {
        *error = "cannot write " + path;
        return false;
    }
    painter.setMode(QCPPainter::pmVectorized);
    painter.setRenderHint(QPainter::Antialiasing);

    if (data.polarPlot) data.polarPlot->setPrintTheme(true);
    if (data.maxPlot) data.maxPlot->setPrintTheme(true);

    Page page(writer, painter, data);

    // --- Summary
    QList<QPair<QString, QString>> info;
    info << QPair<QString, QString>("Frequency", QString::number(data.freqHz * 1e-6, 'f', 3) + " MHz");
    info << QPair<QString, QString>("Scale", data.normalized ? "normalized (each trace relative to its max)" : "absolute (dBm)");
    info << QPair<QString, QString>("Config file", data.configName.isEmpty() ? QString("(none)") : data.configName);
    info << QPair<QString, QString>("Traces", QString::number(data.traces.size()));
    for (const QString &line : data.notes)
    {
        QPair<QString, QString> kv = splitKeyValue(line);
        if (kv.first == "Comment")
            continue; // shown as a paragraph below
        info << kv;
    }
    page.keyValues(info);

    QStringList comments;
    for (const QString &line : data.notes)
        if (splitKeyValue(line).first == "Comment")
            comments << splitKeyValue(line).second;
    if (!comments.isEmpty())
    {
        page.advance(2);
        page.paragraph(comments.join("\n"), 8.5, QColor(40, 40, 40));
    }

    if (!data.captureSettings.isEmpty())
    {
        QStringList parts;
        for (const QString &line : data.captureSettings)
        {
            QPair<QString, QString> kv = splitKeyValue(line);
            parts << kv.first + ": " + kv.second;
        }
        page.advance(2);
        page.paragraph("Live capture  " + QString(QChar(0x00B7)) + "  " + parts.join("   "), 7.5, kMuted);
    }

    // --- Radiation pattern + traces
    page.sectionTitle("Radiation pattern at " + QString::number(data.freqHz * 1e-6, 'f', 3) + " MHz");
    if (data.polarPlot)
    {
        int side = qMin(page.width(), 500);
        page.plot(data.polarPlot, side, side);
    }
    tracesTable(page, data);

    // --- Max emissions (own page, it is wide)
    if (data.maxPlot)
    {
        page.newPage();
        page.sectionTitle("Max emissions (max hold over all angles, live capture)");
        page.plot(data.maxPlot, page.width(), 380);
    }

    // --- Trace details
    page.sectionTitle("Trace details");
    for (const ReportTrace &t : data.traces)
    {
        page.ensureSpace(40);
        QCPPainter &p = page.painter();
        p.fillRect(QRect(0, page.y() + 4, 10, 8), t.color);
        p.setFont(page.baseFont(9, true));
        p.setPen(Qt::black);
        p.drawText(QRect(16, page.y(), page.width() - 16, 16), Qt::AlignLeft | Qt::AlignVCenter, t.name);
        page.advance(17);
        page.paragraph(t.source.isEmpty() ? QString("Live capture, not saved to a file") : t.source, 7.5, kMuted, 16);
        if (t.angleOffsetDeg != 0 || t.gainOffsetDb != 0)
            page.paragraph("Display adjustments: rotation " + degrees(t.angleOffsetDeg) + ", gain "
                           + (t.gainOffsetDb >= 0 ? "+" : "") + QString::number(t.gainOffsetDb, 'f', 1) + " dB", 8, Qt::black, 16);
        QStringList lines;
        for (const QString &line : t.metadata)
        {
            QPair<QString, QString> kv = splitKeyValue(line);
            lines << (kv.first.isEmpty() ? kv.second : kv.first + ": " + kv.second);
        }
        page.paragraph(lines.isEmpty() ? QString("(no metadata)") : lines.join("\n"), 7.5, QColor(40, 40, 40), 16);
        page.advance(6);
    }

    page.finish();
    painter.end();

    if (data.polarPlot)
    {
        data.polarPlot->setPrintTheme(false);
        data.polarPlot->replot();
    }
    if (data.maxPlot)
    {
        data.maxPlot->setPrintTheme(false);
        data.maxPlot->replot();
    }
    return true;
}
