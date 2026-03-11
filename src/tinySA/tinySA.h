#ifndef TINY_SA_H
#define TINY_SA_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QByteArray>
#include <QString>
#include <QtSerialPort/QSerialPort>

class PowerSpectrum
{
public:
    QVector<float> power;
    QVector<float> freq;
};

class TinySA : public QObject
{
    Q_OBJECT

public:
    struct Setup
    {
        // Only fields backed by the provided USB doc are included here.
        QString mode = "low";          // "low" or "high"
        int attenuationDb = -1;        // -1 => auto, else 0..31
        int rbwKHz = -1;               // -1 => auto, else 3..600
        int repeatCount = 1;           // 1..1000
        bool spurReduction = false;    // spur on/off
        float extGainDb = 0.0f;        // -100..100

        quint64 centerHz = 0;
        quint64 spanHz = 0;

        int points = 290;              // scan max is 290, scanraw unlimited
        float rawOffsetDb = 174.0f;    // 128=tinySA, 174=tinySA Ultra
    };

    explicit TinySA(QObject *parent = nullptr);
    ~TinySA();

    bool isOpen() const;

    // Applies only commands explicitly documented on the USB page.
    bool setDefaults();
    bool applySetup(const Setup &setup);

    // Capture using scanraw, decoded into PowerSpectrum.
    PowerSpectrum capture();

    void setSetup(const Setup &setup);

private slots:
    void pollConnection();
    void onSerialReadyRead();
    void onPortError(QSerialPort::SerialPortError error);

private:
    bool sendCommand(const QString &cmd);
    bool sendCommandExpectQuiet(const QString &cmd, int settleMs = 50);

    QByteArray readExact(int byteCount, int timeoutMs);
    QByteArray readUntilByte(char terminator, int timeoutMs);
    void clearPendingInput();

    bool setMode(const QString &mode);
    bool setAttenuationAutoOrDb(int attenuationDb);
    bool setRbwAutoOrKHz(int rbwKHz);
    bool setRepeat(int repeatCount);
    bool setSpur(bool enabled);
    bool setExtGain(float db);
    bool setSweepCenter(quint64 centerHz);
    bool setSweepSpan(quint64 spanHz);

private:
    const int portPollIntervalMs = 500;
    const quint16 portVID = 0x0483;
    const quint16 portPID = 0x5740;

    const QByteArray commandEol = "\r\n";

    QSerialPort port;
    QByteArray portBuffer;
    QTimer pollTimer;
    bool isConnected = false;

    Setup currentSetup;
};

#endif
