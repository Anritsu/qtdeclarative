/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef QMLTCCOMPILER_H
#define QMLTCCOMPILER_H

#include "qmltctyperesolver.h"
#include "qmltcvisitor.h"
#include "qmltcoutputir.h"

#include <QtCore/qcommandlineparser.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qstring.h>
#include <QtCore/qhash.h>

#include <private/qqmljslogger_p.h>

#include <memory>

QT_BEGIN_NAMESPACE

struct QmltcCompilerInfo
{
    QString outputCppFile;
    QString outputHFile;
    QString outputNamespace;
    QString resourcePath;
};

class CodeGenerator;
class QmltcCompiler
{
public:
    QmltcCompiler(const QString &url, QmltcTypeResolver *resolver, QmltcVisitor *visitor,
                  QQmlJSLogger *logger);
    void compile(const QmltcCompilerInfo &info);

    ~QmltcCompiler();

private:
    QString m_url; // QML input file url
    QmltcTypeResolver *m_typeResolver = nullptr;
    QmltcVisitor *m_visitor = nullptr;
    QQmlJSLogger *m_logger = nullptr;
    std::unique_ptr<CodeGenerator> m_prototypeCodegen;
    QmltcCompilerInfo m_info {}; // miscellaneous input/output information
    QString m_urlMethodName;

    struct UniqueStringId;
    struct QmltcTypeLocalData;
    // per-type, per-property code generation cache of created symbols
    QHash<UniqueStringId, QmltcTypeLocalData> m_uniques;

    void compileUrlMethod(QmltcMethod &urlMethod, const QString &urlMethodName);
    void
    compileType(QmltcType &current, const QQmlJSScope::ConstPtr &type,
                std::function<void(QmltcType &, const QQmlJSScope::ConstPtr &)> compileElements);
    void compileTypeElements(QmltcType &current, const QQmlJSScope::ConstPtr &type);
    void compileEnum(QmltcType &current, const QQmlJSMetaEnum &e);
    void compileMethod(QmltcType &current, const QQmlJSMetaMethod &m,
                       const QQmlJSScope::ConstPtr &owner);
    void compileProperty(QmltcType &current, const QQmlJSMetaProperty &p,
                         const QQmlJSScope::ConstPtr &owner);
    void compileAlias(QmltcType &current, const QQmlJSMetaProperty &alias,
                      const QQmlJSScope::ConstPtr &owner);

    /*!
        \internal

        Helper structure that holds the information necessary for most bindings,
        such as accessor name, which is used to reference the properties. For
        example:
        > (accessor.name)->(propertyName) results in "this->myProperty"

        This data is also used in more advanced scenarios by attached and
        grouped properties
    */
    struct BindingAccessorData
    {
        QQmlJSScope::ConstPtr scope; // usually the current type
        QString name = QStringLiteral("this");
        QString propertyName = QString();
        bool isValueType = false;
    };
    void compileBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                        const QQmlJSScope::ConstPtr &type, const BindingAccessorData &accessor);

    // special case (for simplicity)
    void compileScriptBinding(QmltcType &current, const QQmlJSMetaPropertyBinding &binding,
                              const QString &bindingSymbolName, const QQmlJSScope::ConstPtr &type,
                              const QString &propertyName,
                              const QQmlJSScope::ConstPtr &propertyType,
                              const BindingAccessorData &accessor);

    // TODO: remove this special case
    void compileScriptBindingOfComponent(QmltcType &current, const QQmlJSScope::ConstPtr &type,
                                         const QQmlJSMetaPropertyBinding &binding,
                                         const QString &propertyName);

    /*!
        \internal
        Helper structure that acts as a key in a hash-table of
        QmltcType-specific data (such as local variable names). Using a
        hash-table allows to avoid creating the same variables multiple times
        during binding compilation, which leads to better code generation and
        faster object creation. This is really something that the QML optimizer
        should do, but we have only this home-grown alternative at the moment
    */
    struct UniqueStringId
    {
        QString unique;
        UniqueStringId(const QmltcType &context, const QString &property)
            : unique(context.cppType + u"_" + property) // this is unique enough
        {
            Q_ASSERT(!context.cppType.isEmpty());
            Q_ASSERT(!property.isEmpty());
        }
        friend bool operator==(const UniqueStringId &x, const UniqueStringId &y)
        {
            return x.unique == y.unique;
        }
        friend bool operator!=(const UniqueStringId &x, const UniqueStringId &y)
        {
            return !(x == y);
        }
        friend size_t qHash(const UniqueStringId &x, size_t seed = 0)
        {
            return qHash(x.unique, seed);
        }
    };

    struct QmltcTypeLocalData
    {
        // empty QString() means that the local data is not present (yet)
        QString qmlListVariableName;
        QString onAssignmentObjectName;
        QString attachedVariableName;
    };

    QHash<QString, qsizetype> m_symbols;
    QString newSymbol(const QString &base);

    bool hasErrors() const { return m_logger->hasErrors(); }
    void recordError(const QQmlJS::SourceLocation &location, const QString &message,
                     QQmlJSLoggerCategory category = Log_Compiler)
    {
        // pretty much any compiler error is a critical error (we cannot
        // generate code - compilation fails)
        m_logger->log(message, category, location);
    }
    void recordError(const QV4::CompiledData::Location &location, const QString &message,
                     QQmlJSLoggerCategory category = Log_Compiler)
    {
        recordError(QQmlJS::SourceLocation { 0, 0, location.line(), location.column() }, message,
                    category);
    }
};

QT_END_NAMESPACE

#endif // QMLTCCOMPILER_H
