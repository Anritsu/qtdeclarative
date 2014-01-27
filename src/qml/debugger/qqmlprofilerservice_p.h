/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQml module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQMLPROFILERSERVICE_P_H
#define QQMLPROFILERSERVICE_P_H

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

#include <private/qqmldebugservice_p.h>
#include <private/qqmlboundsignal_p.h>
// this contains QUnifiedTimer
#include <private/qabstractanimation_p.h>
#include <private/qv4function_p.h>

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvector.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qwaitcondition.h>

#define Q_QML_PROFILE_IF_ENABLED(Code)\
    if (QQmlProfilerService::enabled) {\
        Code;\
    } else\
        (void)0

#define Q_QML_PROFILE(Method)\
    Q_QML_PROFILE_IF_ENABLED(QQmlProfilerService::Method)

#define Q_QML_SG_PROFILE2(Type1, Type2, Params)\
    Q_QML_PROFILE_IF_ENABLED((QQmlProfilerService::sceneGraphFrame<Type1, Type2> Params))

#define Q_QML_SG_PROFILE1(Type, Params) Q_QML_SG_PROFILE2(Type, Type, Params)

QT_BEGIN_NAMESPACE

// This struct is somewhat dangerous to use:
// You can save values either with 32 or 64 bit precision. toByteArrays will
// guess the precision from messageType. If you state the wrong messageType
// you will get undefined results.
// The messageType is itself a bit field. You can pack multiple messages into
// one object, e.g. RangeStart and RangeLocation. Each one will be read
// independently by toByteArrays. Thus you can only pack messages if their data
// doesn't overlap. Again, it's up to you to figure that out.
struct Q_AUTOTEST_EXPORT QQmlProfilerData
{
    QQmlProfilerData() {}

    QQmlProfilerData(qint64 time, int messageType, int detailType, const QUrl &url,
                     int x = 0, int y = 0, int framerate = 0, int count = 0, int bindingType = 0) :
        time(time), messageType(messageType), detailType(detailType), detailUrl(url),
        x(x), y(y), framerate(framerate), count(count), bindingType(bindingType) {}

    QQmlProfilerData(qint64 time, int messageType, int detailType, const QString &str,
                     int x = 0, int y = 0, int framerate = 0, int count = 0, int bindingType = 0) :
        time(time), messageType(messageType), detailType(detailType),detailString(str),
        x(x), y(y), framerate(framerate), count(count), bindingType(bindingType) {}

    QQmlProfilerData(qint64 time, int messageType, int detailType, const QString &str,
                     const QUrl &url, int x = 0, int y = 0, int framerate = 0, int count = 0,
                     int bindingType = 0) :
        time(time), messageType(messageType), detailType(detailType), detailString(str),
        detailUrl(url), x(x), y(y), framerate(framerate), count(count), bindingType(bindingType) {}


    QQmlProfilerData(qint64 time, int messageType, int detailType) :
        time(time), messageType(messageType), detailType(detailType) {}

    // Special ctor for scenegraph frames. Note that it's missing the QString/QUrl params.
    // This is slightly ugly, but makes it easier to disambiguate between int and qint64 params.
    QQmlProfilerData(qint64 time, int messageType, int detailType,
                     qint64 d1, qint64 d2, qint64 d3, qint64 d4, qint64 d5) :
        time(time), messageType(messageType), detailType(detailType),
        subtime_1(d1), subtime_2(d2), subtime_3(d3), subtime_4(d4), subtime_5(d5) {}


    qint64 time;
    int messageType;        //bit field of QQmlProfilerService::Message
    int detailType;

    QString detailString;   //used by RangeData and possibly by RangeLocation
    QUrl detailUrl;         //used by RangeLocation, overrides detailString

    union {
        qint64 subtime_1;
        int x;              //used by RangeLocation and for pixmaps
    };

    union {
        qint64 subtime_2;
        int y;              //used by RangeLocation and for pixmaps
    };

    union {
        qint64 subtime_3;
        int framerate;      //used by animation events
    };

    union {
        qint64 subtime_4;
        int count;          //used by animation events and for pixmaps
    };

    union {
        qint64 subtime_5;
        int bindingType;
    };

    void toByteArrays(QList<QByteArray> &messages) const;
};

Q_DECLARE_TYPEINFO(QQmlProfilerData, Q_MOVABLE_TYPE);

class QUrl;
class QQmlEngine;


class Q_QML_PRIVATE_EXPORT QQmlProfilerService : public QQmlDebugService
{
public:
    enum Message {
        Event,
        RangeStart,
        RangeData,
        RangeLocation,
        RangeEnd,
        Complete, // end of transmission
        PixmapCacheEvent,
        SceneGraphFrame,

        MaximumMessage
    };

