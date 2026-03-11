#include "tinySA.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QDebug>

TinySA::TinySA()
{
    connect(&port, &QSerialPort::errorOccurred, this, &TinySA::onPortError);

    pollTimer.setSingleShot(false);
    pollTimer.setInterval( portPollIntervalMs );
    connect(&pollTimer, &QTimer::timeout, this, &TinySA::pollConnection);
    pollTimer.start();
}

TinySA::~TinySA()
{

}

void TinySA::setDefaults()
{

}

PowerSpectrum TinySA::capture()
{
    PowerSpectrum spec;
    return spec;
}

void TinySA::pollConnection()
{
    if ( isConnected ) return;

    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        if (info.hasVendorIdentifier() &&
            info.hasProductIdentifier() &&
            info.vendorIdentifier() == portVID &&
            info.productIdentifier() == portPID)
        {
            qDebug() << "Found STM32 VCP on" << info.portName();

            port.setPort(info);
            port.setBaudRate(QSerialPort::Baud115200);
            port.setDataBits(QSerialPort::Data8);
            port.setParity(QSerialPort::NoParity);
            port.setStopBits(QSerialPort::OneStop);
            port.setFlowControl(QSerialPort::NoFlowControl);

            if (port.open(QIODevice::ReadWrite))
            {
                qDebug() << "Serial opened";
                isConnected = true;
            }
            else
            {
                qDebug() << "Open failed:" << port.errorString();
                isConnected = false;
            }
        }
    }
}

void TinySA::onSerialData(QByteArray &data)
{
    qDebug() << "data";
}

void TinySA::onPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        isConnected = false;
    }
}
