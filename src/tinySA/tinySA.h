#ifndef TINY_SA_H
#define TINY_SA_H

#include <QObject>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

class PowerSpectrum
{
public:
    QVector<float> power;
    QVector<float> freq;
};

class TinySA: public QObject
{
public:
    TinySA();
    ~TinySA();
    void setDefaults();
    PowerSpectrum capture();

private:
    const int portPollIntervalMs = 500;
    const quint16 portVID = 0x0483;
    const quint16 portPID = 0x5740;

    Q_SLOT void pollConnection();
    Q_SLOT void onSerialData(QByteArray &data);
    Q_SLOT void onPortError(QSerialPort::SerialPortError error);
    QSerialPort port;
    QByteArray portBuffer;
    QTimer pollTimer;
    bool isConnected = false;
};

#endif
