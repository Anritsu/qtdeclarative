/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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
****************************************************************************/

#include "componentversion.h"
#include <QtCore/qstring.h>

ComponentVersion::ComponentVersion(const QString &versionString)
{
    const int dotIdx = versionString.indexOf(QLatin1Char('.'));
    if (dotIdx == -1)
        return;
    bool ok = false;
    const int maybeMajor = versionString.leftRef(dotIdx).toInt(&ok);
    if (!ok)
        return;
    const int maybeMinor = versionString.midRef(dotIdx + 1).toInt(&ok);
    if (!ok)
        return;
    m_major = maybeMajor;
    m_minor = maybeMinor;
}

bool operator<(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() < rhs.majorVersion()
            || (lhs.majorVersion() == rhs.majorVersion() && lhs.minorVersion() < rhs.minorVersion());
}

bool operator<=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() < rhs.majorVersion()
            || (lhs.majorVersion() == rhs.majorVersion()
                && lhs.minorVersion() <= rhs.minorVersion());
}

bool operator>(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return rhs < lhs;
}

bool operator>=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return rhs <= lhs;
}

bool operator==(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return lhs.majorVersion() == rhs.majorVersion()
            && lhs.minorVersion() == rhs.minorVersion();
}

bool operator!=(const ComponentVersion &lhs, const ComponentVersion &rhs)
{
    return !(lhs == rhs);
}
