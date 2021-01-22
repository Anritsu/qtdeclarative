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

#include "qv4memberdata_p.h"
#include <private/qv4mm_p.h>
#include "qv4value_p.h"

using namespace QV4;

DEFINE_MANAGED_VTABLE(MemberData);

static size_t nextPowerOfTwo(size_t s)
{
    --s;
    s |= s >> 1;
    s |= s >> 2;
    s |= s >> 4;
    s |= s >> 8;
    s |= s >> 16;
#if (QT_POINTER_SIZE == 8)
        s |= s >> 32;
#endif
    ++s;
    return s;
}

Heap::MemberData *MemberData::allocate(ExecutionEngine *e, uint n, Heap::MemberData *old)
{
    Q_ASSERT(!old || old->values.size <= n);
    if (!n)
        n = 4;

    size_t alloc = MemoryManager::align(sizeof(Heap::MemberData) + (n - 1)*sizeof(Value));
    // round up to next power of two to avoid quadratic behaviour for very large objects
    alloc = nextPowerOfTwo(alloc);

    // The above code can overflow in a number of interesting ways. All of those are unsigned,
    // and therefore defined behavior. Still, apply some sane bounds.
    const size_t intMax = std::numeric_limits<int>::max();
    if (alloc > intMax)
        alloc = intMax;

    Heap::MemberData *m;
    if (old) {
        const size_t oldSize = sizeof(Heap::MemberData) + (old->values.size - 1) * sizeof(Value);
        if (oldSize > alloc)
            alloc = oldSize;
        m = e->memoryManager->allocManaged<MemberData>(alloc);
        // no write barrier required here
        memcpy(m, old, oldSize);
    } else {
        m = e->memoryManager->allocManaged<MemberData>(alloc);
        m->init();
    }

    m->values.alloc = static_cast<uint>((alloc - sizeof(Heap::MemberData) + sizeof(Value))/sizeof(Value));
    m->values.size = m->values.alloc;
    return m;
}
