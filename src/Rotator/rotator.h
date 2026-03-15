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

signals:
    void connected();
    void disconnected();

private slots:
    void pollConnection();
    void onSerialReadyRead();
    void onPortError(QSerialPort::SerialPortError error);

private:
    const int portPollIntervalMs = 500;
    const quint16 portVID = 0x2341;
    const quint16 portPID = 0x8037;
    Q_SLOT bool sendCommand(const QString &cmd);

    QSerialPort port;
    QTimer pollTimer;
    bool isConnected = false;
};

#endif
