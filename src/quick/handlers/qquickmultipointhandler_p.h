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

#ifndef QQUICKPOINTERMULTIHANDLER_H
#define QQUICKPOINTERMULTIHANDLER_H

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

#include "qquickitem.h"
#include "qevent.h"
#include "qquickhandlerpoint_p.h"
#include "qquickpointerdevicehandler_p.h"

QT_BEGIN_NAMESPACE

class QQuickMultiPointHandlerPrivate;

class Q_QUICK_PRIVATE_EXPORT QQuickMultiPointHandler : public QQuickPointerDeviceHandler
{
    Q_OBJECT
    Q_PROPERTY(int minimumPointCount READ minimumPointCount WRITE setMinimumPointCount NOTIFY minimumPointCountChanged)
    Q_PROPERTY(int maximumPointCount READ maximumPointCount WRITE setMaximumPointCount NOTIFY maximumPointCountChanged)
    Q_PROPERTY(QQuickHandlerPoint centroid READ centroid NOTIFY centroidChanged)

public:
    explicit QQuickMultiPointHandler(QQuickItem *parent = nullptr, int minimumPointCount = 2, int maximumPointCount = -1);

    int minimumPointCount() const;
    void setMinimumPointCount(int c);

    int maximumPointCount() const;
    void setMaximumPointCount(int maximumPointCount);

    const QQuickHandlerPoint &centroid() const;

signals:
    void minimumPointCountChanged();
    void maximumPointCountChanged();
    void centroidChanged();

protected:
    struct PointData {
        PointData() : id(0), angle(0) {}
        PointData(quint64 id, qreal angle) : id(id), angle(angle) {}
        quint64 id;
        qreal angle;
    };

    bool wantsPointerEvent(QQuickPointerEvent *event) override;
    void handlePointerEventImpl(QQuickPointerEvent *event) override;
    void onActiveChanged() override;
    void onGrabChanged(QQuickPointerHandler *grabber, QQuickEventPoint::GrabTransition transition, QQuickEventPoint *point) override;
    QVector<QQuickHandlerPoint> &currentPoints();
    QQuickHandlerPoint &mutableCentroid();
    bool hasCurrentPoints(QQuickPointerEvent *event);
    QVector<QQuickEventPoint *> eligiblePoints(QQuickPointerEvent *event);
    qreal averageTouchPointDistance(const QPointF &ref);
    qreal averageStartingDistance(const QPointF &ref);
    qreal averageTouchPointAngle(const QPointF &ref);
    qreal averageStartingAngle(const QPointF &ref);
    QVector<PointData> angles(const QPointF &ref) const;
    static qreal averageAngleDelta(const QVector<PointData> &old, const QVector<PointData> &newAngles);
    void acceptPoints(const QVector<QQuickEventPoint *> &points);
    bool grabPoints(const QVector<QQuickEventPoint *> &points);
    void moveTarget(QPointF pos);

    Q_DECLARE_PRIVATE(QQuickMultiPointHandler)
};

QT_END_NAMESPACE

QML_DECLARE_TYPE(QQuickMultiPointHandler)

#endif // QQUICKPOINTERMULTIHANDLER_H
