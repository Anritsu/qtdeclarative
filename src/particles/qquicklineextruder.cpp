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
#include "qquicklineextruder_p.h"
#include <QRandomGenerator>
#include <cmath>

/*!
    \qmltype LineShape
    \instantiates QQuickLineExtruder
    \inqmlmodule QtQuick.Particles
    \inherits Shape
    \brief Represents a line for affectors and emitters.
    \ingroup qtquick-particles

*/

/*!
    \qmlproperty bool QtQuick.Particles::LineShape::mirrored

    By default, the line goes from (0,0) to (width, height) of the item that
    this shape is being applied to.

    If mirrored is set to true, this will be mirrored along the y axis.
    The line will then go from (0,height) to (width, 0).
*/

QQuickLineExtruder::QQuickLineExtruder(QObject *parent) :
    QQuickParticleExtruder(parent), m_mirrored(false)
{
}

QPointF QQuickLineExtruder::extrude(const QRectF &r)
{
    qreal x,y;
    if (!r.height()){
        x = r.width() * QRandomGenerator::global()->generateDouble();
        y = 0;
    }else{
        y = r.height() * QRandomGenerator::global()->generateDouble();
        if (!r.width()){
            x = 0;
        }else{
            x = r.width()/r.height() * y;
            if (m_mirrored)
                x = r.width() - x;
        }
    }
    return QPointF(x,y);
}

#include "moc_qquicklineextruder_p.cpp"
