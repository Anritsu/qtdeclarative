requires(qtHaveModule(opengl))
requires(qtHaveModule(widgets))

QT += opengl widgets
CONFIG += console
macx:CONFIG -= app_bundle

SOURCES += paintbenchmark.cpp
