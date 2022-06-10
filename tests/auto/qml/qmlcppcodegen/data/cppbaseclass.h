/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CPPBASECLASS_H
#define CPPBASECLASS_H

#include <QtCore/qobject.h>
#include <QtCore/qproperty.h>
#include <QtQml/qqml.h>

class CppBaseClass : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int cppProp MEMBER cppProp BINDABLE cppPropBindable FINAL)
    Q_PROPERTY(int cppProp2 MEMBER cppProp2 BINDABLE cppProp2Bindable FINAL)
    Q_PROPERTY(QList<int> boo MEMBER boo FINAL CONSTANT)
    QML_ELEMENT
public:
    CppBaseClass(QObject *parent = nullptr)
        : QObject(parent)
    {
        boo.append(16);
        boo.append(17);
    }

    QProperty<int> cppProp;
    QBindable<int> cppPropBindable() { return QBindable<int>(&cppProp); }

    QProperty<int> cppProp2;
    QBindable<int> cppProp2Bindable() { return QBindable<int>(&cppProp2); }

    Q_INVOKABLE void doCall(QObject *foo);

private:
    QList<int> boo;
};

inline void CppBaseClass::doCall(QObject *foo)
{
    cppProp = foo ? 17 : 18;
}

class CppSingleton : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    explicit CppSingleton(QObject *parent = nullptr) : QObject(parent)
    {
        setObjectName(QStringLiteral("ItIsTheSingleton"));
    }
};

#endif // CPPBASECLASS_H
