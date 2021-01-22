/****************************************************************************
**
** Copyright (C) 2018 Crimson AS <info@crimson.no>
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

#ifndef QV4SETOBJECT_P_H
#define QV4SETOBJECT_P_H

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

#include "qv4object_p.h"
#include "qv4objectproto_p.h"
#include "qv4functionobject_p.h"
#include "qv4string_p.h"

QT_BEGIN_NAMESPACE

namespace QV4 {

class ESTable;

namespace Heap {

struct WeakSetCtor : FunctionObject {
    void init(QV4::ExecutionContext *scope);
};


struct SetCtor : WeakSetCtor {
    void init(QV4::ExecutionContext *scope);
};

struct SetObject : Object {
    static void markObjects(Heap::Base *that, MarkStack *markStack);
    void init();
    void destroy();
    void removeUnmarkedKeys();

    ESTable *esTable;
    SetObject *nextWeakSet;
    bool isWeakSet;
};

}


struct WeakSetCtor: FunctionObject
{
    V4_OBJECT2(WeakSetCtor, FunctionObject)

    static ReturnedValue construct(const FunctionObject *f, const Value *argv, int argc, const Value *, bool weakSet);

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
    static ReturnedValue virtualCall(const FunctionObject *f, const Value *thisObject, const Value *argv, int argc);
};

struct SetCtor : WeakSetCtor
{
    V4_OBJECT2(SetCtor, WeakSetCtor)

    static ReturnedValue virtualCallAsConstructor(const FunctionObject *f, const Value *argv, int argc, const Value *);
};

struct SetObject : Object
{
    V4_OBJECT2(SetObject, Object)
    V4_PROTOTYPE(setPrototype)
    V4_NEEDS_DESTROY
};

struct WeakSetPrototype : Object
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_add(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


struct SetPrototype : WeakSetPrototype
{
    void init(ExecutionEngine *engine, Object *ctor);

    static ReturnedValue method_add(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_clear(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_delete(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_entries(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_forEach(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_has(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_get_size(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
    static ReturnedValue method_values(const FunctionObject *, const Value *thisObject, const Value *argv, int argc);
};


} // namespace QV4


QT_END_NAMESPACE

#endif // QV4SETOBJECT_P_H
