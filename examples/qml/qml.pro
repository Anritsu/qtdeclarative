TEMPLATE = subdirs
QT_FOR_CONFIG += qml

qtHaveModule(quick) {
    SUBDIRS += \
        qmlextensionplugins \
        xmlhttprequest
}

SUBDIRS += \
          referenceexamples \
          tutorials \
          shell

EXAMPLE_FILES = \
    dynamicscene \
    qml-i18n \
    locale
