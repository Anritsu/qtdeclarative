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

#ifndef QQUICKPOSITIONERS_P_P_H
#define QQUICKPOSITIONERS_P_P_H

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

#include <QtQuick/private/qtquickglobal_p.h>

QT_REQUIRE_CONFIG(quick_positioners);

#include "qquickpositioners_p.h"
#include "qquickimplicitsizeitem_p_p.h"

#include <private/qlazilyallocated_p.h>

#include <QtCore/qobject.h>
#include <QtCore/qstring.h>
#include <QtCore/qtimer.h>

QT_BEGIN_NAMESPACE

class QQuickItemViewTransitioner;

class QQuickBasePositionerPrivate : public QQuickImplicitSizeItemPrivate, public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickBasePositioner)

public:
    struct ExtraData {
        ExtraData();

        qreal padding;
        qreal topPadding;
        qreal leftPadding;
        qreal rightPadding;
        qreal bottomPadding;
        bool explicitTopPadding : 1;
        bool explicitLeftPadding : 1;
        bool explicitRightPadding : 1;
        bool explicitBottomPadding : 1;
    };
    QLazilyAllocated<ExtraData> extra;

    QQuickBasePositionerPrivate()
        : spacing(0), type(QQuickBasePositioner::None)
        , transitioner(0), positioningDirty(false)
        , doingPositioning(false), anchorConflict(false), layoutDirection(Qt::LeftToRight)

    {
    }

    void init(QQuickBasePositioner::PositionerType at)
    {
        type = at;
    }

    qreal spacing;

    QQuickBasePositioner::PositionerType type;
    QQuickItemViewTransitioner *transitioner;

    void watchChanges(QQuickItem *other);
    void unwatchChanges(QQuickItem* other);
    void setPositioningDirty() {
        Q_Q(QQuickBasePositioner);
        if (!positioningDirty) {
            positioningDirty = true;
            q->polish();
        }
    }

    bool positioningDirty : 1;
    bool doingPositioning : 1;
    bool anchorConflict : 1;

    Qt::LayoutDirection layoutDirection;

    void mirrorChange() override {
        effectiveLayoutDirectionChange();
    }
    bool isLeftToRight() const {
        if (type == QQuickBasePositioner::Vertical)
            return true;
        else
            return effectiveLayoutMirror ? layoutDirection == Qt::RightToLeft : layoutDirection == Qt::LeftToRight;
    }

    void itemSiblingOrderChanged(QQuickItem* other) override
    {
        Q_UNUSED(other);
        setPositioningDirty();
    }

    void itemGeometryChanged(QQuickItem *, QQuickGeometryChange change, const QRectF &) override
    {
        if (change.sizeChange())
            setPositioningDirty();
    }

    void itemVisibilityChanged(QQuickItem *) override
    {
        setPositioningDirty();
    }

    void itemDestroyed(QQuickItem *item) override
    {
        Q_Q(QQuickBasePositioner);
        int index = q->positionedItems.find(QQuickBasePositioner::PositionedItem(item));
        if (index >= 0)
            q->removePositionedItem(&q->positionedItems, index);
    }

    static Qt::LayoutDirection getLayoutDirection(const QQuickBasePositioner *positioner)
    {
        return positioner->d_func()->layoutDirection;
    }

    static Qt::LayoutDirection getEffectiveLayoutDirection(const QQuickBasePositioner *positioner)
    {
        if (positioner->d_func()->effectiveLayoutMirror)
            return positioner->d_func()->layoutDirection == Qt::RightToLeft ? Qt::LeftToRight : Qt::RightToLeft;
        else
            return positioner->d_func()->layoutDirection;
    }

    virtual void effectiveLayoutDirectionChange()
    {
    }

    inline qreal padding() const { return extra.isAllocated() ? extra->padding : 0.0; }
    void setTopPadding(qreal value, bool reset = false);
    void setLeftPadding(qreal value, bool reset = false);
    void setRightPadding(qreal value, bool reset = false);
    void setBottomPadding(qreal value, bool reset = false);
};

QT_END_NAMESPACE

#endif // QQUICKPOSITIONERS_P_P_H
