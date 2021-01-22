/****************************************************************************
**
** Copyright (C) 2016 BlackBerry Ltd.
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQuick module of the Qt Toolkit.
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

#ifndef QQUICKOPENGLINFO_P_H
#define QQUICKOPENGLINFO_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <QtGui/qsurfaceformat.h>
#include <QtQml/qqml.h>

QT_BEGIN_NAMESPACE

class QQuickItem;
class QQuickWindow;

class QQuickOpenGLInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int majorVersion READ majorVersion NOTIFY majorVersionChanged FINAL)
    Q_PROPERTY(int minorVersion READ minorVersion NOTIFY minorVersionChanged FINAL)
    Q_PROPERTY(ContextProfile profile READ profile NOTIFY profileChanged FINAL)
    Q_PROPERTY(RenderableType renderableType READ renderableType NOTIFY renderableTypeChanged FINAL)

    QML_NAMED_ELEMENT(OpenGLInfo)
    QML_UNCREATABLE("OpenGLInfo is only available via attached properties.")
    QML_ADDED_IN_MINOR_VERSION(4)
    QML_ATTACHED(QQuickOpenGLInfo)

public:
    QQuickOpenGLInfo(QQuickItem *item = 0);

    int majorVersion() const;
    int minorVersion() const;

    // keep in sync with QSurfaceFormat::OpenGLContextProfile
    enum ContextProfile {
        NoProfile = QSurfaceFormat::NoProfile,
        CoreProfile = QSurfaceFormat::CoreProfile,
        CompatibilityProfile = QSurfaceFormat::CompatibilityProfile
    };
    Q_ENUM(ContextProfile)
    ContextProfile profile() const;

    // keep in sync with QSurfaceFormat::RenderableType
    enum RenderableType {
        Unspecified = QSurfaceFormat::DefaultRenderableType,
        OpenGL = QSurfaceFormat::OpenGL,
        OpenGLES = QSurfaceFormat::OpenGLES
    };
    Q_ENUM(RenderableType)
    RenderableType renderableType() const;

    static QQuickOpenGLInfo *qmlAttachedProperties(QObject *object);

Q_SIGNALS:
    void majorVersionChanged();
    void minorVersionChanged();
    void profileChanged();
    void renderableTypeChanged();

private Q_SLOTS:
    void updateFormat();
    void setWindow(QQuickWindow *window);

private:
    QPointer<QQuickWindow> m_window;
    int m_majorVersion;
    int m_minorVersion;
    ContextProfile m_profile;
    RenderableType m_renderableType;
};

QT_END_NAMESPACE

#endif // QQUICKOPENGLINFO_P_H
