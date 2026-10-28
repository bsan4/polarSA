#ifndef ROTATOR_H
#define ROTATOR_H

#include <QObject>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

class Rotator : public QObject
{
    Q_OBJECT

public:
    explicit Rotator(QObject *parent = nullptr);
    ~Rotator();
    bool isOpen() const;
    void setAngle(int deg);
    void setSpeed(int degPerSec);   // 0 = firmware maximum
    int moveTimeoutMs() const;

signals:
    void connected();
    void disconnected();
    void positionReached(int deg);

private slots:
    void pollConnection();
    void onSerialReadyRead();
    void onPortError(QSerialPort::SerialPortError error);

private:
    const int portPollIntervalMs = 500;
    const quint16 portVID = 0x2341;
    const quint16 portPID = 0x8037;
    const int fullTravelDeg = 405;
    Q_SLOT bool sendCommand(const QString &cmd);

    QSerialPort port;
    QTimer pollTimer;
    bool isConnected = false;
    QByteArray rxBuffer;
    int speedDps = 0;
};

#endif
