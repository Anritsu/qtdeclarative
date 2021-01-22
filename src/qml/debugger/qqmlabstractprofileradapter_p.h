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

#ifndef QQMLABSTRACTPROFILERADAPTER_P_H
#define QQMLABSTRACTPROFILERADAPTER_P_H

#include <private/qtqmlglobal_p.h>
#include <private/qqmlprofilerdefinitions_p.h>

#include <QtCore/QObject>
#include <QtCore/QElapsedTimer>

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

QT_REQUIRE_CONFIG(qml_debug);

class QQmlProfilerService;
class Q_QML_PRIVATE_EXPORT QQmlAbstractProfilerAdapter : public QObject, public QQmlProfilerDefinitions {
    Q_OBJECT

public:
    static const int s_numMessagesPerBatch = 1000;

    QQmlAbstractProfilerAdapter(QObject *parent = nullptr) :
        QObject(parent), service(nullptr), waiting(true), featuresEnabled(0) {}
    ~QQmlAbstractProfilerAdapter() override {}
    void setService(QQmlProfilerService *new_service) { service = new_service; }

    virtual qint64 sendMessages(qint64 until, QList<QByteArray> &messages) = 0;

    void startProfiling(quint64 features);

    void stopProfiling();

    void reportData() { emit dataRequested(); }

    void stopWaiting() { waiting = false; }
    void startWaiting() { waiting = true; }

    bool isRunning() const { return featuresEnabled != 0; }
    quint64 features() const { return featuresEnabled; }

    void synchronize(const QElapsedTimer &t) { emit referenceTimeKnown(t); }

signals:
    void profilingEnabled(quint64 features);
    void profilingEnabledWhileWaiting(quint64 features);

    void profilingDisabled();
    void profilingDisabledWhileWaiting();

    void dataRequested();
    void referenceTimeKnown(const QElapsedTimer &timer);

protected:
    QQmlProfilerService *service;

private:
    bool waiting;
    quint64 featuresEnabled;
};

class Q_QML_PRIVATE_EXPORT QQmlAbstractProfilerAdapterFactory : public QObject
{
    Q_OBJECT
public:
    virtual QQmlAbstractProfilerAdapter *create(const QString &key) = 0;
};

#define QQmlAbstractProfilerAdapterFactory_iid "org.qt-project.Qt.QQmlAbstractProfilerAdapterFactory"

QT_END_NAMESPACE

#endif // QQMLABSTRACTPROFILERADAPTER_P_H
