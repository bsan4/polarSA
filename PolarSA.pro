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
    src/Corrections/corrections.cpp \
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
    src/Corrections/corrections.h \
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

# EMC limit files and calibration template shipped with the application:
# the app looks for limits/ and calibration/ next to the executable, then in the source tree.
DEFINES += POLARSA_SOURCE_DIR=\\\"$$PWD\\\"
DISTFILES += $$files(limits/*) $$files(calibration/*)
limits.files = $$files(limits/*)
limits.path = $$target.path/limits
calibration.files = $$files(calibration/*)
calibration.path = $$target.path/calibration
!isEmpty(target.path): INSTALLS += limits calibration

# Windows Release: standalone <build-dir>/deploy/ folder (exe, Qt runtime, limits/, calibration/)
win32:CONFIG(release, debug|release) {
    QMAKE_POST_LINK += $$shell_quote($$shell_path($$PWD/deploy_win.bat)) \
        $$shell_quote($$shell_path($$OUT_PWD)) \
        $$TARGET \
        $$shell_quote($$shell_path($$[QT_INSTALL_BINS])) \
        $$shell_quote($$shell_path($$PWD)) \
        $$shell_quote($$shell_path($$OUT_PWD/deploy))
}
DISTFILES += deploy_win.bat

RESOURCES += \
    rsc/rsc.qrc
