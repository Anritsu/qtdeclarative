QT += qml quick

CONFIG += qmltypes
QML_IMPORT_NAME = SceneGraphRendering
QML_IMPORT_MAJOR_VERSION = 2

HEADERS += customrenderitem.h \
           openglrenderer.h \
           softwarerenderer.h

SOURCES += customrenderitem.cpp \
           openglrenderer.cpp \
           softwarerenderer.cpp \
           main.cpp

RESOURCES += rendernode.qrc

target.path = $$[QT_INSTALL_EXAMPLES]/quick/scenegraph/rendernode
INSTALLS += target

OTHER_FILES += \
    main.qml

macos {
    HEADERS += metalrenderer.h
    SOURCES += metalrenderer.mm
    LIBS += -framework Metal -framework AppKit
}
