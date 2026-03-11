#include "tinySA.h"

#include <QtSerialPort/QSerialPortInfo>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QThread>
#include <QDebug>

TinySA::TinySA(QObject *parent)
    : QObject(parent)
{
    connect(&port, &QSerialPort::readyRead, this, &TinySA::onSerialReadyRead);
    connect(&port, &QSerialPort::errorOccurred, this, &TinySA::onPortError);

    pollTimer.setSingleShot(false);
    pollTimer.setInterval(portPollIntervalMs);
    connect(&pollTimer, &QTimer::timeout, this, &TinySA::pollConnection);
    pollTimer.start();
    // Manually connect
    pollConnection();
}

TinySA::~TinySA()
{
    if (port.isOpen())
        port.close();
}

bool TinySA::isOpen() const
{
    return isConnected && port.isOpen();
}

void TinySA::setSetup(const Setup &setup)
{
    currentSetup = setup;
}

bool TinySA::setDefaults()
{
    return applySetup(currentSetup);
}

bool TinySA::applySetup(const Setup &setup)
{
    if (!isOpen())
    {
        qDebug() << "TinySA not connected";
        return false;
    }

    // Stop auto-refresh traffic if it is on, then pause sweeping before reconfiguration.
    // Both commands are documented on the USB page.
    if (!sendCommandExpectQuiet("refresh off")) return false;
    if (!sendCommandExpectQuiet("pause")) return false;

    if (!setMode(setup.mode)) return false;
    if (!setAttenuationAutoOrDb(setup.attenuationDb)) return false;
    if (!setRbwAutoOrKHz(setup.rbwKHz)) return false;
    if (!setRepeat(setup.repeatCount)) return false;
    if (!setSpur(setup.spurReduction)) return false;
    if (!setExtGain(setup.extGainDb)) return false;

    if (setup.centerHz != 0)
    {
        if (!setSweepCenter(setup.centerHz)) return false;
    }

    if (setup.spanHz != 0)
    {
        if (!setSweepSpan(setup.spanHz)) return false;
    }

    currentSetup = setup;
    return true;
}

PowerSpectrum TinySA::capture()
{
    PowerSpectrum spec;

    if (!isOpen())
    {
        qDebug() << "TinySA not connected";
        return spec;
    }

    if (currentSetup.centerHz == 0 || currentSetup.spanHz == 0)
    {
        qDebug() << "Capture aborted: center/span not configured";
        return spec;
    }

    const quint64 startHz = currentSetup.centerHz - currentSetup.spanHz / 2;
    const quint64 stopHz  = currentSetup.centerHz + currentSetup.spanHz / 2;
    const int points = currentSetup.points;

    if (points <= 1)
    {
        qDebug() << "Capture aborted: points must be > 1";
        return spec;
    }

    clearPendingInput();

    // scanraw format from doc:
    // '{' ('x' LSB MSB)*points '}' Note that LSB and MSB are reverted compared to doc
    // dBm = raw16/32 - 128   for tinySA
    // dBm = raw16/32 - 174   for tinySA Ultra
    //
    // We use option 0 to avoid continuous/unbuffered variants.
    const QString cmd = QString("scanraw %1 %2 %3")
                            .arg(startHz)
                            .arg(stopHz)
                            .arg(points);

    if (!sendCommand(cmd))
    {
        qDebug() << "Failed to send scanraw";
        return spec;
    }
    clearPendingInput();


    // The device echoes commands, so the command will be part of the output buffer
    // 4 is for "ch> " that is printed as well
    int echoLength = 4 + cmd.length() + commandEol.length();

    int pointsBytes = 3 * points; // each data point is x LSB MSB, total 3 bytes

    QByteArray raw = readExact( echoLength + 1 + pointsBytes + 1 + 4, 15000);
    raw.remove(0, echoLength);
    if (raw.size() != (1 + pointsBytes + 1 + 4))
    {
        qDebug() << "scanraw: incomplete frame, got" << raw.size() << "bytes";
        return spec;
    }

    if (raw.front() != '{' || raw.at(1 + pointsBytes) != '}')
    {
        qDebug() << "scanraw: bad framing";
        return spec;
    }

    spec.freq.reserve(points);
    spec.power.reserve(points);

    const double stepHz = double(stopHz - startHz) / double(points - 1);

    int pos = 1;
    for (int i = 0; i < points; ++i)
    {
        const char tag = raw.at(pos++);
        if (tag != 'x')
        {
            qDebug() << "scanraw: unexpected sample tag at index" << i << ":" << tag;
            spec.freq.clear();
            spec.power.clear();
            return spec;
        }

        const quint8 lsb = static_cast<quint8>(raw.at(pos++));
        const quint8 msb = static_cast<quint8>(raw.at(pos++));
        const quint16 v = (quint16(msb) << 8) | quint16(lsb);

        const float dbm = float(double(v) / 32.0 - double(currentSetup.rawOffsetDb));
        const float freqHz = float(double(startHz) + stepHz * double(i));

        spec.freq.push_back(freqHz);
        spec.power.push_back(dbm);
    }

    return spec;
}

