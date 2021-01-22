/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qqmlextensionplugin.h"
#include "qqmlextensionplugin_p.h"

QT_BEGIN_NAMESPACE

/*!
    \since 5.14
    \inmodule QtQml
    \class QQmlEngineExtensionPlugin
    \brief The QQmlEngineExtensionPlugin class provides an abstract base for custom QML extension
           plugins.

    \ingroup plugins

    \include qqmlextensionplugin.qdocinc

    The \l {Writing QML Extensions with C++} tutorial also contains a chapter
    on creating QML plugins.

    \sa QQmlEngine::importPlugin(), {How to Create Qt Plugins}
*/

/*!
    \fn void QQmlExtensionPlugin::registerTypes(const char *uri)
    \internal

    Registers the QML types in the given \a uri. Subclasses should implement
    this to call qmlRegisterType() for all types which are provided by the extension
    plugin.

    The \a uri is an identifier for the plugin generated by the QML engine
    based on the name and path of the extension's plugin library.
*/

/*!
    \internal
*/
QQmlExtensionPlugin::QQmlExtensionPlugin(QObject *parent)
    : QObject(*(new QQmlExtensionPluginPrivate), parent)
{
}

/*!
    Constructs a QML extension plugin with the given \a parent.

    Note that this constructor is invoked automatically by the
    Q_PLUGIN_METADATA() macro, so there is no need for calling it
    explicitly.
 */
QQmlEngineExtensionPlugin::QQmlEngineExtensionPlugin(QObject *parent)
    : QObject(parent)
{
}


/*!
  \internal
 */
QQmlExtensionPlugin::~QQmlExtensionPlugin() = default;

/*!
  \internal
 */
QQmlEngineExtensionPlugin::~QQmlEngineExtensionPlugin() = default;

/*!
    \since 5.1
    \internal
    \brief Returns the URL of the directory from which the extension is loaded.

    This is useful when the plugin also needs to load QML files or other
    assets from the same directory.
*/
QUrl QQmlExtensionPlugin::baseUrl() const
{
    Q_D(const QQmlExtensionPlugin);
    return d->baseUrl;
}

/*!
    \internal
*/

void QQmlExtensionPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
}

/*!
    Initializes the extension from the \a uri using the \a engine. Here an application
    plugin might, for example, expose some data or objects to QML,
    as context properties on the engine's root context.
 */
void QQmlEngineExtensionPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(engine);
    Q_UNUSED(uri);
}

/*!
  \class QQmlExtensionInterface
  \internal
  \inmodule QtQml
*/

/*!
  \class QQmlTypesExtensionInterface
  \internal
  \inmodule QtQml
*/

/*!
  \class QQmlEngineExtensionInterface
  \internal
  \inmodule QtQml
*/

QT_END_NAMESPACE

#include "moc_qqmlextensionplugin.cpp"
