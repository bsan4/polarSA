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

    const QString cmd = QString("scanraw %1 %2 %3")
                            .arg(startHz)
                            .arg(stopHz)
                            .arg(points);

    // Remove stale leftovers before issuing the binary command.
    clearPendingInput();

    if (!sendCommand(cmd))
    {
        qDebug() << "Failed to send scanraw";
        return spec;
    }

    // Expected raw frame:
    // '{' ('x' LSB MSB) * points '}'
    const int expectedSize = 1 + 3 * points + 1;

    // 1) Synchronize on the opening brace, discarding text echoes/prompts/garbage
    QElapsedTimer syncTimer;
    syncTimer.start();

    while (syncTimer.elapsed() < 15000)
    {
        int idx = portBuffer.indexOf('{');
        if (idx >= 0)
        {
            if (idx > 0)
                portBuffer.remove(0, idx);
            break;
        }

        // No start byte yet. Discard stale text already in buffer so we don't
        // keep re-checking the same "ch> ..." forever.
        if (!portBuffer.isEmpty())
            portBuffer.clear();

        if (port.waitForReadyRead(50))
            portBuffer += port.readAll();
        else
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }

    if (portBuffer.isEmpty() || portBuffer.front() != '{')
    {
        qDebug() << "scanraw: did not find frame start. buffer =" << portBuffer;
        return spec;
    }

    // 2) Wait for the exact packet length from the opening '{'
    QElapsedTimer dataTimer;
    dataTimer.start();

    while (portBuffer.size() < expectedSize && dataTimer.elapsed() < 15000)
    {
        if (port.waitForReadyRead(50))
            portBuffer += port.readAll();
        else
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }

    if (portBuffer.size() < expectedSize)
    {
        qDebug() << "scanraw: incomplete frame, got" << portBuffer.size()
        << "expected at least" << expectedSize;
        return spec;
    }

    QByteArray raw = portBuffer.left(expectedSize);
    portBuffer.remove(0, expectedSize);

    if (raw.front() != '{' || raw.back() != '}')
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
        if (pos + 2 >= raw.size())
        {
            qDebug() << "scanraw: truncated payload at point" << i;
            spec.freq.clear();
            spec.power.clear();
            return spec;
        }

        const char tag = raw.at(pos++);
        if (tag != 'x')
        {
            qDebug() << "scanraw: unexpected sample tag at index" << i << ":" << int(quint8(tag));
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
                emit connected();
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

void TinySA::onSerialReadyRead()
{
    portBuffer += port.readAll();
}

void TinySA::onPortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError ||
        error == QSerialPort::DeviceNotFoundError ||
        error == QSerialPort::PermissionError)
    {
        qDebug() << "Serial disconnected/error:" << port.errorString();
        isConnected = false;
        if (port.isOpen())
            port.close();
        portBuffer.clear();
        emit disconnected();
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

    // Drain command echo / prompt / any textual reply for a short time.
    if (settleMs > 0)
    {
        QElapsedTimer t;
        t.start();

        while (t.elapsed() < settleMs)
        {
            if (port.waitForReadyRead(10))
                portBuffer += port.readAll();

            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
        }
    }

    // We do not want setup-command echoes to pollute the next binary scanraw read.
    portBuffer.clear();
    return true;
}

QByteArray TinySA::readExact(int byteCount, int timeoutMs)
{
    QElapsedTimer t;
    t.start();

    while (portBuffer.size() < byteCount && t.elapsed() < timeoutMs)
    {
        if (port.waitForReadyRead(50))
            portBuffer += port.readAll();
        else
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }

    QByteArray out = portBuffer.left(byteCount);
    portBuffer.remove(0, out.size());
    return out;
}

QByteArray TinySA::readUntil(const char endOfText, int timeoutMs)
{
    QElapsedTimer t;
    t.start();

    while (!portBuffer.contains(endOfText) && t.elapsed() < timeoutMs)
    {
        if (port.waitForReadyRead(50))
            portBuffer += port.readAll();
        else
            QCoreApplication::processEvents(QEventLoop::AllEvents, 5);
    }

    int idx = portBuffer.indexOf(endOfText);
    if (idx < 0)
        return QByteArray();

    QByteArray out = portBuffer.left(idx + 1);
    portBuffer.remove(0, idx + 1);
    return out;
}

QByteArray TinySA::readUntilByte(char terminator, int timeoutMs)
{
    return readUntil(terminator, timeoutMs);
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

bool TinySA::setRbwAutoOrKHz(float rbwKHz)
{
    if (rbwKHz < 0)
        return sendCommandExpectQuiet("rbw auto");

    if (rbwKHz < 0.2 || rbwKHz > 850)
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
