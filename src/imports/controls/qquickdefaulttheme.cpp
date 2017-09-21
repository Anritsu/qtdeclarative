/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the Qt Quick Controls 2 module of the Qt Toolkit.
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

#include "qquickdefaulttheme_p.h"

QT_BEGIN_NAMESPACE

QQuickDefaultTheme::QQuickDefaultTheme()
    : QQuickTheme(QStringLiteral("Default"))
{
    systemPalette.setColor(QPalette::ButtonText, QColor::fromRgba(0xFF26282A));
    systemPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor::fromRgba(0x4D26282A));

    systemPalette.setColor(QPalette::BrightText, QColor::fromRgba(0xFFFFFFFF));
    systemPalette.setColor(QPalette::Disabled, QPalette::BrightText, QColor::fromRgba(0x4DFFFFFF));

    systemPalette.setColor(QPalette::Highlight, QColor::fromRgba(0xFF0066FF));
    systemPalette.setColor(QPalette::Disabled, QPalette::Highlight, QColor::fromRgba(0xFFF0F6FF));

    systemPalette.setColor(QPalette::Text, QColor::fromRgba(0xFF353637));
    systemPalette.setColor(QPalette::Disabled, QPalette::Text, QColor::fromRgba(0xFFC2C2C2));

    systemPalette.setColor(QPalette::WindowText, QColor::fromRgba(0xFF26282A));
    systemPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor::fromRgba(0xFFBDBEBF));

    systemPalette = resolvePalette(systemPalette);
}

const QPalette *QQuickDefaultTheme::palette(QPlatformTheme::Palette type) const
{
    Q_UNUSED(type);
    return &systemPalette;
}

QT_END_NAMESPACE
