/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qqmljsimportvisitor_p.h"

#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>
#include <QtCore/qqueue.h>

QT_BEGIN_NAMESPACE

using namespace QQmlJS::AST;

QQmlJSImportVisitor::QQmlJSImportVisitor(
        QQmlJSImporter *importer, const QString &implicitImportDirectory,
        const QStringList &qmltypesFiles)
    : m_implicitImportDirectory(implicitImportDirectory)
    , m_qmltypesFiles(qmltypesFiles)
    , m_currentScope(QQmlJSScope::create(QQmlJSScope::JSFunctionScope))
    , m_importer(importer)
{
    m_globalScope = m_currentScope;
    m_currentScope->setIsComposite(true);
}

void QQmlJSImportVisitor::enterEnvironment(QQmlJSScope::ScopeType type, const QString &name,
                                           const QQmlJS::SourceLocation &location)
{
    m_currentScope = QQmlJSScope::create(type, m_currentScope);
    if (type == QQmlJSScope::GroupedPropertyScope || type == QQmlJSScope::AttachedPropertyScope)
        m_currentScope->setInternalName(name);
    else
        m_currentScope->setBaseTypeName(name);
    m_currentScope->setIsComposite(true);
    m_currentScope->setSourceLocation(location);
}

void QQmlJSImportVisitor::leaveEnvironment()
{
    m_currentScope = m_currentScope->parentScope();
}

void QQmlJSImportVisitor::resolveAliases()
{
    QQueue<QQmlJSScope::Ptr> objects;
    objects.enqueue(m_exportedRootScope);

    while (!objects.isEmpty()) {
        const QQmlJSScope::Ptr object = objects.dequeue();
        const auto properties = object->ownProperties();
        for (auto property : properties) {
            if (!property.isAlias())
                continue;
            const auto it = m_scopesById.find(property.typeName());
            if (it != m_scopesById.end()) {
                property.setType(QQmlJSScope::ConstPtr(*it));
                if (!it->isNull()) {
                    if (const QString internalName = (*it)->internalName(); !internalName.isEmpty())
                        property.setTypeName(internalName);
                }
                object->addOwnProperty(property);
            }
        }

        const auto childScopes = object->childScopes();
        for (const auto &childScope : childScopes)
            objects.enqueue(childScope);
    }
}

QQmlJSScope::Ptr QQmlJSImportVisitor::result() const
{
    return m_exportedRootScope;
}

void QQmlJSImportVisitor::importBaseModules()
{
    Q_ASSERT(m_rootScopeImports.isEmpty());
    m_rootScopeImports = m_importer->importBuiltins();

    if (!m_qmltypesFiles.isEmpty())
        m_rootScopeImports.insert(m_importer->importQmltypes(m_qmltypesFiles));

    m_rootScopeImports.insert(m_importer->importDirectory(m_implicitImportDirectory));
    m_errors.append(m_importer->takeWarnings());
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiProgram *)
{
    importBaseModules();
    return true;
}

void QQmlJSImportVisitor::endVisit(UiProgram *)
{
    resolveAliases();
}

bool QQmlJSImportVisitor::visit(UiObjectDefinition *definition)
{
    QString superType;
    for (auto segment = definition->qualifiedTypeNameId; segment; segment = segment->next) {
        if (!superType.isEmpty())
            superType.append(u'.');
        superType.append(segment->name.toString());
    }
    enterEnvironment(QQmlJSScope::QMLScope, superType, definition->firstSourceLocation());
    if (!m_exportedRootScope)
        m_exportedRootScope = m_currentScope;

    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports);
    return true;
}

