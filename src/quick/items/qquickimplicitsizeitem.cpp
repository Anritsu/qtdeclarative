/****************************************************************************
**
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

#include "qquickimplicitsizeitem_p.h"
#include "qquickimplicitsizeitem_p_p.h"

QT_BEGIN_NAMESPACE

/*!
    \internal

    QQuickImplicitSizeItem redefines the implicitWidth and implicitHeight
    properties as readonly, as some items (e.g. Image, where the implicit size
    represents the real size of the image) should not be able to have their
    implicit size modified.
*/

QQuickImplicitSizeItem::QQuickImplicitSizeItem(QQuickImplicitSizeItemPrivate &dd, QQuickItem *parent)
    : QQuickItem(dd, parent)
{
}

QT_END_NAMESPACE

#include "moc_qquickimplicitsizeitem_p.cpp"
