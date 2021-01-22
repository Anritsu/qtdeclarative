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

#ifndef QFINITESTACK_P_H
#define QFINITESTACK_P_H

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

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

template<typename T>
struct QFiniteStack {
    inline QFiniteStack();
    inline ~QFiniteStack();

    inline void deallocate();
    inline void allocate(int size);

    inline int capacity() const { return _alloc; }

    inline bool isEmpty() const;
    inline const T &top() const;
    inline T &top();
    inline void push(const T &o);
    inline T pop();
    inline int count() const;
    inline const T &at(int index) const;
    inline T &operator[](int index);
private:
    T *_array;
    int _alloc;
    int _size;
};

template<typename T>
QFiniteStack<T>::QFiniteStack()
: _array(nullptr), _alloc(0), _size(0)
{
}

template<typename T>
QFiniteStack<T>::~QFiniteStack()
{
    deallocate();
}

template<typename T>
bool QFiniteStack<T>::isEmpty() const
{
    return _size == 0;
}

template<typename T>
const T &QFiniteStack<T>::top() const
{
    return _array[_size - 1];
}

template<typename T>
T &QFiniteStack<T>::top()
{
    return _array[_size - 1];
}

template<typename T>
void QFiniteStack<T>::push(const T &o)
{
    Q_ASSERT(_size < _alloc);
    if (QTypeInfo<T>::isComplex) {
        new (_array + _size++) T(o);
    } else {
        _array[_size++] = o;
    }
}

template<typename T>
T QFiniteStack<T>::pop()
{
    Q_ASSERT(_size > 0);
    --_size;

    if (QTypeInfo<T>::isComplex) {
        T rv = _array[_size];
        (_array + _size)->~T();
        return rv;
    } else {
        return _array[_size];
    }
}

template<typename T>
int QFiniteStack<T>::count() const
{
    return _size;
}

template<typename T>
const T &QFiniteStack<T>::at(int index) const
{
    return _array[index];
}

template<typename T>
T &QFiniteStack<T>::operator[](int index)
{
    return _array[index];
}

template<typename T>
void QFiniteStack<T>::allocate(int size)
{
    Q_ASSERT(_array == nullptr);
    Q_ASSERT(_alloc == 0);
    Q_ASSERT(_size == 0);

    if (!size) return;

    _array = (T *)malloc(size * sizeof(T));
    _alloc = size;
}

template<typename T>
void QFiniteStack<T>::deallocate()
{
    if (QTypeInfo<T>::isComplex) {
        T *i = _array + _size;
        while (i != _array)
            (--i)->~T();
    }

    free(_array);

    _array = nullptr;
    _alloc = 0;
    _size = 0;
}

QT_END_NAMESPACE

#endif // QFINITESTACK_P_H

