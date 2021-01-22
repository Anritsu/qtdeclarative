/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
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

#ifndef QTQMLCOMPILERGLOBAL_H
#define QTQMLCOMPILERGLOBAL_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

#if defined(QT_BUILD_QMLDEVTOOLS_LIB) || defined(QT_QMLDEVTOOLS_LIB) || defined(QT_STATIC)
#  define Q_QMLCOMPILER_EXPORT
#else
#  if defined(QT_BUILD_QML_LIB)
#    define Q_QMLCOMPILER_EXPORT Q_DECL_EXPORT
#  else
#    define Q_QMLCOMPILER_EXPORT Q_DECL_IMPORT
#  endif
#endif

QT_END_NAMESPACE
#endif // QTQMLCOMPILERGLOBAL_H
