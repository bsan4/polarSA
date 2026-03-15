#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "src/tinySA/tinySA.h"
#include "src/Rotator/rotator.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Q_SLOT void triggerSingleCapture();
    Q_SLOT void triggerSequenceCapture();
    Q_SLOT void triggerStopCapture();
    void sendSAParameters();
    void sendRotatorParameters();
    Ui::MainWindow *ui;
    TinySA tinySA;
    Rotator rotator;
    bool stopCapture = false;

    const QList<QPair<QString, float>> rbwMap = {
        QPair<QString, float>("Auto", -1),
        QPair<QString, float>("200Hz", 0.2),
        QPair<QString, float>("1kHz", 1),
        QPair<QString, float>("3kHz", 3),
        QPair<QString, float>("10kHz", 10),
        QPair<QString, float>("30kHz", 30),
        QPair<QString, float>("100kHz", 100),
        QPair<QString, float>("600kHz", 600),
        QPair<QString, float>("850kHz", 850)
    };
};
#endif // MAINWINDOW_H
