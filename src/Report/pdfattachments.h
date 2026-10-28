#ifndef PDFATTACHMENTS_H
#define PDFATTACHMENTS_H

#include <QByteArray>
#include <QList>
#include <QString>

struct PdfAttachment
{
    QString name;       // file name from the file specification, or "attachment_<n>" when not found
    QByteArray data;    // decoded content
};

namespace PdfAttachments
{
    // Extracts the embedded files of a PDF (as written by QPdfWriter::addFileAttachment).
    // Minimal reader: scans the file for /EmbeddedFile streams, so it does not need the
    // cross-reference table. Handles uncompressed and FlateDecode streams; encrypted PDFs
    // are rejected. Returns false (with *error set) when the file cannot be read or holds
    // no attachment.
    bool read(const QString &path, QList<PdfAttachment> *attachments, QString *error);
}

#endif
