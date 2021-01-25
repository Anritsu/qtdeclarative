/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "qqmlpropertybinding_p.h"
#include <qqmlinfo.h>

QT_BEGIN_NAMESPACE

QUntypedPropertyBinding QQmlPropertyBinding::create(const QQmlPropertyData *pd, QV4::Function *function,
                                                    QObject *obj, const QQmlRefPointer<QQmlContextData> &ctxt,
                                                    QV4::ExecutionContext *scope, QObject *target, QQmlPropertyIndex targetIndex)
{
    if (auto aotFunction = function->aotFunction; aotFunction && aotFunction->returnType == pd->propType()) {
        return QUntypedPropertyBinding(aotFunction->returnType,
            [
                aotFunction,
                unit = QQmlRefPointer<QV4::ExecutableCompilationUnit>(function->executableCompilationUnit()),
                scopeObject = QPointer<QObject>(obj),
                context = ctxt,
                engine = scope->engine()
            ](const QMetaType &, void *dataPtr) -> bool {
                QQmlPrivate::AOTCompiledContext aotContext;
                aotContext.qmlContext = context->asQQmlContext();
                aotContext.qmlScopeObject = scopeObject.data();
                aotContext.engine = engine->jsEngine();
                aotContext.compilationUnit = unit.data();
                aotFunction->functionPtr(&aotContext, dataPtr, nullptr);
                // ### Fixme: The aotFunction should do the check whether old and new value are the same and
                // return false in that case
                return true;
            },
            QPropertyBindingSourceLocation());
    }

    auto buffer = new std::byte[sizeof(QQmlPropertyBinding)]; // QQmlPropertyBinding uses delete[]
    auto binding = new(buffer) QQmlPropertyBinding(QMetaType(pd->propType()), target, targetIndex);
    binding->setNotifyOnValueChanged(true);
    binding->setContext(ctxt);
    binding->setScopeObject(obj);
    binding->setupFunction(scope, function);
    return QUntypedPropertyBinding(static_cast<QPropertyBindingPrivate *>(QPropertyBindingPrivatePtr(binding).data()));
}

void QQmlPropertyBinding::expressionChanged()
{
    const auto currentTag = m_error.tag();
    if (currentTag & InEvaluationLoop) {
        QQmlError err;
        auto location = QQmlJavaScriptExpression::sourceLocation();
        err.setUrl(QUrl{location.sourceFile});
        err.setLine(location.line);
        err.setColumn(location.column);
        err.setDescription(QString::fromLatin1("Binding loop detected"));
        err.setObject(target());
        qmlWarning(this->scopeObject(), err);
        return;
    }
    m_error.setTag(currentTag | InEvaluationLoop);
    markDirtyAndNotifyObservers();
    m_error.setTag(currentTag);
}

QQmlPropertyBinding::QQmlPropertyBinding(QMetaType mt, QObject *target, QQmlPropertyIndex targetIndex)
    : QPropertyBindingPrivate(mt,
                              &QtPrivate::bindingFunctionVTable<QQmlPropertyBinding>,
                              QPropertyBindingSourceLocation(), true)
{
    static_assert (std::is_trivially_destructible_v<TargetData>);
    static_assert (sizeof(TargetData) + sizeof(DeclarativeErrorCallback) <= sizeof(QPropertyBindingSourceLocation));
    static_assert (alignof(TargetData) <= alignof(QPropertyBindingSourceLocation));
    new (&declarativeExtraData) TargetData {target, targetIndex};
    errorCallBack = bindingErrorCallback;
}

