/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qv4compileddata_p.h"
#include "qv4jsir_p.h"
#include <private/qv4value_p.h>
#ifndef V4_BOOTSTRAP
#include <private/qv4engine_p.h>
#include <private/qv4function_p.h>
#include <private/qv4objectproto_p.h>
#include <private/qv4lookup_p.h>
#include <private/qv4regexpobject_p.h>
#include <private/qqmlpropertycache_p.h>
#include <private/qqmltypeloader_p.h>
#include <private/qqmlengine_p.h>
#include <private/qqmlcompiler_p.h>
#include <QQmlPropertyMap>
#endif
#include <private/qqmlirbuilder_p.h>
#include <QCoreApplication>

#include <algorithm>

QT_BEGIN_NAMESPACE

namespace QV4 {

namespace CompiledData {

#ifndef V4_BOOTSTRAP
CompilationUnit::CompilationUnit()
    : data(0)
    , engine(0)
    , runtimeStrings(0)
    , runtimeLookups(0)
    , runtimeRegularExpressions(0)
    , runtimeClasses(0)
    , totalBindingsCount(0)
    , totalParserStatusCount(0)
    , totalObjectCount(0)
    , metaTypeId(-1)
    , listMetaTypeId(-1)
    , isRegisteredWithEngine(false)
{}

CompilationUnit::~CompilationUnit()
{
    unlink();
    if (data && !(data->flags & QV4::CompiledData::Unit::StaticData))
        free(data);
    data = 0;
}

QV4::Function *CompilationUnit::linkToEngine(ExecutionEngine *engine)
{
    this->engine = engine;
    engine->compilationUnits.insert(this);

    Q_ASSERT(!runtimeStrings);
    Q_ASSERT(data);
    runtimeStrings = (QV4::Heap::String **)malloc(data->stringTableSize * sizeof(QV4::Heap::String*));
    // memset the strings to 0 in case a GC run happens while we're within the loop below
    memset(runtimeStrings, 0, data->stringTableSize * sizeof(QV4::Heap::String*));
    for (uint i = 0; i < data->stringTableSize; ++i)
        runtimeStrings[i] = engine->newIdentifier(data->stringAt(i));

    runtimeRegularExpressions = new QV4::Value[data->regexpTableSize];
    // memset the regexps to 0 in case a GC run happens while we're within the loop below
    memset(runtimeRegularExpressions, 0, data->regexpTableSize * sizeof(QV4::Value));
    for (uint i = 0; i < data->regexpTableSize; ++i) {
        const CompiledData::RegExp *re = data->regexpAt(i);
        int flags = 0;
        if (re->flags & CompiledData::RegExp::RegExp_Global)
            flags |= IR::RegExp::RegExp_Global;
        if (re->flags & CompiledData::RegExp::RegExp_IgnoreCase)
            flags |= IR::RegExp::RegExp_IgnoreCase;
        if (re->flags & CompiledData::RegExp::RegExp_Multiline)
            flags |= IR::RegExp::RegExp_Multiline;
        runtimeRegularExpressions[i] = engine->newRegExpObject(data->stringAt(re->stringIndex), flags);
    }

    if (data->lookupTableSize) {
        runtimeLookups = new QV4::Lookup[data->lookupTableSize];
        memset(runtimeLookups, 0, data->lookupTableSize * sizeof(QV4::Lookup));
        const CompiledData::Lookup *compiledLookups = data->lookupTable();
        for (uint i = 0; i < data->lookupTableSize; ++i) {
            QV4::Lookup *l = runtimeLookups + i;

            Lookup::Type type = Lookup::Type(compiledLookups[i].type_and_flags);
            if (type == CompiledData::Lookup::Type_Getter)
                l->getter = QV4::Lookup::getterGeneric;
            else if (type == CompiledData::Lookup::Type_Setter)
                l->setter = QV4::Lookup::setterGeneric;
            else if (type == CompiledData::Lookup::Type_GlobalGetter)
                l->globalGetter = QV4::Lookup::globalGetterGeneric;
            else if (type == CompiledData::Lookup::Type_IndexedGetter)
                l->indexedGetter = QV4::Lookup::indexedGetterGeneric;
            else if (type == CompiledData::Lookup::Type_IndexedSetter)
                l->indexedSetter = QV4::Lookup::indexedSetterGeneric;

            for (int j = 0; j < QV4::Lookup::Size; ++j)
                l->classList[j] = 0;
            l->level = -1;
            l->index = UINT_MAX;
            l->nameIndex = compiledLookups[i].nameIndex;
            if (type == CompiledData::Lookup::Type_IndexedGetter || type == CompiledData::Lookup::Type_IndexedSetter)
                l->engine = engine;
        }
    }

    if (data->jsClassTableSize) {
        runtimeClasses = (QV4::InternalClass**)malloc(data->jsClassTableSize * sizeof(QV4::InternalClass*));
        for (uint i = 0; i < data->jsClassTableSize; ++i) {
            int memberCount = 0;
            const CompiledData::JSClassMember *member = data->jsClassAt(i, &memberCount);
            QV4::InternalClass *klass = engine->emptyClass;
            for (int j = 0; j < memberCount; ++j, ++member)
                klass = klass->addMember(runtimeStrings[member->nameOffset]->identifier, member->isAccessor ? QV4::Attr_Accessor : QV4::Attr_Data);

            runtimeClasses[i] = klass;
        }
    }

    linkBackendToEngine(engine);

    if (data->indexOfRootFunction != -1)
        return runtimeFunctions[data->indexOfRootFunction];
    else
        return 0;
}

void CompilationUnit::unlink()
{
    if (engine)
        engine->compilationUnits.erase(engine->compilationUnits.find(this));

    if (isRegisteredWithEngine) {
        Q_ASSERT(data && quint32(propertyCaches.count()) > data->indexOfRootObject && !propertyCaches.at(data->indexOfRootObject).isNull());
        QQmlEnginePrivate *qmlEngine = QQmlEnginePrivate::get(propertyCaches.at(data->indexOfRootObject)->engine);
        qmlEngine->unregisterInternalCompositeType(this);
        isRegisteredWithEngine = false;
    }

    for (int ii = 0; ii < propertyCaches.count(); ++ii)
        if (propertyCaches.at(ii).data())
            propertyCaches.at(ii)->release();
    propertyCaches.clear();

    for (int ii = 0; ii < dependentScripts.count(); ++ii)
        dependentScripts.at(ii)->release();
    dependentScripts.clear();

    importCache = nullptr;

    for (auto resolvedType = resolvedTypes.begin(), end = resolvedTypes.end();
         resolvedType != end; ++resolvedType) {
        if ((*resolvedType)->component)
            (*resolvedType)->component->release();
        if ((*resolvedType)->typePropertyCache)
            (*resolvedType)->typePropertyCache->release();
    }
    qDeleteAll(resolvedTypes);
    resolvedTypes.clear();

    engine = 0;
    free(runtimeStrings);
    runtimeStrings = 0;
    delete [] runtimeLookups;
    runtimeLookups = 0;
    delete [] runtimeRegularExpressions;
    runtimeRegularExpressions = 0;
    free(runtimeClasses);
    runtimeClasses = 0;
    qDeleteAll(runtimeFunctions);
    runtimeFunctions.clear();
}

void CompilationUnit::markObjects(QV4::ExecutionEngine *e)
{
    for (uint i = 0; i < data->stringTableSize; ++i)
        if (runtimeStrings[i])
            runtimeStrings[i]->mark(e);
    if (runtimeRegularExpressions) {
        for (uint i = 0; i < data->regexpTableSize; ++i)
            runtimeRegularExpressions[i].mark(e);
    }
}

void CompilationUnit::destroy()
{
    QQmlEngine *qmlEngine = 0;
    if (engine)
        qmlEngine = engine->qmlEngine();
    if (qmlEngine)
        QQmlEnginePrivate::deleteInEngineThread(qmlEngine, this);
    else
        delete this;
}

IdentifierHash<int> CompilationUnit::namedObjectsPerComponent(int componentObjectIndex)
{
    auto it = namedObjectsPerComponentCache.find(componentObjectIndex);
    if (it == namedObjectsPerComponentCache.end()) {
        IdentifierHash<int> namedObjectCache(engine);
        const CompiledData::Object *component = data->objectAt(componentObjectIndex);
        const quint32 *namedObjectIndexPtr = component->namedObjectsInComponentTable();
        for (quint32 i = 0; i < component->nNamedObjectsInComponent; ++i, ++namedObjectIndexPtr) {
            const CompiledData::Object *namedObject = data->objectAt(*namedObjectIndexPtr);
            namedObjectCache.add(runtimeStrings[namedObject->idNameIndex], namedObject->id);
        }
        it = namedObjectsPerComponentCache.insert(componentObjectIndex, namedObjectCache);
    }
    return *it;
}

#endif // V4_BOOTSTRAP

Unit *CompilationUnit::createUnitData(QmlIR::Document *irDocument)
{
    return irDocument->jsGenerator.generateUnit(QV4::Compiler::JSUnitGenerator::GenerateWithoutStringTable);
}

QString Binding::valueAsString(const Unit *unit) const
{
    switch (type) {
    case Type_Script:
    case Type_String:
        return unit->stringAt(stringIndex);
    case Type_Boolean:
        return value.b ? QStringLiteral("true") : QStringLiteral("false");
    case Type_Number:
        return QString::number(value.d);
    case Type_Invalid:
        return QString();
#ifdef QT_NO_TRANSLATION
    case Type_TranslationById:
    case Type_Translation:
        return unit->stringAt(stringIndex);
#else
    case Type_TranslationById: {
        QByteArray id = unit->stringAt(stringIndex).toUtf8();
        return qtTrId(id.constData(), value.translationData.number);
    }
    case Type_Translation: {
        // This code must match that in the qsTr() implementation
        const QString &path = unit->stringAt(unit->sourceFileIndex);
        int lastSlash = path.lastIndexOf(QLatin1Char('/'));
        QStringRef context = (lastSlash > -1) ? path.midRef(lastSlash + 1, path.length() - lastSlash - 5)
                                              : QStringRef();
        QByteArray contextUtf8 = context.toUtf8();
        QByteArray comment = unit->stringAt(value.translationData.commentIndex).toUtf8();
        QByteArray text = unit->stringAt(stringIndex).toUtf8();
        return QCoreApplication::translate(contextUtf8.constData(), text.constData(),
                                           comment.constData(), value.translationData.number);
    }
#endif
    default:
        break;
    }
    return QString();
}

//reverse of Lexer::singleEscape()
QString Binding::escapedString(const QString &string)
{
    QString tmp = QLatin1String("\"");
    for (int i = 0; i < string.length(); ++i) {
        const QChar &c = string.at(i);
        switch (c.unicode()) {
        case 0x08:
            tmp += QLatin1String("\\b");
            break;
        case 0x09:
            tmp += QLatin1String("\\t");
            break;
        case 0x0A:
            tmp += QLatin1String("\\n");
            break;
        case 0x0B:
            tmp += QLatin1String("\\v");
            break;
        case 0x0C:
            tmp += QLatin1String("\\f");
            break;
        case 0x0D:
            tmp += QLatin1String("\\r");
            break;
        case 0x22:
            tmp += QLatin1String("\\\"");
            break;
        case 0x27:
            tmp += QLatin1String("\\\'");
            break;
        case 0x5C:
            tmp += QLatin1String("\\\\");
            break;
        default:
            tmp += c;
            break;
        }
    }
    tmp += QLatin1Char('\"');
    return tmp;
}

QString Binding::valueAsScriptString(const Unit *unit) const
{
    if (type == Type_String)
        return escapedString(unit->stringAt(stringIndex));
    else
        return valueAsString(unit);
}

#ifndef V4_BOOTSTRAP
/*!
Returns the property cache, if one alread exists.  The cache is not referenced.
*/
QQmlPropertyCache *CompilationUnit::ResolvedTypeReference::propertyCache() const
{
    if (type)
        return typePropertyCache;
    else
        return component->compilationUnit->rootPropertyCache();
}

/*!
Returns the property cache, creating one if it doesn't already exist.  The cache is not referenced.
*/
QQmlPropertyCache *CompilationUnit::ResolvedTypeReference::createPropertyCache(QQmlEngine *engine)
{
    if (typePropertyCache) {
        return typePropertyCache;
    } else if (type) {
        typePropertyCache = QQmlEnginePrivate::get(engine)->cache(type->metaObject());
        typePropertyCache->addref();
        return typePropertyCache;
    } else {
        return component->compilationUnit->rootPropertyCache();
    }
}

template <typename T>
bool qtTypeInherits(const QMetaObject *mo) {
    while (mo) {
        if (mo == &T::staticMetaObject)
            return true;
        mo = mo->superClass();
    }
    return false;
}

void CompilationUnit::ResolvedTypeReference::doDynamicTypeCheck()
{
    const QMetaObject *mo = 0;
    if (typePropertyCache)
        mo = typePropertyCache->firstCppMetaObject();
    else if (type)
        mo = type->metaObject();
    else if (component)
        mo = component->compilationUnit->rootPropertyCache()->firstCppMetaObject();
    isFullyDynamicType = qtTypeInherits<QQmlPropertyMap>(mo);
}
#endif

}

}

QT_END_NAMESPACE
