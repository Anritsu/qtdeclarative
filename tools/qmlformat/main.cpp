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

#include <QCoreApplication>
#include <QFile>

#include <QtQml/private/qqmljslexer_p.h>
#include <QtQml/private/qqmljsparser_p.h>
#include <QtQml/private/qqmljsengine_p.h>
#include <QtQml/private/qqmljsastvisitor_p.h>
#include <QtQml/private/qqmljsast_p.h>

#if QT_CONFIG(commandlineparser)
#include <QCommandLineParser>
#endif

#include "commentastvisitor.h"
#include "dumpastvisitor.h"
#include "restructureastvisitor.h"

bool parseFile(const QString& filename, bool inplace, bool verbose, bool sortImports, bool force)
{
    QFile file(filename);

    if (!file.open(QIODevice::Text | QIODevice::ReadOnly)) {
        qWarning().noquote() << "Failed to open" << filename << "for reading.";
        return false;
    }

    QString code = QString::fromUtf8(file.readAll());
    file.close();

    QQmlJS::Engine engine;
    QQmlJS::Lexer lexer(&engine);

    lexer.setCode(code, 1, true);
    QQmlJS::Parser parser(&engine);

    bool success = parser.parse();

    if (!success) {
        const auto diagnosticMessages = parser.diagnosticMessages();
        for (const QQmlJS::DiagnosticMessage &m : diagnosticMessages) {
            qWarning().noquote() << QString::fromLatin1("%1:%2 : %3")
                                    .arg(filename).arg(m.line).arg(m.message);
        }

        qWarning().noquote() << "Failed to parse" << filename;
        return false;
    }

    // Try to attach comments to AST nodes
    CommentAstVisitor comment(&engine, parser.rootNode());

    if (verbose)
        qWarning().noquote() << comment.attachedComments().size() << "comment(s) attached.";

    if (verbose) {
        int orphaned = 0;

        for (const auto& orphanList : comment.orphanComments().values())
            orphaned += orphanList.size();

        qWarning().noquote() << orphaned << "comments are orphans.";
    }

    if (verbose && sortImports)
        qWarning().noquote() << "Sorting imports";

    // Do the actual restructuring
    RestructureAstVisitor restructure(parser.rootNode(), sortImports);

    // Turn AST back into source code
    if (verbose)
        qWarning().noquote() << "Dumping" << filename;

    DumpAstVisitor dump(parser.rootNode(), &comment);

    if (dump.error()) {
        if (force) {
            qWarning().noquote() << "An error has occurred. The output may not be reliable.";
        } else {
            qWarning().noquote() << "Am error has occurred. Aborting.";
            return false;
        }
   }

    if (inplace) {
        if (verbose)
            qWarning().noquote() << "Writing to file" << filename;

        if (!file.open(QIODevice::Text | QIODevice::WriteOnly))
        {
            qWarning().noquote() << "Failed to open" << filename << "for writing";
            return false;
        }

        file.write(dump.toString().toUtf8());
        file.close();
    } else {
        QTextStream(stdout) << dump.toString();
    }

    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("qmlformat");
    QCoreApplication::setApplicationVersion("1.0");

    bool success = true;
#if QT_CONFIG(commandlineparser)
    QCommandLineParser parser;
    parser.setApplicationDescription("Formats QML files according to the QML Coding Conventions.");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption(QCommandLineOption({"V", "verbose"},
                     QStringLiteral("Verbose mode. Outputs more detailed information.")));

    parser.addOption(QCommandLineOption({"n", "no-sort"},
                     QStringLiteral("Do not sort imports.")));

    parser.addOption(QCommandLineOption({"i", "inplace"},
                     QStringLiteral("Edit file in-place instead of outputting to stdout.")));

    parser.addOption(QCommandLineOption({"f", "force"},
                     QStringLiteral("Continue even if an error has occurred.")));

    parser.addPositionalArgument("filenames", "files to be processed by qmlformat");

    parser.process(app);

    const auto positionalArguments = parser.positionalArguments();

    if (positionalArguments.isEmpty())
        parser.showHelp(-1);

    for (const QString& file: parser.positionalArguments()) {
        if (!parseFile(file, parser.isSet("inplace"), parser.isSet("verbose"), !parser.isSet("no-sort"), parser.isSet("force")))
            success = false;
    }
#endif

    return success ? 0 : 1;
}