    enum EventType {
        FramePaint,
        Mouse,
        Key,
        AnimationFrame,
        EndTrace,
        StartTrace,

        MaximumEventType
    };

    enum RangeType {
        Painting,
        Compiling,
        Creating,
        Binding,            //running a binding
        HandlingSignal,     //running a signal handler

        MaximumRangeType
    };

    enum BindingType {
        QmlBinding,
        V8Binding,
        V4Binding,

        MaximumBindingType
    };

    enum PixmapEventType {
        PixmapSizeKnown,
        PixmapReferenceCountChanged,
        PixmapCacheCountChanged,
        PixmapLoadingStarted,
        PixmapLoadingFinished,
        PixmapLoadingError,

        MaximumPixmapEventType
    };

    enum SceneGraphFrameType {
        SceneGraphRendererFrame,
        SceneGraphAdaptationLayerFrame,
        SceneGraphContextFrame,
        SceneGraphRenderLoopFrame,
        SceneGraphTexturePrepare,
        SceneGraphTextureDeletion,
        SceneGraphPolishAndSync,
        SceneGraphWindowsRenderShow,
        SceneGraphWindowsAnimations,
        SceneGraphWindowsPolishFrame,

        MaximumSceneGraphFrameType
    };

    static void initialize();

    static bool startProfiling();
    static bool stopProfiling();

    template<EventType DetailType>
    static void addEvent()
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << Event,
                                                  1 << DetailType));
    }

    static void animationFrame(qint64 delta)
    {
        int animCount = QUnifiedTimer::instance()->runningAnimationCount();

        if (animCount > 0 && delta > 0) {
            instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << Event,
                                                      1 << AnimationFrame, QString(), 0, 0,
                                                      1000 / (int)delta /* trim fps to integer */,
                                                      animCount));
        }
    }

    template<SceneGraphFrameType FrameType1, SceneGraphFrameType FrameType2>
    static void sceneGraphFrame(qint64 value1, qint64 value2 = -1, qint64 value3 = -1,
                                qint64 value4 = -1, qint64 value5 = -1)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << SceneGraphFrame,
                                                  1 << FrameType1 | 1 << FrameType2,
                                                  value1, value2, value3, value4, value5));
    }

    template<PixmapEventType PixmapState>
    static void pixmapStateChanged(const QUrl &url)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << PixmapCacheEvent,
                                                  1 << PixmapState, url));
    }

    static void pixmapLoadingFinished(const QUrl &url, const QSize &size)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << PixmapCacheEvent,
                (1 << PixmapLoadingFinished) | ((size.width() > 0 && size.height() > 0) ? (1 << PixmapSizeKnown) : 0),
                url, size.width(), size.height()));
    }

    template<PixmapEventType CountType>
    static void pixmapCountChanged(const QUrl &url, int count)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << PixmapCacheEvent,
                                                  1 << CountType, url, 0, 0, 0, count));
    }

    qint64 timestamp() {return m_timer.nsecsElapsed();}

    static void sendProfilingData();

    QQmlProfilerService();
    ~QQmlProfilerService();

protected:
    virtual void stateAboutToBeChanged(State state);
    virtual void messageReceived(const QByteArray &);

private:
    bool startProfilingImpl();
    bool stopProfilingImpl();

    static void startBinding(const QString &fileName, int line, int column, BindingType bindingType)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeLocation),
                                                  1 << Binding, fileName, line, column, 0, 0,
                                                  bindingType));
    }

    // Have toByteArrays() construct another RangeData event from the same QString later.
    // This is somewhat pointless but important for backwards compatibility.
    static void startCompiling(const QString &name)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(),
                       (1 << RangeStart | 1 << RangeLocation | 1 << RangeData), 1 << Compiling,
                       name, 1, 1, 0, 0, QmlBinding));
    }

    static void startHandlingSignal(const QString &fileName, int line, int column)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeLocation),
                                                  1 << HandlingSignal, fileName, line, column, 0, 0,
                                                  QmlBinding));
    }

    static void startCreating(const QString &typeName, const QUrl &fileName, int line, int column)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeLocation | 1 << RangeData),
                                                  1 << Creating, typeName, fileName, line, column,
                                                  0, 0, QmlBinding));
    }

    static void startCreating(const QString &typeName)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(),
                                                  (1 << RangeStart | 1 << RangeData), 1 << Creating,
                                                  typeName, 0, 0, 0, 0, QmlBinding));
    }

    static void creatingLocation(const QUrl &fileName, int line, int column)
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << RangeLocation,
                                                  1 << Creating, fileName, line, column));
    }

    template<RangeType Range>
    static void endRange()
    {
        instance->processMessage(QQmlProfilerData(instance->timestamp(), 1 << RangeEnd,
                                                  1 << Range));
    }

    void sendMessages();

    void processMessage(const QQmlProfilerData &message)
    {
        QMutexLocker locker(&m_dataMutex);
        m_data.append(message);
    }

    static void animationTimerCallback(qint64 delta);

