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

#include "qqmljslinter_p.h"

#include "qqmljslintercodegen_p.h"

#include <QtQmlCompiler/private/qqmljsimporter_p.h>
#include <QtQmlCompiler/private/qqmljsimportvisitor_p.h>

#include <QtCore/qjsonobject.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qloggingcategory.h>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>
#include <QtQml/private/qqmljsdiagnosticmessage_p.h>

QT_BEGIN_NAMESPACE

class CodegenWarningInterface final : public QV4::Compiler::CodegenWarningInterface
{
public:
    CodegenWarningInterface(QQmlJSLogger *logger) : m_logger(logger) { }

    void reportVarUsedBeforeDeclaration(const QString &name, const QString &fileName,
                                        QQmlJS::SourceLocation declarationLocation,
                                        QQmlJS::SourceLocation accessLocation) override
    {
        Q_UNUSED(fileName)
        m_logger->log(
                u"Variable \"%1\" is used here before its declaration. The declaration is at %2:%3."_qs
                        .arg(name)
                        .arg(declarationLocation.startLine)
                        .arg(declarationLocation.startColumn),
                Log_Type, accessLocation);
    }

private:
    QQmlJSLogger *m_logger;
};

QQmlJSLinter::QQmlJSLinter(const QStringList &importPaths, bool useAbsolutePath)
    : m_useAbsolutePath(useAbsolutePath), m_importer(importPaths, nullptr, true)
{
}

void QQmlJSLinter::parseComments(QQmlJSLogger *logger,
                                 const QList<QQmlJS::SourceLocation> &comments)
{
    QHash<int, QSet<QQmlJSLoggerCategory>> disablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> enablesPerLine;
    QHash<int, QSet<QQmlJSLoggerCategory>> oneLineDisablesPerLine;

    const QString code = logger->code();
    const QStringList lines = code.split(u'\n');

    for (const auto &loc : comments) {
        const QString comment = code.mid(loc.offset, loc.length);
        if (!comment.startsWith(u" qmllint ") && !comment.startsWith(u"qmllint "))
            continue;

        QStringList words = comment.split(u' ');
        if (words.constFirst().isEmpty())
            words.removeFirst();

        const QString command = words.at(1);

        QSet<QQmlJSLoggerCategory> categories;
        for (qsizetype i = 2; i < words.size(); i++) {
            const QString category = words.at(i);
            const auto option = logger->options().constFind(category);
            if (option != logger->options().constEnd())
                categories << option->m_category;
            else
                logger->log(u"qmllint directive on unknown category \"%1\""_qs.arg(category),
                            Log_Syntax, loc);
        }

        if (categories.isEmpty()) {
            for (const auto &option : logger->options())
                categories << option.m_category;
        }

        if (command == u"disable"_qs) {
            const QString line = lines[loc.startLine - 1];
            const QString preComment = line.left(line.indexOf(comment) - 2);

            bool lineHasContent = false;
            for (qsizetype i = 0; i < preComment.size(); i++) {
                if (!preComment[i].isSpace()) {
                    lineHasContent = true;
                    break;
                }
            }

            if (lineHasContent)
                oneLineDisablesPerLine[loc.startLine] |= categories;
            else
                disablesPerLine[loc.startLine] |= categories;
        } else if (command == u"enable"_qs) {
            enablesPerLine[loc.startLine + 1] |= categories;
        } else {
            logger->log(u"Invalid qmllint directive \"%1\" provided"_qs.arg(command), Log_Syntax,
                        loc);
        }
    }

    if (disablesPerLine.isEmpty() && oneLineDisablesPerLine.isEmpty())
        return;

    QSet<QQmlJSLoggerCategory> currentlyDisabled;
    for (qsizetype i = 1; i <= lines.length(); i++) {
        currentlyDisabled.unite(disablesPerLine[i]).subtract(enablesPerLine[i]);

        currentlyDisabled.unite(oneLineDisablesPerLine[i]);

        if (!currentlyDisabled.isEmpty())
            logger->ignoreWarnings(i, currentlyDisabled);

        currentlyDisabled.subtract(oneLineDisablesPerLine[i]);
    }
}