bool QQmlPropertyBinding::evaluate(QMetaType metaType, void *dataPtr)
{
    const auto ctxt = context();
    QQmlEngine *engine = ctxt ? ctxt->engine() : nullptr;
    if (!engine) {
        QPropertyBindingError error(QPropertyBindingError::EvaluationError);
        QPropertyBindingPrivate::currentlyEvaluatingBinding()->setError(std::move(error));
        return false;
    }
    QQmlEnginePrivate *ep = QQmlEnginePrivate::get(engine);
    ep->referenceScarceResources();

    bool isUndefined = false;

    QV4::Scope scope(engine->handle());
    QV4::ScopedValue result(scope, QQmlJavaScriptExpression::evaluate(&isUndefined));

    ep->dereferenceScarceResources();

    if (hasError()) {
        QPropertyBindingError error(QPropertyBindingError::UnknownError, delayedError()->error().description());
        QPropertyBindingPrivate::currentlyEvaluatingBinding()->setError(std::move(error));
        bindingErrorCallback(this);
        return false;
    }

    int propertyType = metaType.id();

    switch (propertyType) {
    case QMetaType::Bool: {
        bool b;
        if (result->isBoolean())
            b = result->booleanValue();
        else
            b = result->toBoolean();
        if (b == *static_cast<bool *>(dataPtr))
            return false;
        *static_cast<bool *>(dataPtr) = b;
        return true;
    }
    case QMetaType::Int: {
        int i;
        if (result->isInteger())
            i = result->integerValue();
        else if (result->isNumber()) {
            i = QV4::StaticValue::toInteger(result->doubleValue());
        } else {
            break;
        }
        if (i == *static_cast<int *>(dataPtr))
            return false;
        *static_cast<int *>(dataPtr) = i;
        return true;
    }
    case QMetaType::Double:
        if (result->isNumber()) {
            double d = result->asDouble();
            if (d == *static_cast<double *>(dataPtr))
                return false;
            *static_cast<double *>(dataPtr) = d;
            return true;
        }
        break;
    case QMetaType::Float:
        if (result->isNumber()) {
            float d = float(result->asDouble());
            if (d == *static_cast<float *>(dataPtr))
                return false;
            *static_cast<float *>(dataPtr) = d;
            return true;
        }
        break;
    case QMetaType::QString:
        if (result->isString()) {
            QString s = result->toQStringNoThrow();
            if (s == *static_cast<QString *>(dataPtr))
                return false;
            *static_cast<QString *>(dataPtr) = s;
            return true;
        }
        break;
    default:
        break;
    }

    QVariant resultVariant(scope.engine->toVariant(result, metaType.id()));
    resultVariant.convert(metaType);
    const bool hasChanged = !metaType.equals(resultVariant.constData(), dataPtr);
    metaType.destruct(dataPtr);
    metaType.construct(dataPtr, resultVariant.constData());
    return hasChanged;
}

QString QQmlPropertyBinding::createBindingLoopErrorDescription(QJSEnginePrivate *ep)
{
    QQmlPropertyData *propertyData = nullptr;
    QQmlPropertyData valueTypeData;
    QQmlData *data = QQmlData::get(target(), false);
    Q_ASSERT(data);
    if (Q_UNLIKELY(!data->propertyCache)) {
        data->propertyCache = ep->cache(target()->metaObject());
        data->propertyCache->addref();
    }
    propertyData = data->propertyCache->property(targetIndex().coreIndex());
    Q_ASSERT(propertyData);
    Q_ASSERT(!targetIndex().hasValueTypeIndex());
    QQmlProperty prop = QQmlPropertyPrivate::restore(target(), *propertyData, &valueTypeData, nullptr);
    return QStringLiteral(R"(QML %1: Binding loop detected for property "%2")").arg(QQmlMetaType::prettyTypeName(target()) , prop.name());
}

QObject *QQmlPropertyBinding::target()
{
    return std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData))->target;
}

QQmlPropertyIndex QQmlPropertyBinding::targetIndex()
{
    return std::launder(reinterpret_cast<TargetData *>(&declarativeExtraData))->targetIndex;
}

void QQmlPropertyBinding::bindingErrorCallback(QPropertyBindingPrivate *that)
{
    auto This = static_cast<QQmlPropertyBinding *>(that);
    auto target = This->target();
    auto engine = qmlEngine(target);
    if (!engine)
        return;
    auto ep = QQmlEnginePrivate::get(engine);

    auto error = This->bindingError();
    QQmlError qmlError;
    auto location = This->QQmlJavaScriptExpression::sourceLocation();
    qmlError.setColumn(location.column);
    qmlError.setLine(location.line);
    qmlError.setUrl(QUrl {location.sourceFile});
    auto description = error.description();
    if (error.type() == QPropertyBindingError::BindingLoop) {
        description = This->createBindingLoopErrorDescription(ep);
    }
    qmlError.setDescription(description);
    qmlError.setObject(target);
    ep->warning(qmlError);
}

QUntypedPropertyBinding QQmlTranslationPropertyBinding::create(const QQmlPropertyData *pd, const QQmlRefPointer<QV4::ExecutableCompilationUnit> &compilationUnit, const QV4::CompiledData::Binding *binding)
{
    auto translationBinding = [compilationUnit, binding](const QMetaType &metaType, void *dataPtr) -> bool {
        // Create a dependency to the uiLanguage
        QJSEnginePrivate::get(compilationUnit->engine)->uiLanguage.value();

        QVariant resultVariant(compilationUnit->bindingValueAsString(binding));
        if (metaType.id() != QMetaType::QString)
            resultVariant.convert(metaType);

        const bool hasChanged = !metaType.equals(resultVariant.constData(), dataPtr);
        metaType.destruct(dataPtr);
        metaType.construct(dataPtr, resultVariant.constData());
        return hasChanged;
    };

    return QUntypedPropertyBinding(QMetaType(pd->propType()), translationBinding, QPropertyBindingSourceLocation());
}

QT_END_NAMESPACE
