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

#ifndef QQMLJSAST_FWD_P_H
#define QQMLJSAST_FWD_P_H

#include "qqmljsglobal_p.h"
#include <private/qqmljssourcelocation_p.h>

#include <QtCore/qglobal.h>

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

QT_BEGIN_NAMESPACE

namespace QQmlJS { namespace AST {

class BaseVisitor;
class Visitor;
class Node;
class ExpressionNode;
class Statement;
class ThisExpression;
class IdentifierExpression;
class NullExpression;
class TrueLiteral;
class FalseLiteral;
class SuperLiteral;
class NumericLiteral;
class StringLiteral;
class TemplateLiteral;
class RegExpLiteral;
class Pattern;
class ArrayPattern;
class ObjectPattern;
class PatternElement;
class PatternElementList;
class PatternProperty;
class PatternPropertyList;
class Elision;
class PropertyName;
class IdentifierPropertyName;
class StringLiteralPropertyName;
class NumericLiteralPropertyName;
class ComputedPropertyName;
class ArrayMemberExpression;
class FieldMemberExpression;
class TaggedTemplate;
class NewMemberExpression;
class NewExpression;
class CallExpression;
class ArgumentList;
class PostIncrementExpression;
class PostDecrementExpression;
class DeleteExpression;
class VoidExpression;
class TypeOfExpression;
class PreIncrementExpression;
class PreDecrementExpression;
class UnaryPlusExpression;
class UnaryMinusExpression;
class TildeExpression;
class NotExpression;
class BinaryExpression;
class ConditionalExpression;
class Expression; // ### rename
class YieldExpression;
class Block;
class LeftHandSideExpression;
class StatementList;
class VariableStatement;
class VariableDeclarationList;
class EmptyStatement;
class ExpressionStatement;
class IfStatement;
class DoWhileStatement;
class WhileStatement;
class ForStatement;
class ForEachStatement;
class ContinueStatement;
class BreakStatement;
class ReturnStatement;
class WithStatement;
class SwitchStatement;
class CaseBlock;
class CaseClauses;
class CaseClause;
class DefaultClause;
class LabelledStatement;
class ThrowStatement;
class TryStatement;
class Catch;
class Finally;
class FunctionDeclaration;
class FunctionExpression;
class FormalParameterList;
class ExportSpecifier;
class ExportsList;
class ExportClause;
class ExportDeclaration;
class Program;
class ImportSpecifier;
class ImportsList;
class NamedImports;
class NameSpaceImport;
class NamedImport;
class ImportClause;
class FromClause;
class ImportDeclaration;
class ESModule;
class DebuggerStatement;
class NestedExpression;
class ClassExpression;
class ClassDeclaration;
class ClassElementList;
class TypeArgumentList;
class Type;
class TypeAnnotation;

// ui elements
class UiProgram;
class UiPragma;
class UiImport;
class UiPublicMember;
class UiParameterList;
class UiObjectDefinition;
class UiInlineComponent;
class UiObjectInitializer;
class UiObjectBinding;
class UiScriptBinding;
class UiSourceElement;
class UiArrayBinding;
class UiObjectMember;
class UiObjectMemberList;
class UiArrayMemberList;
class UiQualifiedId;
class UiHeaderItemList;
class UiEnumDeclaration;
class UiEnumMemberList;
class UiVersionSpecifier;
class UiRequired;
class UiAnnotation;
class UiAnnotationList;

} // namespace AST
} // namespace QQmlJS

QT_END_NAMESPACE

#endif