public:
    static bool enabled;
private:
    QElapsedTimer m_timer;
    QVector<QQmlProfilerData> m_data;
    QMutex m_dataMutex;
    QMutex m_initializeMutex;
    QWaitCondition m_initializeCondition;

    static QQmlProfilerService *instance;

    friend struct QQmlBindingProfiler;
    friend struct QQmlHandlingSignalProfiler;
    friend struct QQmlVmeProfiler;
    friend struct QQmlCompilingProfiler;
};

//
// RAII helper structs
//

struct QQmlBindingProfiler {
    QQmlBindingProfiler(const QString &url, int line, int column, QQmlProfilerService::BindingType bindingType)
    {
        Q_QML_PROFILE(startBinding(url, line, column, bindingType));
    }

    ~QQmlBindingProfiler()
    {
        Q_QML_PROFILE(endRange<QQmlProfilerService::Binding>());
    }
};

struct QQmlHandlingSignalProfiler {
    QQmlHandlingSignalProfiler(QQmlBoundSignalExpression *expression)
    {
        Q_QML_PROFILE_IF_ENABLED({
            QV4::Function *function;
            if (expression->sourceFile().isEmpty() && (function = expression->function())) {
                QQmlProfilerService::startHandlingSignal(
                        function->sourceFile(), function->compiledFunction->location.line,
                        function->compiledFunction->location.column);

            } else {
                QQmlProfilerService::startHandlingSignal(
                        expression->sourceFile(), expression->lineNumber(),
                        expression->columnNumber());
            }
        });
    }

    ~QQmlHandlingSignalProfiler()
    {
        Q_QML_PROFILE(endRange<QQmlProfilerService::HandlingSignal>());
    }
};

struct QQmlCompilingProfiler {
    QQmlCompilingProfiler(const QString &name)
    {
        Q_QML_PROFILE(startCompiling(name));
    }

    ~QQmlCompilingProfiler()
    {
        Q_QML_PROFILE(endRange<QQmlProfilerService::Compiling>());
    }
};

#define Q_QML_VME_PROFILE(Method) Q_QML_PROFILE_IF_ENABLED(Method)

struct QQmlVmeProfiler {
public:

    struct Data {
        Data() : line(0), column(0) {}
        QUrl url;
        int line;
        int column;
        QString typeName;
    };

    QQmlVmeProfiler() : running(false) {}

    void clear()
    {
        ranges.clear();
        if (running)
            QQmlProfilerService::instance->endRange<QQmlProfilerService::Creating>();
        for (int i = 0; i < backgroundRanges.count(); ++i) {
            QQmlProfilerService::instance->endRange<QQmlProfilerService::Creating>();
        }
        backgroundRanges.clear();
        running = false;
    }

    void startBackground(const QString &typeName)
    {
        if (running) {
            QQmlProfilerService::instance->endRange<QQmlProfilerService::Creating>();
            running = false;
        }
        QQmlProfilerService::instance->startCreating(typeName);
        backgroundRanges.push(typeName);
    }

    void start(const QString &typeName, const QUrl &url, int line, int column)
    {
        switchRange();
        setCurrentRange(typeName, url, line, column);
        QQmlProfilerService::instance->startCreating(typeName, url, line, column);
    }

    void stop()
    {
        if (running) {
            QQmlProfilerService::instance->endRange<QQmlProfilerService::Creating>();
            running = false;
        }
    }

    void pop()
    {
        if (ranges.count() > 0) {
            switchRange();
            currentRange = ranges.pop();
            QQmlProfilerService::instance->startCreating(currentRange.typeName, currentRange.url,
                                                         currentRange.line, currentRange.column);
        }
    }

    void push()
    {
        if (running)
            ranges.push(currentRange);
    }

    void foreground(const QUrl &url, int line, int column)
    {
        if (backgroundRanges.count() > 0) {
            switchRange();
            setCurrentRange(backgroundRanges.pop(), url, line, column);
            QQmlProfilerService::instance->creatingLocation(url, line, column);
        }
    }

private:

    void switchRange()
    {
        if (running)
            QQmlProfilerService::instance->endRange<QQmlProfilerService::Creating>();
        else
            running = true;
    }

    void setCurrentRange(const QString &typeName, const QUrl &url, int line, int column)
    {
        currentRange.typeName = typeName;
        currentRange.url = url;
        currentRange.line = line;
        currentRange.column = column;
    }

    Data currentRange;
    QStack<Data> ranges;
    QStack<QString> backgroundRanges;
    bool running;
};

QT_END_NAMESPACE

#endif // QQMLPROFILERSERVICE_P_H

