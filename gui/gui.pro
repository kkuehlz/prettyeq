QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    collisionmanager.cpp \
    curvepoint.cpp \
    eqhoverer.cpp \
    filtercurve.cpp \
    frequencytick.cpp \
    frequencytickbuilder.cpp \
    gui.cpp \
    highshelfcurve.cpp \
    lowshelfcurve.cpp \
    main.cpp \
    peakingcurve.cpp \
    prettygraphicsscene.cpp \
    runguard.cpp \
    shelfcurve.cpp \
    spectrumanalyzer.cpp \
    unixsignalhandler.cpp

HEADERS += \
    collisionmanager.h \
    curvepoint.h \
    eqhoverer.h \
    filtercurve.h \
    frequencytick.h \
    frequencytickbuilder.h \
    gui.h \
    highshelfcurve.h \
    lowshelfcurve.h \
    macro.h \
    peakingcurve.h \
    prettygraphicsscene.h \
    prettyshim.h \
    ringbuffer.h \
    runguard.h \
    shelfcurve.h \
    spectrumanalyzer.h \
    unixsignalhandler.h

FORMS += \
    gui.ui

QMAKE_CXXFLAGS_WARN_ON += -Wno-unused-parameter
INCLUDEPATH += '../equalizer'
unix:LIBS += -L ../equalizer -lequalizer -lm -lpulse -lpthread -ffast-math -fopenmp
TARGET = ../prettyeq

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    resources.qrc
