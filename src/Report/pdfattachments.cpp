#include "pdfattachments.h"

#include <QFile>
#include <QFileInfo>
#include <QMap>
#include <QRegularExpression>
#include <QtEndian>

/*
 * A PDF attachment is a file specification dictionary pointing to an embedded file stream:
 *   12 0 obj << /Type /Filespec /F (data.csv) /UF (data.csv) /EF << /F 13 0 R >> >> endobj
 *   13 0 obj << /Type /EmbeddedFile /Subtype /text#2Fcsv /Length 4242 /Filter /FlateDecode >>
 *   stream ... endstream endobj
 * The reader walks the "N G obj" headers in file order, skipping stream data, and collects
 * stream objects and file specifications. File specifications stored in compressed object
 * streams (PDFs re-saved by some editors) are found by inflating those streams too.
 */

namespace
{
struct StreamObject
{
    QString dict;       // latin-1 view of the stream dictionary
    int dataStart = 0;  // byte range of the raw stream data
    int dataLength = 0;
};

bool isWhite(QChar c)
{
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\f' || c == QChar(0);
}

// Index of the first character after the literal string starting at pos ('('), or -1
int literalStringEnd(const QString &text, int pos)
{
    int depth = 0;
    for( int i = pos; i < text.size(); i++ )
    {
        QChar c = text.at(i);
        if ( c == '\\' ) i++;
        else if ( c == '(' ) depth++;
        else if ( c == ')' && --depth == 0 ) return i + 1;
    }
    return -1;
}

// Index of the first character after the dictionary starting at pos ("<<"), or -1
int dictEnd(const QString &text, int pos)
{
    int depth = 0;
    int i = pos;
    while ( i < text.size() )
    {
        QChar c = text.at(i);
        QChar next = i + 1 < text.size() ? text.at(i + 1) : QChar();
        if ( c == '(' )
        {
            i = literalStringEnd(text, i);
            if ( i < 0 ) return -1;
        }
        else if ( c == '<' && next == '<' )
        {
            depth++;
            i += 2;
        }
        else if ( c == '>' && next == '>' )
        {
            i += 2;
            if ( --depth == 0 ) return i;
        }
        else if ( c == '<' ) // hex string
        {
            i = text.indexOf('>', i);
            if ( i < 0 ) return -1;
            i++;
        }
        else if ( c == '%' ) // comment
        {
            while ( i < text.size() && text.at(i) != '\n' && text.at(i) != '\r' ) i++;
        }
        else
        {
            i++;
        }
    }
    return -1;
}

// Decodes a PDF text string (bytes of a literal or hex string): UTF-16BE with BOM,
// UTF-8 with BOM, else PDFDocEncoding (approximated as latin-1)
QString decodeTextString(const QByteArray &bytes)
{
    if ( bytes.startsWith("\xFE\xFF") )
    {
        QString s;
        for( int i = 2; i + 1 < bytes.size(); i += 2 )
        {
            s.append(QChar(static_cast<char16_t>((static_cast<uchar>(bytes.at(i)) << 8) | static_cast<uchar>(bytes.at(i + 1)))));
        }
        return s;
    }
    if ( bytes.startsWith("\xEF\xBB\xBF") )
    {
        return QString::fromUtf8(bytes.mid(3));
    }
    return QString::fromLatin1(bytes);
}

// String value starting at pos ('(' or '<'), decoded; empty if malformed
QString parseString(const QString &text, int pos)
{
    QByteArray bytes;
    if ( text.at(pos) == '<' )
    {
        int end = text.indexOf('>', pos);
        if ( end < 0 ) return QString();
        QByteArray hex = text.mid(pos + 1, end - pos - 1).toLatin1();
        hex.replace(" ", "").replace("\n", "").replace("\r", "").replace("\t", "");
        if ( hex.size() % 2 ) hex.append('0');
        bytes = QByteArray::fromHex(hex);
        return decodeTextString(bytes);
    }

    int end = literalStringEnd(text, pos);
    if ( end < 0 ) return QString();
    for( int i = pos + 1; i < end - 1; i++ )
    {
        QChar c = text.at(i);
        if ( c != '\\' )
        {
            bytes.append(static_cast<char>(c.unicode()));
            continue;
        }
        QChar e = text.at(++i);
        if ( e == 'n' ) bytes.append('\n');
        else if ( e == 'r' ) bytes.append('\r');
        else if ( e == 't' ) bytes.append('\t');
        else if ( e == 'b' ) bytes.append('\b');
        else if ( e == 'f' ) bytes.append('\f');
        else if ( e == '\r' || e == '\n' ) // line continuation
        {
            if ( e == '\r' && i + 1 < end - 1 && text.at(i + 1) == '\n' ) i++;
        }
        else if ( e >= '0' && e <= '7' )
        {
            int value = 0;
            int n = 0;
            for( ; n < 3 && i < end - 1 && text.at(i) >= '0' && text.at(i) <= '7'; n++, i++ )
            {
                value = value * 8 + (text.at(i).unicode() - '0');
            }
            i--;
            bytes.append(static_cast<char>(value));
        }
        else bytes.append(static_cast<char>(e.unicode()));
    }
    return decodeTextString(bytes);
}

bool hasFlateFilter(const QString &dict, bool *unsupported)
{
    static const QRegularExpression filterRe("/Filter\\s*(\\[[^\\]]*\\]|/[A-Za-z0-9]+)");
    QRegularExpressionMatch m = filterRe.match(dict);
    *unsupported = false;
    if ( !m.hasMatch() )
    {
        return false;
    }
    QString filters = m.captured(1);
    filters.remove('[').remove(']');
    QStringList list = filters.split('/', Qt::SkipEmptyParts);
    for( QString &f : list ) f = f.trimmed();
    list.removeAll(QString());
    if ( list == QStringList{"FlateDecode"} || list == QStringList{"Fl"} )
    {
        return true;
    }
    *unsupported = !list.isEmpty();
    return false;
}

bool inflate(const QByteArray &raw, int sizeHint, QByteArray *out)
{
    // qUncompress expects zlib data prefixed by the big-endian expected size (only a hint in Qt 6)
    QByteArray buf(4, '\0');
    qToBigEndian<quint32>(static_cast<quint32>(qMax(sizeHint, 1)), buf.data());
    buf.append(raw);
    *out = qUncompress(buf);
    return !out->isEmpty() || raw.isEmpty();
}

// Decoded data of a stream; false on unsupported filter or corrupted data
bool decodeStream(const QByteArray &file, const StreamObject &s, QByteArray *out)
{
    QByteArray raw = file.mid(s.dataStart, s.dataLength);
    bool unsupported = false;
    if ( hasFlateFilter(s.dict, &unsupported) )
    {
        static const QRegularExpression sizeRe("/Size\\s+(\\d+)");
        QRegularExpressionMatch m = sizeRe.match(s.dict);
        int hint = m.hasMatch() ? m.captured(1).toInt() : raw.size() * 8;
        return inflate(raw, hint, out);
    }
    if ( unsupported )
    {
        return false;
    }
    *out = raw;
    return true;
}

// Records the attachments named by the file specification dictionaries found in text
// (objects of the file or decompressed object streams): stream object number -> name
void scanFileSpecs(const QString &text, QMap<int, QString> *names)
{
    static const QRegularExpression efRe("/EF\\s*<<");
    static const QRegularExpression refRe("/(?:UF|F)\\s+(\\d+)\\s+\\d+\\s+R");
    static const QRegularExpression nameRe("/(UF|F)\\s*([(<])");

    int pos = 0;
    while ( (pos = text.indexOf("<<", pos)) >= 0 )
    {
        int end = dictEnd(text, pos);
        if ( end < 0 )
        {
            return;
        }
        QString dict = text.mid(pos, end - pos);
        QRegularExpressionMatch ef = efRe.match(dict);
        if ( !ef.hasMatch() )
        {
            pos = end;
            continue;
        }

        // Embedded file reference, inside the /EF sub-dictionary
        int efStart = ef.capturedEnd() - 2;
        int efEnd = dictEnd(dict, efStart);
        QRegularExpressionMatch ref = refRe.match(dict.mid(efStart, efEnd - efStart));
        if ( ref.hasMatch() )
        {
            // File name: /UF (unicode) preferred over /F, outside the /EF sub-dictionary
            QString outer = dict.left(efStart) + dict.mid(efEnd);
            QString name;
            QRegularExpressionMatchIterator it = nameRe.globalMatch(outer);
            while ( it.hasNext() )
            {
                QRegularExpressionMatch nm = it.next();
                int strPos = nm.capturedStart(2);
                if ( outer.mid(strPos, 2) == "<<" ) continue;
                QString value = parseString(outer, strPos);
                if ( !value.isEmpty() && (name.isEmpty() || nm.captured(1) == "UF") )
                {
                    name = value;
                }
            }
            int objNum = ref.captured(1).toInt();
            if ( !name.isEmpty() || !names->contains(objNum) )
            {
                names->insert(objNum, name);
            }
        }
        pos = end;
    }
}
} // namespace

