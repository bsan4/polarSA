#include "exportdialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

ExportDialog::ExportDialog(const QString &summary, QWidget *parent, const QString &title, const QString &okText)
    : QDialog(parent)
{
    setWindowTitle(title);

    QSettings settings;
    settings.beginGroup("export");

    mDut = new QLineEdit(settings.value("dut").toString());
    mDut->setPlaceholderText("Device under test (also used in the file name)");
    mAntenna = new QLineEdit(settings.value("antenna").toString());
    mAntenna->setPlaceholderText("e.g. PCB inverted-F, whip 1/4 wave...");
    mMatching = new QLineEdit(settings.value("matching").toString());
    mMatching->setPlaceholderText("e.g. pi network 2.2nH / 1.5pF / NC");

    mPolarization = new QComboBox;
    mPolarization->setEditable(true);
    mPolarization->addItems({"", "Horizontal", "Vertical"});
    mPolarization->setCurrentText(settings.value("polarization").toString());

    mOperator = new QLineEdit(settings.value("operator").toString());

    mComment = new QPlainTextEdit;
    mComment->setPlaceholderText("Free comments: setup, distance, firmware, TX power...");
    mComment->setPlainText(settings.value("comment").toString());

    settings.endGroup();

    QFormLayout *form = new QFormLayout;
    form->addRow("DUT", mDut);
    form->addRow("Antenna", mAntenna);
    form->addRow("Matching", mMatching);
    form->addRow("Polarization", mPolarization);
    form->addRow("Operator", mOperator);
    form->addRow("Comments", mComment);

    QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    buttons->button(QDialogButtonBox::Ok)->setText(okText);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() { saveFields(); accept(); });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel(summary));
    layout->addLayout(form);
    layout->addWidget(buttons);

    resize(460, 380);
    mDut->setFocus();
}

QString ExportDialog::dutName() const
{
    return mDut->text().trimmed();
}

QStringList ExportDialog::metadata() const
{
    QStringList lines;
    auto add = [&lines](const QString &key, const QString &value) {
        QString v = value.simplified(); // one line per entry
        if (!v.isEmpty())
            lines.append(key + "," + v);
    };

    add("dut", mDut->text());
    add("antenna", mAntenna->text());
    add("matching", mMatching->text());
    add("polarization", mPolarization->currentText());
    add("operator", mOperator->text());

    // Multi-line comments: one "comment" entry per line
    for (const QString &line : mComment->toPlainText().split('\n'))
        add("comment", line);

    return lines;
}

void ExportDialog::saveFields() const
{
    QSettings settings;
    settings.beginGroup("export");
    settings.setValue("dut", mDut->text());
    settings.setValue("antenna", mAntenna->text());
    settings.setValue("matching", mMatching->text());
    settings.setValue("polarization", mPolarization->currentText());
    settings.setValue("operator", mOperator->text());
    settings.setValue("comment", mComment->toPlainText());
    settings.endGroup();
}
