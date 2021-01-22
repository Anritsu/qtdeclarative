/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Quick Layouts module of the Qt Toolkit.
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

#ifndef QQUICKLINEARLAYOUT_P_H
#define QQUICKLINEARLAYOUT_P_H

#include "qquicklayout_p.h"
#include "qquickgridlayoutengine_p.h"

QT_BEGIN_NAMESPACE

/**********************************
 **
 ** QQuickGridLayoutBase
 **
 **/
class QQuickGridLayoutBasePrivate;

class QQuickGridLayoutBase : public QQuickLayout
{
    Q_OBJECT

    Q_PROPERTY(Qt::LayoutDirection layoutDirection READ layoutDirection WRITE setLayoutDirection NOTIFY layoutDirectionChanged REVISION 1)
    QML_ANONYMOUS
    QML_ADDED_IN_MINOR_VERSION(1)

public:

    QQuickGridLayoutBase();

    explicit QQuickGridLayoutBase(QQuickGridLayoutBasePrivate &dd,
                                  Qt::Orientation orientation,
                                  QQuickItem *parent = 0);

    ~QQuickGridLayoutBase();
    void componentComplete() override;
    void invalidate(QQuickItem *childItem = 0) override;
    Qt::Orientation orientation() const;
    void setOrientation(Qt::Orientation orientation);
    QSizeF sizeHint(Qt::SizeHint whichSizeHint) const override;
    Qt::LayoutDirection layoutDirection() const;
    void setLayoutDirection(Qt::LayoutDirection dir);
    Qt::LayoutDirection effectiveLayoutDirection() const;
    void setAlignment(QQuickItem *item, Qt::Alignment align) override;

    /* QQuickItemChangeListener */
    void itemDestroyed(QQuickItem *item) override;
    void itemVisibilityChanged(QQuickItem *item) override;

protected:
    void updateLayoutItems() override;
    QQuickItem *itemAt(int index) const override;
    int itemCount() const override;

    void rearrange(const QSizeF &size) override;
    virtual void insertLayoutItems() {}

signals:
    Q_REVISION(1) void layoutDirectionChanged();

private:
    void removeGridItem(QGridLayoutItem *gridItem);
    Q_DECLARE_PRIVATE(QQuickGridLayoutBase)
};

class QQuickLayoutStyleInfo;

class QQuickGridLayoutBasePrivate : public QQuickLayoutPrivate
{
    Q_DECLARE_PUBLIC(QQuickGridLayoutBase)

public:
    QQuickGridLayoutBasePrivate() : m_recurRearrangeCounter(0)
                                    , m_rearranging(false)
                                    , m_updateAfterRearrange(false)
                                    , m_layoutDirection(Qt::LeftToRight)
                                    {}

    void mirrorChange() override
    {
        Q_Q(QQuickGridLayoutBase);
        q->invalidate();
    }

    QQuickGridLayoutEngine engine;
    Qt::Orientation orientation;
    unsigned m_recurRearrangeCounter : 2;
    unsigned m_rearranging : 1;
    unsigned m_updateAfterRearrange : 1;
    QVector<QQuickItem *> m_invalidateAfterRearrange;
    Qt::LayoutDirection m_layoutDirection : 2;

    QQuickLayoutStyleInfo *styleInfo;
};

/**********************************
 **
 ** QQuickGridLayout
 **
 **/
class QQuickGridLayoutPrivate;
class QQuickGridLayout : public QQuickGridLayoutBase
{
    Q_OBJECT

    Q_PROPERTY(qreal columnSpacing READ columnSpacing WRITE setColumnSpacing NOTIFY columnSpacingChanged)
    Q_PROPERTY(qreal rowSpacing READ rowSpacing WRITE setRowSpacing NOTIFY rowSpacingChanged)
    Q_PROPERTY(int columns READ columns WRITE setColumns NOTIFY columnsChanged)
    Q_PROPERTY(int rows READ rows WRITE setRows NOTIFY rowsChanged)
    Q_PROPERTY(Flow flow READ flow WRITE setFlow NOTIFY flowChanged)
    QML_NAMED_ELEMENT(GridLayout)
public:
    explicit QQuickGridLayout(QQuickItem *parent = 0);
    qreal columnSpacing() const;
    void setColumnSpacing(qreal spacing);
    qreal rowSpacing() const;
    void setRowSpacing(qreal spacing);

    int columns() const;
    void setColumns(int columns);
    int rows() const;
    void setRows(int rows);

    enum Flow { LeftToRight, TopToBottom };
    Q_ENUM(Flow)
    Flow flow() const;
    void setFlow(Flow flow);

    void insertLayoutItems() override;

signals:
    void columnSpacingChanged();
    void rowSpacingChanged();

    void columnsChanged();
    void rowsChanged();

    void flowChanged();
private:
    Q_DECLARE_PRIVATE(QQuickGridLayout)
};

class QQuickGridLayoutPrivate : public QQuickGridLayoutBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickGridLayout)
public:
    QQuickGridLayoutPrivate(): columns(-1), rows(-1), flow(QQuickGridLayout::LeftToRight) {}
    int columns;
    int rows;
    QQuickGridLayout::Flow flow;
};


/**********************************
 **
 ** QQuickLinearLayout
 **
 **/
class QQuickLinearLayoutPrivate;
class QQuickLinearLayout : public QQuickGridLayoutBase
{
    Q_OBJECT
    Q_PROPERTY(qreal spacing READ spacing WRITE setSpacing NOTIFY spacingChanged)
public:
    explicit QQuickLinearLayout(Qt::Orientation orientation,
                                QQuickItem *parent = 0);
    void insertLayoutItem(QQuickItem *item);
    qreal spacing() const;
    void setSpacing(qreal spacing);

    void insertLayoutItems() override;

signals:
    void spacingChanged();
private:
    Q_DECLARE_PRIVATE(QQuickLinearLayout)
};

class QQuickLinearLayoutPrivate : public QQuickGridLayoutBasePrivate
{
    Q_DECLARE_PUBLIC(QQuickLinearLayout)
public:
    QQuickLinearLayoutPrivate() {}
};


/**********************************
 **
 ** QQuickRowLayout
 **
 **/
class QQuickRowLayout : public QQuickLinearLayout
{
    Q_OBJECT
    QML_NAMED_ELEMENT(RowLayout)

public:
    explicit QQuickRowLayout(QQuickItem *parent = 0)
        : QQuickLinearLayout(Qt::Horizontal, parent) {}
};


/**********************************
 **
 ** QQuickColumnLayout
 **
 **/
class QQuickColumnLayout : public QQuickLinearLayout
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ColumnLayout)

public:
    explicit QQuickColumnLayout(QQuickItem *parent = 0)
        : QQuickLinearLayout(Qt::Vertical, parent) {}
};

QT_END_NAMESPACE

#endif // QQUICKLINEARLAYOUT_P_H
