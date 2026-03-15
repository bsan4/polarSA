#ifndef TINY_SA_H
#define TINY_SA_H

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QByteArray>
#include <QString>
#include <QtSerialPort/QSerialPort>

#include "inc/defines.h"

class TinySA : public QObject
{
    Q_OBJECT

public:
    struct Setup
    {
        QString mode = "low";          // "low" or "high"
        int attenuationDb = -1;        // -1 => auto, else 0..31
        float rbwKHz = -1;               // -1 => auto, else 3..600
        int repeatCount = 1;           // 1..1000
        bool spurReduction = false;    // spur on/off
        float extGainDb = 0.0f;        // -100..100

        quint64 centerHz = 0;
        quint64 spanHz = 0;

        int points = 290;              // tinySA scan max is 290, scanraw can do more
        float rawOffsetDb = 174.0f;    // 128=tinySA, 174=tinySA Ultra
    };

    explicit TinySA(QObject *parent = nullptr);
    ~TinySA();

    bool isOpen() const;

    bool setDefaults();
    bool applySetup(const Setup &setup);

    PowerSpectrum capture();

    void setSetup(const Setup &setup);

signals:
    void connected();
    void disconnected();

private slots:
    void pollConnection();
    void onSerialReadyRead();
    void onPortError(QSerialPort::SerialPortError error);

private:
    bool sendCommand(const QString &cmd);
    bool sendCommandExpectQuiet(const QString &cmd, int settleMs = 50);

    bool waitForIncomingData(int timeoutMs);
    bool waitForBufferSize(int minBytes, int timeoutMs);
    bool discardUntilByte(char target, int timeoutMs);
    QByteArray takeFromBuffer(int count);
    void clearPendingInput();

    bool readScanrawFrame(int points, QByteArray &frame, int timeoutMs);
    bool decodeScanrawFrame(const QByteArray &frame,
                            quint64 startHz,
                            quint64 stopHz,
                            int points,
                            PowerSpectrum &spec) const;

    bool setMode(const QString &mode);
    bool setAttenuationAutoOrDb(int attenuationDb);
    bool setRbwAutoOrKHz(float rbwKHz);
    bool setRepeat(int repeatCount);
    bool setSpur(bool enabled);
    bool setExtGain(float db);
    bool setSweepCenter(quint64 centerHz);
    bool setSweepSpan(quint64 spanHz);
    QByteArray readExact(int byteCount, int timeoutMs);
    QByteArray readUntil(const char endOfText, int timeoutMs);
    QByteArray readUntilByte(char terminator, int timeoutMs);

private:
    const int portPollIntervalMs = 500;
    const quint16 portVID = 0x0483;
    const quint16 portPID = 0x5740;

    const QByteArray commandEol = "\r\n";
    const QByteArray prompt = "ch> ";

    QSerialPort port;
    QByteArray portBuffer;
    QTimer pollTimer;
    bool isConnected = false;

    Setup currentSetup;
};

#endif
