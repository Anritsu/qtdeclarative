/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "tst_qmltyperegistrar.h"
#include <QtTest/qtest.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qfile.h>
#include <QtQml/QQmlEngine>
#include <QtQml/QQmlComponent>

void tst_qmltyperegistrar::initTestCase()
{
    Q_ASSERT(QCoreApplication::instance());
    QFile file(QCoreApplication::applicationDirPath() + "/tst_qmltyperegistrar.qmltypes");
    QVERIFY(file.open(QIODevice::ReadOnly));
    qmltypesData = file.readAll();
    QVERIFY(file.atEnd());
    QCOMPARE(file.error(), QFile::NoError);
}

void tst_qmltyperegistrar::qmltypesHasForeign()
{
    QVERIFY(qmltypesData.contains("things"));
}

void tst_qmltyperegistrar::qmltypesHasHppClassAndNoext()
{
    QVERIFY(qmltypesData.contains("HppClass"));
#ifdef BUILD_WITH_CMAKE
    QEXPECT_FAIL("", "Type registration does not work manually moced files", Continue);
#endif
    QVERIFY(qmltypesData.contains("Noext"));
}

void tst_qmltyperegistrar::qmltypesHasReadAndWrite()
{
    QVERIFY(qmltypesData.contains(R"(read: "eieiei")"));
    QVERIFY(qmltypesData.contains(R"(write: "setEieiei")"));
}

void tst_qmltyperegistrar::qmltypesHasFileNames()
{
    QVERIFY(qmltypesData.contains("file: \"hppheader.hpp\""));
#ifdef BUILD_WITH_CMAKE
    QEXPECT_FAIL("", "Type registration does not work manually moced files", Continue);
#endif
    QVERIFY(qmltypesData.contains("file: \"noextheader\""));
    QVERIFY(qmltypesData.contains("file: \"tst_qmltyperegistrar.h\""));
}

void tst_qmltyperegistrar::qmltypesHasFlags()
{
    QVERIFY(qmltypesData.contains("name: \"Flags\""));
    QVERIFY(qmltypesData.contains("alias: \"Flag\""));
    QVERIFY(qmltypesData.contains("isFlag: true"));
}

void tst_qmltyperegistrar::superAndForeignTypes()
{
    QVERIFY(qmltypesData.contains("values: [\"Pixel\", \"Centimeter\", \"Inch\", \"Point\"]"));
    QVERIFY(qmltypesData.contains("name: \"SizeGadget\""));
    QVERIFY(qmltypesData.contains("prototype: \"SizeEnums\""));
    QVERIFY(qmltypesData.contains("Property { name: \"height\"; type: \"int\"; read: \"height\"; write: \"setHeight\" }"));
    QVERIFY(qmltypesData.contains("Property { name: \"width\"; type: \"int\"; read: \"width\"; write: \"setWidth\" }"));
    QVERIFY(qmltypesData.contains("Method { name: \"sizeToString\"; type: \"QString\" }"));
    QVERIFY(qmltypesData.contains("extension: \"SizeValueType\""));
}

void tst_qmltyperegistrar::accessSemantics()
{
    QVERIFY(qmltypesData.contains("accessSemantics: \"reference\""));
    QVERIFY(qmltypesData.contains("accessSemantics: \"value\""));
}

void tst_qmltyperegistrar::isBindable()
{
    // TODO: readonly?
    QVERIFY(qmltypesData.contains(R"(Property { name: "someProperty"; bindable: "bindableSomeProperty"; type: "int"; isReadonly: true)"));
}

void tst_qmltyperegistrar::restrictToImportVersion()
{
    QVERIFY(qmltypesData.contains("ExcessiveVersion"));
    QVERIFY(!qmltypesData.contains("1536"));           // Q_REVISION(6, 0)
    QVERIFY(!qmltypesData.contains("paletteChanged")); // Added in version 6.0
}

void tst_qmltyperegistrar::pastMajorVersions()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QML\nimport QmlTypeRegistrarTest 0.254\nQtObject {}", QUrl());
    QVERIFY2(!c.isError(), qPrintable(c.errorString()));
}

void tst_qmltyperegistrar::implementsInterfaces()
{
    QVERIFY(qmltypesData.contains("interfaces: [\"Interface\"]"));
    QVERIFY(qmltypesData.contains("interfaces: [\"Interface\", \"Interface2\"]"));
}

void tst_qmltyperegistrar::namespacedElement()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QML\nimport QmlTypeRegistrarTest 1.0\nElement {}", QUrl());
    QVERIFY2(!c.isError(), qPrintable(c.errorString()));
}

void tst_qmltyperegistrar::derivedFromForeign()
{
    QVERIFY(qmltypesData.contains("name: \"DerivedFromForeign\""));
    QVERIFY(qmltypesData.contains("prototype: \"QTimeLine\""));
    QVERIFY(qmltypesData.contains("name: \"QTimeLine\""));
}

void tst_qmltyperegistrar::metaTypesRegistered()
{
    QQmlEngine engine;
    QQmlComponent c(&engine);
    c.setData("import QmlTypeRegistrarTest\nOoo {}", QUrl());
    QVERIFY(c.isReady());
    QScopedPointer<QObject> obj(c.create());

    auto verifyMetaType = [](const char *name, const char *className) {
        const auto foundMetaType = QMetaType::fromName(name);
        QVERIFY(foundMetaType.isValid());
        QCOMPARE(foundMetaType.name(), name);
        QVERIFY(foundMetaType.metaObject());
        QCOMPARE(foundMetaType.metaObject()->className(), className);
    };

    verifyMetaType("Foo", "Foo");
    verifyMetaType("Ooo*", "Ooo");
    verifyMetaType("Bbb*", "Bbb");
    verifyMetaType("Ccc*", "Ccc");
}

QTEST_MAIN(tst_qmltyperegistrar)