void QQmlJSImportVisitor::endVisit(UiObjectDefinition *)
{
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports);
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(UiPublicMember *publicMember)
{
    switch (publicMember->type) {
    case UiPublicMember::Signal: {
        UiParameterList *param = publicMember->parameters;
        QQmlJSMetaMethod method;
        method.setMethodType(QQmlJSMetaMethod::Signal);
        method.setMethodName(publicMember->name.toString());
        while (param) {
            method.addParameter(param->name.toString(), param->type->name.toString());
            param = param->next;
        }
        m_currentScope->addOwnMethod(method);
        break;
    }
    case UiPublicMember::Property: {
        auto typeName = publicMember->memberType
                ? publicMember->memberType->name
                : QStringView();
        const bool isAlias = (typeName == QLatin1String("alias"));
        if (isAlias) {
            const auto expression = cast<ExpressionStatement *>(publicMember->statement);
            if (const auto idExpression = cast<IdentifierExpression *>(expression->expression))
                typeName = idExpression->name;
        }
        QQmlJSMetaProperty prop;
        prop.setPropertyName(publicMember->name.toString());
        prop.setIsList(publicMember->typeModifier == QLatin1String("list"));
        prop.setIsWritable(!publicMember->isReadonlyMember);
        prop.setIsAlias(isAlias);
        const QString typeNameString = typeName.toString();
        if (const auto type = m_rootScopeImports.value(typeNameString)) {
            prop.setType(type);
            const QString internalName = type->internalName();
            prop.setTypeName(internalName.isEmpty() ? typeNameString : internalName);
        } else {
            prop.setTypeName(typeNameString);
        }
        m_currentScope->insertPropertyIdentifier(prop);
        break;
    }
    }
    return true;
}

