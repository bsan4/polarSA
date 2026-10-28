#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QStringList>

class QLineEdit;
class QComboBox;
class QPlainTextEdit;

// Asks the user for free-form measurement notes before an export.
// Field values are remembered between exports (QSettings).
class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(const QString &summary, QWidget *parent = nullptr,
                          const QString &title = "Export Data - Measurement notes",
                          const QString &okText = "Export...");

    // "key,value" lines ready to be written as CSV metadata; empty fields are skipped
    QStringList metadata() const;
    QString dutName() const;

private:
    void saveFields() const;

    QLineEdit *mDut = nullptr;
    QLineEdit *mAntenna = nullptr;
    QLineEdit *mMatching = nullptr;
    QComboBox *mPolarization = nullptr;
    QLineEdit *mOperator = nullptr;
    QPlainTextEdit *mComment = nullptr;
};

#endif
