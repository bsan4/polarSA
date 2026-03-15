#include "rotator.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>

#define ROTATOR_REG_ANGLE_Z 0


Rotator::Rotator(QObject *parent)
    : QObject(parent)
{
    connect(&port, &QSerialPort::readyRead, this, &Rotator::onSerialReadyRead);
    connect(&port, &QSerialPort::errorOccurred, this, &Rotator::onPortError);

    pollTimer.setSingleShot(false);
    pollTimer.setInterval(portPollIntervalMs);
    connect(&pollTimer, &QTimer::timeout, this, &Rotator::pollConnection);
    pollTimer.start();
}

Rotator::~Rotator()
{
    if (port.isOpen())
        port.close();
}

bool Rotator::isOpen() const
{
    return isConnected && port.isOpen();
}

void Rotator::setAngle(int deg)
{
    char cmd[32];
    sprintf(cmd, "set %d %d\n", ROTATOR_REG_ANGLE_Z, deg);
    sendCommand( cmd );
}

void Rotator::pollConnection()
{
    if (isConnected && port.isOpen())
        return;
    if (isConnected && !port.isOpen())
    {
        isConnected = false;
        emit disconnected();
    }

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        if (info.hasVendorIdentifier() &&
            info.hasProductIdentifier() &&
            info.vendorIdentifier() == portVID &&
            info.productIdentifier() == portPID)
        {
            qDebug() << "Found Rotator on" << info.portName();

            port.setPort(info);
            port.setBaudRate(QSerialPort::Baud115200);
            port.setDataBits(QSerialPort::Data8);
            port.setParity(QSerialPort::NoParity);
            port.setStopBits(QSerialPort::OneStop);
            port.setFlowControl(QSerialPort::NoFlowControl);

            if (port.open(QIODevice::ReadWrite))
            {
                qDebug() << "Serial opened";
                emit connected();
                isConnected = true;
            }
            else
            {
                qDebug() << "Open failed:" << port.errorString();
                isConnected = false;
                emit disconnected();
            }

            return;
        }
    }
}

void Rotator::onSerialReadyRead()
{
    ;
}

void Rotator::onPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        qDebug() << "Serial disconnected:" << port.errorString();
        isConnected = false;
        if (port.isOpen())
            port.close();
        emit disconnected();
    }
}

bool Rotator::sendCommand(const QString &cmd)
{
    if (!isOpen())
        return false;

    const QByteArray bytes = cmd.toUtf8();
    const qint64 written = port.write(bytes);
    if (written != bytes.size())
        return false;

    return port.waitForBytesWritten(1000);
}