void QQmlJSImportVisitor::visitFunctionExpressionHelper(QQmlJS::AST::FunctionExpression *fexpr)
{
    using namespace QQmlJS::AST;
    auto name = fexpr->name.toString();
    if (!name.isEmpty()) {
        QQmlJSMetaMethod method(name);
        method.setMethodType(QQmlJSMetaMethod::Method);
        if (const auto *formals = fexpr->formals) {
            const auto parameters = formals->formals();
            for (const auto &parameter : parameters) {
                const QString type = parameter.typeName();
                method.addParameter(parameter.id,
                                    type.isEmpty() ? QStringLiteral("var") : type);
            }
        }
        method.setReturnTypeName(fexpr->typeAnnotation
                                 ? fexpr->typeAnnotation->type->toString()
                                 : QStringLiteral("var"));
        m_currentScope->addOwnMethod(method);

        if (m_currentScope->scopeType() != QQmlJSScope::QMLScope) {
            m_currentScope->insertJSIdentifier(
                        name, {
                            QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                            fexpr->firstSourceLocation()
                        });
        }
        enterEnvironment(QQmlJSScope::JSFunctionScope, name, fexpr->firstSourceLocation());
    } else {
        enterEnvironment(QQmlJSScope::JSFunctionScope, QStringLiteral("<anon>"),
                         fexpr->firstSourceLocation());
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FunctionExpression *fexpr)
{
    visitFunctionExpressionHelper(fexpr);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FunctionExpression *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FunctionDeclaration *fdecl)
{
    visitFunctionExpressionHelper(fdecl);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::FunctionDeclaration *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ClassExpression *ast)
{
    QQmlJSMetaProperty prop;
    prop.setPropertyName(ast->name.toString());
    m_currentScope->addOwnProperty(prop);
    enterEnvironment(QQmlJSScope::JSFunctionScope, ast->name.toString(),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ClassExpression *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(UiScriptBinding *scriptBinding)
{
    const auto id = scriptBinding->qualifiedId;
    const auto *statement = cast<ExpressionStatement *>(scriptBinding->statement);
    if (!id->next && id->name == QLatin1String("id")) {
        const auto *idExprension = cast<IdentifierExpression *>(statement->expression);
        m_scopesById.insert(idExprension->name.toString(), m_currentScope);
    } else {
        for (auto group = id; group->next; group = group->next) {
            const QString name = group->name.toString();

            if (name.isEmpty())
                break;

            enterEnvironment(name.front().isUpper() ? QQmlJSScope::AttachedPropertyScope
                                                    : QQmlJSScope::GroupedPropertyScope,
                             name, group->firstSourceLocation());
        }

        // TODO: remember the actual binding, once we can process it.

        while (m_currentScope->scopeType() == QQmlJSScope::GroupedPropertyScope
               || m_currentScope->scopeType() == QQmlJSScope::AttachedPropertyScope) {
            leaveEnvironment();
        }

        if (!statement || !statement->expression->asFunctionDefinition()) {
            enterEnvironment(QQmlJSScope::JSFunctionScope, QStringLiteral("binding"),
                             scriptBinding->statement->firstSourceLocation());
        }
    }

    return true;
}

void QQmlJSImportVisitor::endVisit(UiScriptBinding *scriptBinding)
{
    const auto id = scriptBinding->qualifiedId;
    if (id->next || id->name != QLatin1String("id")) {
        const auto *statement = cast<ExpressionStatement *>(scriptBinding->statement);
        if (!statement || !statement->expression->asFunctionDefinition())
            leaveEnvironment();
    }
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiEnumDeclaration *uied)
{
    QQmlJSMetaEnum qmlEnum(uied->name.toString());
    for (const auto *member = uied->members; member; member = member->next) {
        qmlEnum.addKey(member->member.toString());
        qmlEnum.addValue(int(member->value));
    }
    m_currentScope->addOwnEnumeration(qmlEnum);
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiImport *import)
{
    // construct path
    QString prefix = QLatin1String("");
    if (import->asToken.isValid()) {
        prefix += import->importId;
    }
    auto filename = import->fileName.toString();
    if (!filename.isEmpty()) {
        const QFileInfo file(filename);
        const QFileInfo path(file.isRelative() ? QDir(m_implicitImportDirectory).filePath(filename)
                                               : filename);
        if (path.isDir()) {
            m_rootScopeImports.insert(m_importer->importDirectory(path.canonicalFilePath(), prefix));
        } else if (path.isFile()) {
            const auto scope = m_importer->importFile(path.canonicalFilePath());
            m_rootScopeImports.insert(prefix.isEmpty() ? scope->internalName() : prefix, scope);
        }
        return true;
    }

    QString path {};
    auto uri = import->importUri;
    while (uri) {
        path.append(uri->name);
        path.append(u'/');
        uri = uri->next;
    }
    path.chop(1);

    const auto imported = m_importer->importModule(
                path, prefix, import->version ? import->version->version : QTypeRevision());

    m_rootScopeImports.insert(imported);

    m_errors.append(m_importer->takeWarnings());
    return true;
}

void QQmlJSImportVisitor::throwRecursionDepthError()
{
    m_errors.append({
                        QStringLiteral("Maximum statement or expression depth exceeded"),
                        QtCriticalMsg,
                        QQmlJS::SourceLocation()
                    });
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ClassDeclaration *ast)
{
    enterEnvironment(QQmlJSScope::JSFunctionScope, ast->name.toString(),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ClassDeclaration *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ForStatement *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("forloop"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ForStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::ForEachStatement *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("foreachloop"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::ForEachStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::Block *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("block"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::Block *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::CaseBlock *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("case"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::CaseBlock *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::Catch *catchStatement)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("catch"),
                     catchStatement->firstSourceLocation());
    m_currentScope->insertJSIdentifier(
                catchStatement->patternElement->bindingIdentifier.toString(), {
                    QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                    catchStatement->patternElement->firstSourceLocation()
                });
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::Catch *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::WithStatement *ast)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("with"),
                     ast->firstSourceLocation());
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::WithStatement *)
{
    leaveEnvironment();
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::VariableDeclarationList *vdl)
{
    while (vdl) {
        m_currentScope->insertJSIdentifier(
                    vdl->declaration->bindingIdentifier.toString(),
                    {
                        (vdl->declaration->scope == QQmlJS::AST::VariableScope::Var)
                            ? QQmlJSScope::JavaScriptIdentifier::FunctionScoped
                            : QQmlJSScope::JavaScriptIdentifier::LexicalScoped,
                        vdl->declaration->firstSourceLocation()
                    });
        vdl = vdl->next;
    }
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::FormalParameterList *fpl)
{
    for (auto const &boundName : fpl->boundNames()) {
        m_currentScope->insertJSIdentifier(
                    boundName.id, {
                        QQmlJSScope::JavaScriptIdentifier::Parameter,
                        fpl->firstSourceLocation()
                    });
    }
    return true;
}

bool QQmlJSImportVisitor::visit(QQmlJS::AST::UiObjectBinding *uiob)
{
    // property QtObject __styleData: QtObject {...}

    Q_ASSERT(uiob->qualifiedTypeNameId);
    QString name;
    for (auto id = uiob->qualifiedTypeNameId; id; id = id->next)
        name += id->name.toString() + QLatin1Char('.');

    name.chop(1);

    if (!uiob->hasOnToken) {
        QQmlJSMetaProperty prop;
        prop.setPropertyName(uiob->qualifiedId->name.toString());
        prop.setTypeName(name);
        prop.setIsWritable(true);
        prop.setIsPointer(true);
        prop.setIsAlias(name == QLatin1String("alias"));
        prop.setType(m_rootScopeImports.value(uiob->qualifiedTypeNameId->name.toString()));
        m_currentScope->addOwnProperty(prop);
    }

    enterEnvironment(QQmlJSScope::QMLScope, name,
                     uiob->qualifiedTypeNameId->identifierToken);
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports);
    return true;
}

void QQmlJSImportVisitor::endVisit(QQmlJS::AST::UiObjectBinding *uiob)
{
    QQmlJSScope::resolveTypes(m_currentScope, m_rootScopeImports);
    const QQmlJSScope::ConstPtr childScope = m_currentScope;
    leaveEnvironment();

    if (!uiob->hasOnToken) {
        QQmlJSMetaProperty property = m_currentScope->property(uiob->qualifiedId->name.toString());
        property.setType(childScope);
        m_currentScope->addOwnProperty(property);
    }
}

bool QQmlJSImportVisitor::visit(ExportDeclaration *)
{
    Q_ASSERT(!m_exportedRootScope.isNull());
    Q_ASSERT(m_exportedRootScope != m_globalScope);
    Q_ASSERT(m_currentScope == m_globalScope);
    m_currentScope = m_exportedRootScope;
    return true;
}

void QQmlJSImportVisitor::endVisit(ExportDeclaration *)
{
    Q_ASSERT(!m_exportedRootScope.isNull());
    m_currentScope = m_exportedRootScope->parentScope();
    Q_ASSERT(m_currentScope == m_globalScope);
}

bool QQmlJSImportVisitor::visit(ESModule *module)
{
    enterEnvironment(QQmlJSScope::JSLexicalScope, QStringLiteral("module"),
                     module->firstSourceLocation());
    Q_ASSERT(m_exportedRootScope.isNull());
    m_exportedRootScope = m_currentScope;
    m_exportedRootScope->setIsScript(true);
    importBaseModules();
    leaveEnvironment();
    return true;
}

void QQmlJSImportVisitor::endVisit(ESModule *)
{
    QQmlJSScope::resolveTypes(m_exportedRootScope, m_rootScopeImports);
}

bool QQmlJSImportVisitor::visit(Program *)
{
    Q_ASSERT(m_globalScope == m_currentScope);
    Q_ASSERT(m_exportedRootScope.isNull());
    m_exportedRootScope = m_currentScope;
    m_exportedRootScope->setIsScript(true);
    importBaseModules();
    return true;
}

void QQmlJSImportVisitor::endVisit(Program *)
{
    QQmlJSScope::resolveTypes(m_exportedRootScope, m_rootScopeImports);
}

QT_END_NAMESPACE
