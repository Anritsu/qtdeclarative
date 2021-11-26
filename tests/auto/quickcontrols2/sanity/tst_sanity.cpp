/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtTest>
#include <QtQml>
#include <QtCore/private/qhooks_p.h>
#include <QtCore/qpair.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qset.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmlmetatype_p.h>
#include <QtQuickTestUtils/private/visualtestutils_p.h>
#include <QtQuickControlsTestUtils/private/controlstestutils_p.h>
#include <QtQmlLint/private/qqmllinter_p.h>

using namespace QQuickVisualTestUtils;
using namespace QQuickControlsTestUtils;

Q_GLOBAL_STATIC(QObjectList, qt_qobjects)

extern "C" Q_DECL_EXPORT void qt_addQObject(QObject *object)
{
    qt_qobjects->append(object);
}

extern "C" Q_DECL_EXPORT void qt_removeQObject(QObject *object)
{
    qt_qobjects->removeAll(object);
}

class tst_Sanity : public QObject
{
    Q_OBJECT
public:
    tst_Sanity();

private slots:
    void init();
    void cleanup();
    void initTestCase();

    void jsFiles();
    void qmllint();
    void qmllint_data();
    void ids();
    void ids_data();

private:
    QMap<QString, QString> sourceQmlFiles;
    QMap<QString, QString> installedQmlFiles;
    QQuickStyleHelper styleHelper;
    QStringList m_importPaths;

    QQmlLinter m_linter;
    QMap<QString, QQmlJSLogger::Option> m_options;
};

tst_Sanity::tst_Sanity()
    : m_importPaths({ QLibraryInfo::path(QLibraryInfo::QmlImportsPath) }),
      m_linter(m_importPaths),
      m_options(QQmlJSLogger::options())
{
    // We do not care about any warnings that aren't explicitly created by controls-sanity.
    // Mainly because a lot of false positives are generated because we are linting files from
    // different modules directly without their generated qmldirs.
    for (const QString &key : m_options.keys())
        m_options[key].setLevel(u"disable"_qs);

    m_options[u"controls-sanity"_qs].setLevel(u"warning"_qs);
    m_options[u"multiple-attached-objects"_qs].setLevel(u"warning"_qs);
}

void tst_Sanity::init()
{
    qtHookData[QHooks::AddQObject] = reinterpret_cast<quintptr>(&qt_addQObject);
    qtHookData[QHooks::RemoveQObject] = reinterpret_cast<quintptr>(&qt_removeQObject);
}

void tst_Sanity::cleanup()
{
    qt_qobjects->clear();
    qtHookData[QHooks::AddQObject] = 0;
    qtHookData[QHooks::RemoveQObject] = 0;
}

class BaseValidator : public QQmlJS::AST::Visitor
{
public:
    QString errors() const { return m_errors.join(", "); }

    bool validate(const QString& filePath)
    {
        m_errors.clear();
        m_fileName = QFileInfo(filePath).fileName();

        QFile file(filePath);
        if (!file.open(QFile::ReadOnly)) {
            m_errors += QString("%1: failed to open (%2)").arg(m_fileName, file.errorString());
            return false;
        }

        QQmlJS::Engine engine;
        QQmlJS::Lexer lexer(&engine);
        lexer.setCode(QString::fromUtf8(file.readAll()), /*line = */ 1);

        QQmlJS::Parser parser(&engine);
        if (!parser.parse()) {
            const auto diagnosticMessages = parser.diagnosticMessages();
            for (const QQmlJS::DiagnosticMessage &msg : diagnosticMessages)
                m_errors += QString("%s:%d : %s").arg(m_fileName).arg(msg.loc.startLine).arg(msg.message);
            return false;
        }

        QQmlJS::AST::UiProgram* ast = parser.ast();
        ast->accept(this);
        return m_errors.isEmpty();
    }

protected:
    void addError(const QString& error, QQmlJS::AST::Node *node)
    {
        m_errors += QString("%1:%2 : %3").arg(m_fileName).arg(node->firstSourceLocation().startLine).arg(error);
    }

