/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Labs Controls module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qquickstyle_p.h"

#include <QtQuick/private/qquickitem_p.h>

QT_BEGIN_NAMESPACE

static QQuickStyle *attachedStyle(const QMetaObject *type, QObject *object, bool create = false)
{
    if (!object)
        return Q_NULLPTR;
    int idx = -1;
    return qobject_cast<QQuickStyle *>(qmlAttachedPropertiesObject(&idx, object, type, create));
}

static QQuickStyle *findParentStyle(const QMetaObject *type, QObject *object)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (item) {
        // lookup parent items
        QQuickItem *parent = item->parentItem();
        while (parent) {
            QQuickStyle *style = attachedStyle(type, parent);
            if (style)
                return style;
            parent = parent->parentItem();
        }

        // fallback to item's window
        QQuickWindow *window = item->window();
        if (window) {
            QQuickStyle *style = attachedStyle(type, window);
            if (style)
                return style;
        }
    }

    // lookup parent window
    QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
    if (window) {
        QQuickWindow *parentWindow = qobject_cast<QQuickWindow *>(window->parent());
        if (parentWindow) {
            QQuickStyle *style = attachedStyle(type, window);
            if (style)
                return style;
        }
    }

    // fallback to engine (global)
    if (object) {
        QQmlEngine *engine = qmlEngine(object);
        if (engine) {
            QByteArray name = QByteArray("_q_") + type->className();
            QQuickStyle *style = engine->property(name).value<QQuickStyle*>();
            if (!style) {
                style = attachedStyle(type, engine, true);
                engine->setProperty(name, QVariant::fromValue(style));
            }
            return style;
        }
    }

    return Q_NULLPTR;
}

static QList<QQuickStyle *> findChildStyles(const QMetaObject *type, QObject *object)
{
    QList<QQuickStyle *> children;

    QQuickItem *item = qobject_cast<QQuickItem *>(object);
    if (!item) {
        QQuickWindow *window = qobject_cast<QQuickWindow *>(object);
        if (window) {
            item = window->contentItem();

            foreach (QObject *child, window->children()) {
                QQuickWindow *childWindow = qobject_cast<QQuickWindow *>(child);
                if (childWindow) {
                    QQuickStyle *style = attachedStyle(type, childWindow);
                    if (style)
                        children += style;
                }
            }
        }
    }

    if (item) {
        foreach (QQuickItem *child, item->childItems()) {
            QQuickStyle *style = attachedStyle(type, child);
            if (style)
                children += style;
            else
                children += findChildStyles(type, child);
        }
    }

    return children;
}

QQuickStyle::QQuickStyle(QObject *parent) : QObject(parent)
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent);
    if (item)
        QQuickItemPrivate::get(item)->addItemChangeListener(this, QQuickItemPrivate::Parent);
}

QQuickStyle::~QQuickStyle()
{
    QQuickItem *item = qobject_cast<QQuickItem *>(parent());
    if (item)
        QQuickItemPrivate::get(item)->removeItemChangeListener(this, QQuickItemPrivate::Parent);

    setParentStyle(Q_NULLPTR);
}

QList<QQuickStyle *> QQuickStyle::childStyles() const
{
    return m_childStyles;
}

QQuickStyle *QQuickStyle::parentStyle() const
{
    return m_parentStyle;
}

void QQuickStyle::setParentStyle(QQuickStyle *style)
{
    if (m_parentStyle != style) {
        QQuickStyle *oldParent = m_parentStyle;
        if (m_parentStyle)
            m_parentStyle->m_childStyles.removeOne(this);
        m_parentStyle = style;
        if (style)
            style->m_childStyles.append(this);
        parentStyleChange(style, oldParent);
    }
}

void QQuickStyle::init()
{
    QQuickStyle *parentStyle = findParentStyle(metaObject(), parent());
    if (parentStyle)
        setParentStyle(parentStyle);

    QList<QQuickStyle *> children = findChildStyles(metaObject(), parent());
    foreach (QQuickStyle *child, children)
        child->setParentStyle(this);
}

void QQuickStyle::parentStyleChange(QQuickStyle *newParent, QQuickStyle *oldParent)
{
    Q_UNUSED(newParent);
    Q_UNUSED(oldParent);
}

void QQuickStyle::itemParentChanged(QQuickItem *item, QQuickItem *parent)
{
    QQuickStyle *style = attachedStyle(metaObject(), item);
    if (style) {
        QQuickStyle *parentStyle = findParentStyle(metaObject(), parent);
        if (parentStyle)
            style->setParentStyle(parentStyle);
    }
}

QT_END_NAMESPACE
