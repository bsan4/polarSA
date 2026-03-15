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
    void sendSAParameters();
    void sendRotatorParameters();
    Ui::MainWindow *ui;
    TinySA tinySA;
    Rotator rotator;
};
#endif // MAINWINDOW_H