void TinySA::pollConnection()
{
    if (isConnected)
        return;

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
                clearPendingInput();
            }
            else
            {
                qDebug() << "Open failed:" << port.errorString();
                isConnected = false;
            }

            return;
        }
    }
}

void TinySA::onSerialReadyRead()
{
    portBuffer += port.readAll();
}

void TinySA::onPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError)
    {
        qDebug() << "Serial disconnected:" << port.errorString();
        isConnected = false;
        if (port.isOpen())
            port.close();
        portBuffer.clear();
    }
}

bool TinySA::sendCommand(const QString &cmd)
{
    if (!isOpen())
        return false;

    const QByteArray bytes = cmd.toUtf8() + commandEol;
    const qint64 written = port.write(bytes);
    if (written != bytes.size())
        return false;

    return port.waitForBytesWritten(1000);
}

bool TinySA::sendCommandExpectQuiet(const QString &cmd, int settleMs)
{
    clearPendingInput();

    if (!sendCommand(cmd))
        return false;

    // The USB doc does not define a universal textual ACK/response format for all commands,
    // so we avoid parsing undocumented replies here and just let the UART settle.
    if (settleMs > 0)
        QThread::msleep(settleMs);

    return true;
}

QByteArray TinySA::readExact(int byteCount, int timeoutMs)
{
    QElapsedTimer t;
    t.start();

    while (portBuffer.size() < byteCount && t.elapsed() < timeoutMs)
    {
        QCoreApplication::processEvents();
    }

    return portBuffer;
}

QByteArray TinySA::readUntilByte(char terminator, int timeoutMs)
{
    QByteArray out;

    QElapsedTimer t;
    t.start();

    while (t.elapsed() < timeoutMs)
    {
        if (port.bytesAvailable() > 0 || port.waitForReadyRead(50))
        {
            out += port.readAll();
            if (out.contains(terminator))
                return out;
        }
        else
        {
            QCoreApplication::processEvents();
        }
    }

    return out;
}

void TinySA::clearPendingInput()
{
    portBuffer.clear();
    if (port.isOpen())
    {
        port.clear(QSerialPort::Input);
        while (port.waitForReadyRead(10))
            port.readAll();
    }
}

bool TinySA::setMode(const QString &mode)
{
    if (mode != "low" && mode != "high")
    {
        qDebug() << "Unsupported mode:" << mode;
        return false;
    }

    return sendCommandExpectQuiet(QString("mode %1 input").arg(mode));
}

bool TinySA::setAttenuationAutoOrDb(int attenuationDb)
{
    if (attenuationDb < 0)
        return sendCommandExpectQuiet("attenuate auto");

    if (attenuationDb > 31)
    {
        qDebug() << "attenuation out of range";
        return false;
    }

    return sendCommandExpectQuiet(QString("attenuate %1").arg(attenuationDb));
}

bool TinySA::setRbwAutoOrKHz(int rbwKHz)
{
    if (rbwKHz < 0)
        return sendCommandExpectQuiet("rbw auto");

    if (rbwKHz < 3 || rbwKHz > 600)
    {
        qDebug() << "RBW out of range";
        return false;
    }

    return sendCommandExpectQuiet(QString("rbw %1").arg(rbwKHz));
}

bool TinySA::setRepeat(int repeatCount)
{
    if (repeatCount < 1 || repeatCount > 1000)
    {
        qDebug() << "repeat out of range";
        return false;
    }

    return sendCommandExpectQuiet(QString("repeat %1").arg(repeatCount));
}

bool TinySA::setSpur(bool enabled)
{
    return sendCommandExpectQuiet(QString("spur %1").arg(enabled ? "on" : "off"));
}

bool TinySA::setExtGain(float db)
{
    if (db < -100.0f || db > 100.0f)
    {
        qDebug() << "ext_gain out of range";
        return false;
    }

    return sendCommandExpectQuiet(QString("ext_gain %1").arg(db, 0, 'f', 1));
}

bool TinySA::setSweepCenter(quint64 centerHz)
{
    return sendCommandExpectQuiet(QString("sweep center %1").arg(centerHz));
}

bool TinySA::setSweepSpan(quint64 spanHz)
{
    return sendCommandExpectQuiet(QString("sweep span %1").arg(spanHz));
}
