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

#ifndef QSGSOFTWAREPAINTERNODE_H
#define QSGSOFTWAREPAINTERNODE_H

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

#include <private/qsgadaptationlayer_p.h>
#include <QtQuick/qquickpainteditem.h>

#include <QtGui/QPixmap>

QT_BEGIN_NAMESPACE

class QSGSoftwarePainterNode : public QSGPainterNode
{
public:
    QSGSoftwarePainterNode(QQuickPaintedItem *item);
    ~QSGSoftwarePainterNode();

    void setPreferredRenderTarget(QQuickPaintedItem::RenderTarget target) override;

    void setSize(const QSize &size) override;
    QSize size() const { return m_size; }

    void setDirty(const QRect &dirtyRect = QRect()) override;

    void setOpaquePainting(bool opaque) override;
    bool opaquePainting() const { return m_opaquePainting; }

    void setLinearFiltering(bool linearFiltering) override;
    bool linearFiltering() const { return m_linear_filtering; }

    void setMipmapping(bool mipmapping) override;
    bool mipmapping() const { return m_mipmapping; }

    void setSmoothPainting(bool s) override;
    bool smoothPainting() const { return m_smoothPainting; }

    void setFillColor(const QColor &c) override;
    QColor fillColor() const { return m_fillColor; }

    void setContentsScale(qreal s) override;
    qreal contentsScale() const { return m_contentsScale; }

    void setFastFBOResizing(bool dynamic) override;
    bool fastFBOResizing() const { return m_fastFBOResizing; }

    QImage toImage() const override;
    void update() override;
    QSGTexture *texture() const override { return m_texture; }

    void paint(QPainter *painter);

    void paint();

    void setTextureSize(const QSize &size) override;
    QSize textureSize() const { return m_textureSize; }

private:

    QQuickPaintedItem::RenderTarget m_preferredRenderTarget;

    QQuickPaintedItem *m_item;

    QPixmap m_pixmap;
    QSGTexture *m_texture;

    QSize m_size;
    bool m_dirtyContents;
    QRect m_dirtyRect;
    bool m_opaquePainting;
    bool m_linear_filtering;
    bool m_mipmapping;
    bool m_smoothPainting;
    bool m_fastFBOResizing;
    QColor m_fillColor;
    qreal m_contentsScale;
    QSize m_textureSize;

    bool m_dirtyGeometry;
};

QT_END_NAMESPACE

#endif // QSGSOFTWAREPAINTERNODE_H
