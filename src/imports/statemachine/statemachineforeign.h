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

#ifndef STATEMACHINEFOREIGN_H
#define STATEMACHINEFOREIGN_H

#include <QtQml/qqml.h>
#include <QtCore/qhistorystate.h>
#include <QtCore/qstate.h>
#include <QtCore/qabstractstate.h>
#include <QtCore/qsignaltransition.h>

struct QHistoryStateForeign
{
    Q_GADGET
    QML_FOREIGN(QHistoryState)
    QML_NAMED_ELEMENT(HistoryState)
};

struct QStateForeign
{
    Q_GADGET
    QML_FOREIGN(QState)
    QML_NAMED_ELEMENT(QState)
    QML_UNCREATABLE("Don't use this, use State instead.")
};

struct QAbstractStateForeign
{
    Q_GADGET
    QML_FOREIGN(QAbstractState)
    QML_NAMED_ELEMENT(QAbstractState)
    QML_UNCREATABLE("Don't use this, use State instead.")
};

struct QSignalTransitionForeign
{
    Q_GADGET
    QML_FOREIGN(QSignalTransition)
    QML_NAMED_ELEMENT(QSignalTransition)
    QML_UNCREATABLE("Don't use this, use SignalTransition instead.")
};

#endif // STATEMACHINEFOREIGN_H