bool QQmlJSLinter::lintFile(const QString &filename, const QString *fileContents, const bool silent,
                            QJsonArray *json, const QStringList &qmlImportPaths,
                            const QStringList &qmldirFiles, const QStringList &resourceFiles,
                            const QMap<QString, QQmlJSLogger::Option> &options)
{
    // Make sure that we don't expose an old logger if we return before a new one is created.
    m_logger.reset();

    QJsonArray warnings;
    QJsonObject result;

    bool success = true;

    QScopeGuard jsonOutput([&] {
        if (!json)
            return;

        result[u"filename"_qs] = QFileInfo(filename).absoluteFilePath();
        result[u"warnings"] = warnings;
        result[u"success"] = success;

        json->append(result);
    });

    auto addJsonWarning = [&](const QQmlJS::DiagnosticMessage &message,
                              const std::optional<FixSuggestion> &suggestion = {}) {
        QJsonObject jsonMessage;

        QString type;
        switch (message.type) {
        case QtDebugMsg:
            type = u"debug"_qs;
            break;
        case QtWarningMsg:
            type = u"warning"_qs;
            break;
        case QtCriticalMsg:
            type = u"critical"_qs;
            break;
        case QtFatalMsg:
            type = u"fatal"_qs;
            break;
        case QtInfoMsg:
            type = u"info"_qs;
            break;
        default:
            type = u"unknown"_qs;
            break;
        }

        jsonMessage[u"type"_qs] = type;

        if (message.loc.isValid()) {
            jsonMessage[u"line"_qs] = static_cast<int>(message.loc.startLine);
            jsonMessage[u"column"_qs] = static_cast<int>(message.loc.startColumn);
            jsonMessage[u"charOffset"_qs] = static_cast<int>(message.loc.offset);
            jsonMessage[u"length"_qs] = static_cast<int>(message.loc.length);
        }

        jsonMessage[u"message"_qs] = message.message;

        QJsonArray suggestions;
        if (suggestion.has_value()) {
            for (const auto &fix : suggestion->fixes) {
                QJsonObject jsonFix;
                jsonFix[u"message"] = fix.message;
                jsonFix[u"line"_qs] = static_cast<int>(fix.cutLocation.startLine);
                jsonFix[u"column"_qs] = static_cast<int>(fix.cutLocation.startColumn);
                jsonFix[u"charOffset"_qs] = static_cast<int>(fix.cutLocation.offset);
                jsonFix[u"length"_qs] = static_cast<int>(fix.cutLocation.length);
                jsonFix[u"replacement"_qs] = fix.replacementString;
                suggestions << jsonFix;
            }
        }
        jsonMessage[u"suggestions"] = suggestions;

        warnings << jsonMessage;
    };

    QString code;

    if (fileContents == nullptr) {
        QFile file(filename);
        if (!file.open(QFile::ReadOnly)) {
            if (json) {
                addJsonWarning(
                        QQmlJS::DiagnosticMessage { QStringLiteral("Failed to open file %1: %2")
                                                            .arg(filename, file.errorString()),
                                                    QtCriticalMsg, QQmlJS::SourceLocation() });
                success = false;
            } else if (!silent) {
                qWarning() << "Failed to open file" << filename << file.error();
            }
            return false;
        }

        code = QString::fromUtf8(file.readAll());
        file.close();
    } else {
        code = *fileContents;
    }

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    QFileInfo info(filename);
    const QString lowerSuffix = info.suffix().toLower();
    const bool isESModule = lowerSuffix == QLatin1String("mjs");
    const bool isJavaScript = isESModule || lowerSuffix == QLatin1String("js");

    lexer.setCode(code, /*lineno = */ 1, /*qmlMode=*/!isJavaScript);
    QQmlJS::Parser parser(&engine);

    success = isJavaScript ? (isESModule ? parser.parseModule() : parser.parseProgram())
                           : parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            if (json) {
                addJsonWarning(m);
            } else if (!silent) {
                qWarning().noquote() << QString::fromLatin1("%1:%2:%3: %4")
                                                .arg(filename)
                                                .arg(m.loc.startLine)
                                                .arg(m.loc.startColumn)
                                                .arg(m.message);
            }
        }
    }

    if (success && !isJavaScript) {
        const auto processMessages = [&]() {
            if (json) {
                for (const auto &error : m_logger->errors())
                    addJsonWarning(error, error.fixSuggestion);
                for (const auto &warning : m_logger->warnings())
                    addJsonWarning(warning, warning.fixSuggestion);
                for (const auto &info : m_logger->infos())
                    addJsonWarning(info, info.fixSuggestion);
            }
        };

        const auto check = [&](QQmlJSResourceFileMapper *mapper) {
            if (m_importer.importPaths() != qmlImportPaths)
                m_importer.setImportPaths(qmlImportPaths);

            m_importer.setResourceFileMapper(mapper);

            m_logger.reset(new QQmlJSLogger);
            m_logger->setFileName(m_useAbsolutePath ? info.absoluteFilePath() : filename);
            m_logger->setCode(code);
            m_logger->setSilent(silent || json);
            QQmlJSImportVisitor v { &m_importer, m_logger.get(),
                                    QQmlJSImportVisitor::implicitImportDirectory(
                                            m_logger->fileName(), m_importer.resourceFileMapper()),
                                    qmldirFiles };

            parseComments(m_logger.get(), engine.comments());

            for (auto it = options.cbegin(); it != options.cend(); ++it) {
                if (!it.value().m_changed)
                    continue;

                m_logger->setCategoryIgnored(it.value().m_category, it.value().m_ignored);
                m_logger->setCategoryLevel(it.value().m_category, it.value().m_level);
            }

            QQmlJSTypeResolver typeResolver(&m_importer);

            // Type resolving is using document parent mode here so that it produces fewer false
            // positives on the "parent" property of QQuickItem. It does produce a few false
            // negatives this way because items can be reparented. Furthermore, even if items are
            // not reparented, the document parent may indeed not be their visual parent. See
            // QTBUG-95530. Eventually, we'll need cleverer logic to deal with this.
            typeResolver.setParentMode(QQmlJSTypeResolver::UseDocumentParent);

            typeResolver.init(&v, parser.rootNode());
            success = !m_logger->hasWarnings() && !m_logger->hasErrors();

            if (m_logger->hasErrors()) {
                processMessages();
                return;
            }

            QQmlJSTypeInfo typeInfo;

            const QStringList resourcePaths = mapper
                    ? mapper->resourcePaths(QQmlJSResourceFileMapper::localFileFilter(filename))
                    : QStringList();
            const QString resolvedPath =
                    (resourcePaths.size() == 1) ? u':' + resourcePaths.first() : filename;

            QQmlJSLinterCodegen codegen { &m_importer, resolvedPath, qmldirFiles, m_logger.get(),
                                          &typeInfo };
            codegen.setTypeResolver(std::move(typeResolver));
            QQmlJSSaveFunction saveFunction = [](const QV4::CompiledData::SaveableUnitPointer &,
                                                 const QQmlJSAotFunctionMap &,
                                                 QString *) { return true; };

            QQmlJSCompileError error;

            QLoggingCategory::setFilterRules(u"qt.qml.compiler=false"_qs);

            CodegenWarningInterface interface(m_logger.get());
            qCompileQmlFile(filename, saveFunction, &codegen, &error, true, &interface,
                            fileContents);

            QList<QQmlJS::DiagnosticMessage> warnings = m_importer.takeGlobalWarnings();

            if (!warnings.isEmpty()) {
                m_logger->log(QStringLiteral("Type warnings occurred while evaluating file:"),
                              Log_Import, QQmlJS::SourceLocation());
                m_logger->processMessages(warnings, Log_Import);
            }

            success &= !m_logger->hasWarnings() && !m_logger->hasErrors();

            processMessages();
        };

        if (resourceFiles.isEmpty()) {
            check(nullptr);
        } else {
            QQmlJSResourceFileMapper mapper(resourceFiles);
            check(&mapper);
        }
    }

    return success;
}

QT_END_NAMESPACE
