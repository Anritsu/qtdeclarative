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

#ifndef QSGCOMPRESSEDTEXTURE_P_H
#define QSGCOMPRESSEDTEXTURE_P_H

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

#include <private/qtexturefiledata_p.h>
#include <private/qsgcontext_p.h>
#include <private/qsgtexture_p.h>
#include <QQuickTextureFactory>
#include <QOpenGLFunctions>

QT_BEGIN_NAMESPACE

class QSGCompressedTexturePrivate;

class Q_QUICK_PRIVATE_EXPORT QSGCompressedTexture : public QSGTexture
{
    Q_DECLARE_PRIVATE(QSGCompressedTexture)
    Q_OBJECT
public:
    QSGCompressedTexture(const QTextureFileData& texData);
    virtual ~QSGCompressedTexture();

    QSize textureSize() const override;
    bool hasAlphaChannel() const override;
    bool hasMipmaps() const override;

    int textureId() const override;
    void bind() override;

    QTextureFileData textureData() const;

    static bool formatIsOpaque(quint32 glTextureFormat);

protected:
    QTextureFileData m_textureData;
    QSize m_size;
    mutable uint m_textureId = 0;
    QRhiTexture *m_texture = nullptr;
    bool m_hasAlpha = false;
    bool m_uploaded = false;
};

namespace QSGOpenGLAtlasTexture {
    class Manager;
}

class QSGCompressedTexturePrivate : public QSGTexturePrivate
{
    Q_DECLARE_PUBLIC(QSGCompressedTexture)
public:
    int comparisonKey() const override;
    QRhiTexture *rhiTexture() const override;
    void updateRhiTexture(QRhi *rhi, QRhiResourceUpdateBatch *resourceUpdates) override;
};

class Q_QUICK_PRIVATE_EXPORT QSGCompressedTextureFactory : public QQuickTextureFactory
{
public:
    QSGCompressedTextureFactory(const QTextureFileData& texData);
    QSGTexture *createTexture(QQuickWindow *) const override;
    int textureByteCount() const override;
    QSize textureSize() const override;

protected:
    QTextureFileData m_textureData;

private:
    friend class QSGOpenGLAtlasTexture::Manager;
};

QT_END_NAMESPACE

#endif // QSGCOMPRESSEDTEXTURE_P_H