    void throwRecursionDepthError() final
    {
        m_errors += QString::fromLatin1("%1: Maximum statement or expression depth exceeded")
                            .arg(m_fileName);
    }

private:
    QString m_fileName;
    QStringList m_errors;
};

void tst_Sanity::initTestCase()
{
    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(QString("import QtQuick.Templates 2.%1; Control { }").arg(15).toUtf8(), QUrl());

    const QStringList qmlTypeNames = QQmlMetaType::qmlTypeNames();

    // Collect the files from each style in the source tree.
    QDirIterator it(QQC2_IMPORT_PATH, QStringList() << "*.qml" << "*.js", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QFileInfo info = it.fileInfo();
        if (qmlTypeNames.contains(QStringLiteral("QtQuick.Templates/") + info.baseName()))
            sourceQmlFiles.insert(info.dir().dirName() + "/" + info.fileName(), info.filePath());
    }

    // Then, collect the files from each installed style directory.
    const QVector<QPair<QString, QString>> styleRelativePaths = {
        { "basic", "QtQuick/Controls/Basic" },
        { "fusion", "QtQuick/Controls/Fusion" },
        { "material", "QtQuick/Controls/Material" },
        { "universal", "QtQuick/Controls/Universal" },
        // TODO: add native styles: QTBUG-87108
    };
    for (const auto &stylePathPair : styleRelativePaths) {
        forEachControl(&engine, QQC2_IMPORT_PATH, stylePathPair.first, stylePathPair.second, QStringList(),
                [&](const QString &relativePath, const QUrl &absoluteUrl) {
             installedQmlFiles.insert(relativePath, absoluteUrl.toLocalFile());
        });
    }
}

void tst_Sanity::jsFiles()
{
    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it) {
        if (QFileInfo(it.value()).suffix() == QStringLiteral("js"))
            QFAIL(qPrintable(it.value() +  ": JS files are not allowed"));
    }
}

void tst_Sanity::qmllint()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    QJsonArray output;
    bool success =
            m_linter.lintFile(filePath, nullptr, true, &output, m_importPaths, {}, {}, m_options);

    QVERIFY2(success, qPrintable(QJsonDocument(output).toJson(QJsonDocument::Compact)));
}

void tst_Sanity::qmllint_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

class IdValidator : public BaseValidator
{
public:
    IdValidator() : m_depth(0) { }

protected:
    bool visit(QQmlJS::AST::UiObjectBinding *) override
    {
        ++m_depth;
        return true;
    }

    void endVisit(QQmlJS::AST::UiObjectBinding *) override
    {
        --m_depth;
    }

    bool visit(QQmlJS::AST::UiScriptBinding *node) override
    {
        if (m_depth == 0)
            return true;

        QQmlJS::AST::UiQualifiedId *id = node->qualifiedId;
        if (id && id->name ==  QStringLiteral("id"))
            addError(QString("Internal IDs are not allowed (%1)").arg(extractName(node->statement)), node);
        return true;
    }

private:
    QString extractName(QQmlJS::AST::Statement *statement)
    {
        QQmlJS::AST::ExpressionStatement *expressionStatement = static_cast<QQmlJS::AST::ExpressionStatement *>(statement);
        if (!expressionStatement)
            return QString();

        QQmlJS::AST::IdentifierExpression *expression = static_cast<QQmlJS::AST::IdentifierExpression *>(expressionStatement->expression);
        if (!expression)
            return QString();

        return expression->name.toString();
    }

    int m_depth;
};

void tst_Sanity::ids()
{
    QFETCH(QString, control);
    QFETCH(QString, filePath);

    IdValidator validator;
    if (!validator.validate(filePath))
        QFAIL(qPrintable(validator.errors()));
}

void tst_Sanity::ids_data()
{
    QTest::addColumn<QString>("control");
    QTest::addColumn<QString>("filePath");

    QMap<QString, QString>::const_iterator it;
    for (it = sourceQmlFiles.constBegin(); it != sourceQmlFiles.constEnd(); ++it)
        QTest::newRow(qPrintable(it.key())) << it.key() << it.value();
}

QTEST_MAIN(tst_Sanity)

#include "tst_sanity.moc"
