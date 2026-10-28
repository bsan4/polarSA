QT       += core gui printsupport serialport opengl svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DEFINES += QCUSTOMPLOT_USE_OPENGL

SOURCES += \
    main.cpp \
    lib/QCustomPlot/qcustomplot.cpp \
    src/Analysis/patternanalysis.cpp \
    src/ExportDialog/exportdialog.cpp \
    src/Report/pdfattachments.cpp \
    src/Report/pdfreport.cpp \
    src/TraceDialog/tracedialog.cpp \
    src/PowerPlot/powerplot.cpp \
    src/Rotator/rotator.cpp \
    src/SpectrumPlot/spectrumplot.cpp \
    src/tinySA/tinySA.cpp \
    mainwindow.cpp

HEADERS += \
    inc/defines.h \
    lib/QCustomPlot/qcustomplot.h \
    src/Analysis/patternanalysis.h \
    src/ExportDialog/exportdialog.h \
    src/Report/pdfattachments.h \
    src/Report/pdfreport.h \
    src/TraceDialog/tracedialog.h \
    src/PowerPlot/powerplot.h \
    src/Rotator/rotator.h \
    src/SpectrumPlot/spectrumplot.h \
    src/tinySA/tinySA.h \
    mainwindow.h

FORMS += \
    mainwindow.ui

win32-g++:CONFIG(debug, debug|release) {
    QMAKE_CXXFLAGS += -Wa,-mbig-obj
}
win32:LIBS += opengl32.lib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    rsc/polarsa.ico \
    rsc/polarsa_notxt.ico \


RC_ICONS = rsc/polarsa_notxt.ico

RESOURCES += \
    rsc/rsc.qrc