bool PdfAttachments::read(const QString &path, QList<PdfAttachment> *attachments, QString *error)
{
    attachments->clear();
    QFile f(path);
    if ( !f.open(QIODevice::ReadOnly) )
    {
        *error = f.errorString();
        return false;
    }
    const QByteArray file = f.readAll();
    if ( !file.left(1024).contains("%PDF-") )
    {
        *error = "not a PDF file";
        return false;
    }

    const QString text = QString::fromLatin1(file); // 1:1 byte <-> char mapping
    static const QRegularExpression objRe("(\\d+)\\s+\\d+\\s+obj\\b");
    static const QRegularExpression typeRe("/Type\\s*/EmbeddedFile(?![A-Za-z0-9])");
    static const QRegularExpression objStmRe("/Type\\s*/ObjStm(?![A-Za-z0-9])");
    static const QRegularExpression lengthRe("/Length\\s+(\\d+)(\\s+\\d+\\s+R)?");
    static const QRegularExpression encryptRe("/Encrypt\\s*(\\d+\\s+\\d+\\s+R|<<)");

    QMap<int, StreamObject> streams;    // latest definition of each stream object
    QList<int> streamOrder;             // object numbers in file order
    QList<int> typedEmbedded;           // streams with /Type /EmbeddedFile
    QMap<int, QString> names;           // from the file specifications
    bool encrypted = false;

    int pos = 0;
    while ( true )
    {
        QRegularExpressionMatch obj = objRe.match(text, pos);
        if ( !obj.hasMatch() )
        {
            break;
        }
        pos = obj.capturedEnd();
        int objNum = obj.captured(1).toInt();

        int i = pos;
        while ( i < text.size() && isWhite(text.at(i)) ) i++;
        if ( !text.mid(i, 2).startsWith("<<") )
        {
            continue;
        }
        int end = dictEnd(text, i);
        if ( end < 0 )
        {
            break;
        }
        QString dict = text.mid(i, end - i);
        pos = end;

        int s = end;
        while ( s < text.size() && isWhite(text.at(s)) ) s++;
        if ( text.mid(s, 6) != "stream" )
        {
            scanFileSpecs(dict, &names);
            if ( encryptRe.match(dict).hasMatch() ) encrypted = true; // trailer-like dictionary
            continue;
        }

        // Stream data starts after the EOL following "stream"
        s += 6;
        if ( s < text.size() && text.at(s) == '\r' ) s++;
        if ( s < text.size() && text.at(s) == '\n' ) s++;

        int length = -1;
        QRegularExpressionMatch len = lengthRe.match(dict);
        if ( len.hasMatch() && len.captured(2).isEmpty() )
        {
            length = len.captured(1).toInt();
            int after = s + length;
            while ( after < text.size() && isWhite(text.at(after)) ) after++;
            if ( length < 0 || text.mid(after, 9) != "endstream" )
            {
                length = -1; // wrong or indirect length: rely on the "endstream" keyword
            }
        }
        if ( length < 0 )
        {
            int e = text.indexOf("endstream", s);
            if ( e < 0 )
            {
                break;
            }
            length = e - s;
            while ( length > 0 && (text.at(s + length - 1) == '\n' || text.at(s + length - 1) == '\r') ) length--;
        }

        StreamObject so;
        so.dict = dict;
        so.dataStart = s;
        so.dataLength = length;
        if ( !streams.contains(objNum) ) streamOrder.append(objNum);
        streams.insert(objNum, so);
        if ( typeRe.match(dict).hasMatch() && !typedEmbedded.contains(objNum) ) typedEmbedded.append(objNum);
        if ( dict.contains("/XRef") && encryptRe.match(dict).hasMatch() ) encrypted = true;

        if ( objStmRe.match(dict).hasMatch() )
        {
            QByteArray decoded;
            if ( decodeStream(file, so, &decoded) )
            {
                scanFileSpecs(QString::fromLatin1(decoded), &names);
            }
        }
        pos = s + length;
    }

    if ( encrypted )
    {
        *error = "encrypted PDF, attachments cannot be read";
        return false;
    }

    QStringList failed;
    for( int objNum : streamOrder )
    {
        if ( !typedEmbedded.contains(objNum) && !names.contains(objNum) )
        {
            continue;
        }
        PdfAttachment a;
        a.name = names.value(objNum);
        if ( a.name.isEmpty() )
        {
            a.name = "attachment_" + QString::number(attachments->size() + 1);
        }
        if ( !decodeStream(file, streams.value(objNum), &a.data) )
        {
            failed.append(a.name);
            continue;
        }
        attachments->append(a);
    }

    if ( attachments->isEmpty() )
    {
        *error = failed.isEmpty() ? "no embedded data in this PDF"
                                  : "cannot decode embedded file(s): " + failed.join(", ");
        return false;
    }
    return true;
}
