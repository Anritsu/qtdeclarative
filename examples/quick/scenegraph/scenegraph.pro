TEMPLATE = subdirs

qtConfig(opengl(es1|es2)?) {
    SUBDIRS += \
            graph \
            simplematerial \
            sgengine \
            textureinsgnode \
            openglunderqml \
            textureinthread \
            twotextureproviders
}

SUBDIRS += \
        customgeometry \
        rendernode \
        threadedanimation

macos {
    SUBDIRS += metalunderqml
}

win32 {
    SUBDIRS += d3d11underqml
}

EXAMPLE_FILES += \
    shared
