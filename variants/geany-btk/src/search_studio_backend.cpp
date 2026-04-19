#include "search_studio_backend.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

namespace {

struct SearchPatternValidation
{
    bool invalid = false;
    QString diagnosticText;
};

QString qs(const char *text)
{
    return QString::fromUtf8(text);
}

QStringList prototypeDocuments()
{
    return {qs("src/search.c"), qs("src/document.c"), qs("src/prefs.c"), qs("data/geany.glade")};
}

int sampleCountFor(const QString &query, int index)
{
    const int base = query.trimmed().isEmpty() ? 2 : query.trimmed().size();
    return base + index + 1;
}

int sampleLineFor(const QString &query, int index)
{
    const int base = query.trimmed().isEmpty() ? 12 : query.trimmed().size() * 3;
    return base + (index * 17);
}

QString buildLinePreview(const QString &kind, const QString &file, const QString &query,
    int line, const QString &context)
{
    return QString(
        "%1\n\n"
        "File: %2\n"
        "Query: %3\n"
        "Line: %4\n\n"
        "Context:\n%5")
        .formatArgs(kind, file, query, QString::number(line), context);
}

QString targetForScope(SearchStudioTargetScope scope, const QString &target)
{
    if (! target.isEmpty())
        return target;

    switch (scope)
    {
        case SearchStudioTargetScope::SearchStudio:
            return qs("Search Studio");
        case SearchStudioTargetScope::ActiveDocument:
            return qs("Active Document");
        case SearchStudioTargetScope::OpenDocuments:
            return qs("Open Documents");
        case SearchStudioTargetScope::Directory:
            return qs("Directory Search");
        case SearchStudioTargetScope::ExplicitTarget:
            break;
    }

    return QString();
}

SearchStudioResultSpec makeResultSpec(SearchStudioActionKind actionKind,
    SearchStudioResultKind kind, SearchStudioTargetScope scope,
    const QString &action, const QString &target, const QString &query,
    const QString &mode, const QString &summary, const QString &previewTitle,
    const QString &previewBody, bool navigable)
{
    SearchStudioResultSpec row;
    row.actionKind = actionKind;
    row.kind = kind;
    row.scope = scope;
    row.action = action;
    row.target = targetForScope(scope, target);
    row.query = query;
    row.mode = mode;
    row.summary = summary;
    row.previewTitle = previewTitle;
    row.previewBody = previewBody;
    row.navigable = navigable;
    return row;
}

SearchStudioActionKind inferActionKind(const QString &action)
{
    if (action.contains(qs("Replace Preview")))
        return SearchStudioActionKind::ReplacePreview;
    if (action.contains(qs("Replace")))
        return SearchStudioActionKind::Replace;
    if (action.contains(qs("Find in Files")))
        return SearchStudioActionKind::FindInFiles;
    if (action.contains(qs("Count")))
        return SearchStudioActionKind::Count;
    if (action.contains(qs("Collect")) || action.contains(qs("Hit")))
        return SearchStudioActionKind::CollectHits;
    if (action.contains(qs("Mark")))
        return SearchStudioActionKind::Mark;
    if (action.contains(qs("Find")))
        return SearchStudioActionKind::Find;
    return SearchStudioActionKind::Studio;
}

SearchStudioTargetScope inferScope(const QString &target)
{
    if (target == qs("Active Document"))
        return SearchStudioTargetScope::ActiveDocument;
    if (target == qs("Open Documents"))
        return SearchStudioTargetScope::OpenDocuments;
    if (target == qs("Search Studio"))
        return SearchStudioTargetScope::SearchStudio;
    if (target == qs("Directory Search") || target == qs(".") || target.contains('/') || target.contains('\\'))
        return SearchStudioTargetScope::Directory;
    return SearchStudioTargetScope::ExplicitTarget;
}

SearchStudioResultSpec makeResultSpec(SearchStudioActionKind actionKind,
    const QString &action, const QString &target, const QString &query,
    const QString &mode, const QString &summary, const QString &previewTitle,
    const QString &previewBody, bool navigable)
{
    SearchStudioResultKind resultKind = SearchStudioResultKind::Summary;

    if (actionKind == SearchStudioActionKind::ReplacePreview)
        resultKind = SearchStudioResultKind::Preview;
    else if (actionKind == SearchStudioActionKind::FindInFiles)
        resultKind = SearchStudioResultKind::Capture;
    else if (navigable)
        resultKind = SearchStudioResultKind::Impact;

    return makeResultSpec(actionKind, resultKind, inferScope(target), action,
        target, query, mode, summary, previewTitle, previewBody, navigable);
}

QString impactTarget(const QString &file, int line)
{
    return QString("%1:%2").formatArg(file).formatArg(line);
}

QString documentScopeLabel(bool sessionScope)
{
    return sessionScope ? qs("Open Documents") : qs("Active Document");
}

QString directoryScopeLabel()
{
    return qs("Directory Search");
}

QString normalizedDirectoryTarget(const QString &directory)
{
    const QString trimmed = directory.trimmed();
    return trimmed.isEmpty() ? qs(".") : trimmed;
}

template <typename Run>
void applyRunProvenance(Run &run, const QString &sourceLabel,
    const QString &scopeLabel, const QString &targetLabel,
    const QString &scannedRootPath)
{
    run.sourceLabel = sourceLabel;
    run.scopeLabel = scopeLabel;
    run.targetLabel = targetLabel;
    run.scannedRootPath = scannedRootPath;
}

template <typename Run>
void inheritWorkspaceContext(Run &run, const Run &workspaceRun)
{
    if (! workspaceRun.scopeLabel.isEmpty())
        run.scopeLabel = workspaceRun.scopeLabel;
    if (! workspaceRun.targetLabel.isEmpty())
        run.targetLabel = workspaceRun.targetLabel;
    if (! workspaceRun.scannedRootPath.isEmpty())
        run.scannedRootPath = workspaceRun.scannedRootPath;
}

QString decodeExtendedText(const QString &text)
{
    QString decoded;

    for (int index = 0; index < text.size(); ++index)
    {
        const QChar current = text.at(index);
        if (current != '\\' || index + 1 >= text.size())
        {
            decoded.append(current);
            continue;
        }

        const QChar next = text.at(++index);
        if (next == 'n')
            decoded.append('\n');
        else if (next == 'r')
            decoded.append('\r');
        else if (next == 't')
            decoded.append('\t');
        else if (next == '\\')
            decoded.append('\\');
        else
        {
            decoded.append('\\');
            decoded.append(next);
        }
    }

    return decoded;
}

QString normalizedPatternText(const QString &text, const QString &mode)
{
    if (mode == qs("Extended"))
        return decodeExtendedText(text);
    return text;
}

QString normalizedReplacementText(const QString &text, const QString &mode)
{
    if (mode == qs("Extended"))
        return decodeExtendedText(text);
    return text;
}

bool modeUsesRegex(const QString &mode)
{
    return mode == qs("Regex") || mode == qs("Regular expression");
}

SearchPatternValidation validatePattern(const QString &query, const QString &mode)
{
    SearchPatternValidation validation;
    const QString patternText = normalizedPatternText(query, mode);

    if (patternText.isEmpty() || ! modeUsesRegex(mode))
        return validation;

    const QRegularExpression expression(patternText);
    if (! expression.isValid())
    {
        validation.invalid = true;
        validation.diagnosticText = QString("Invalid regular expression: %1")
            .formatArg(expression.errorString());
    }

    return validation;
}

bool looksLikeSearchableTextFile(const QFileInfo &info)
{
    const QString suffix = info.suffix().toLower();
    const QStringList allowedSuffixes = {
        qs("c"), qs("h"), qs("cpp"), qs("hpp"), qs("cc"), qs("hh"),
        qs("txt"), qs("md"), qs("rst"), qs("glade"), qs("css"), qs("conf"),
        qs("am"), qs("ac"), qs("cmake"), qs("bat"), qs("in"), qs("build")
    };

    return allowedSuffixes.contains(suffix) || info.fileName().contains('.');
}

bool looksLikeWorkspaceRoot(const QDir &directory)
{
    return QFileInfo(directory.filePath(qs("src/search.c"))).exists()
        && QFileInfo(directory.filePath(qs("data/geany.glade"))).exists();
}

QString findWorkspaceRoot()
{
    QStringList seeds;
    seeds.append(QDir::currentPath());
    seeds.append(QCoreApplication::applicationDirPath());

    for (const auto &seed : seeds)
    {
        QDir probe(seed);
        while (probe.exists())
        {
            if (looksLikeWorkspaceRoot(probe))
                return probe.absolutePath();
            if (! probe.cdUp())
                break;
        }
    }

    return QString();
}

QString relativePathForDisplay(const QString &rootPath, const QString &absolutePath)
{
    if (rootPath.isEmpty())
        return absolutePath;

    const QDir root(rootPath);
    return root.relativeFilePath(absolutePath);
}

QStringList readFileLines(const QString &path)
{
    QFile file(path);
    if (! file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};

    QTextStream stream(&file);
    QStringList lines;
    while (! stream.atEnd())
        lines.append(stream.readLine());
    return lines;
}

struct SearchLineMatch
{
    bool found = false;
    int count = 0;
    QString matchedText;
    QString replacementLine;
    QString replacementText;
};

SearchLineMatch matchLine(const QString &line, const QString &query,
    const QString &replacement, const QString &mode)
{
    SearchLineMatch result;
    const QString patternText = normalizedPatternText(query, mode);
    const QString replacementText = normalizedReplacementText(replacement, mode);

    if (patternText.isEmpty())
        return result;

    if (modeUsesRegex(mode))
    {
        const QRegularExpression expression(patternText);
        if (! expression.isValid())
            return result;

        const auto matches = expression.globalMatch(line);
        if (matches.isEmpty())
            return result;

        result.found = true;
        result.count = matches.size();
        result.matchedText = matches.first().captured();
        result.replacementLine = QString(line).replace(expression, replacementText);
        result.replacementText = replacementText;
        return result;
    }

    int position = 0;
    while (position >= 0)
    {
        position = line.indexOf(patternText, position);
        if (position < 0)
            break;

        ++result.count;
        if (! result.found)
        {
            result.found = true;
            result.matchedText = patternText;
            result.replacementLine = line;
            result.replacementLine.replace(position, patternText.size(), replacementText);
            result.replacementText = replacementText;
        }
        position += qMax(1, patternText.size());
    }

    return result;
}

SearchStudioImpactRun makeInvalidImpactRun(const QString &diagnosticText)
{
    SearchStudioImpactRun run;
    run.invalidPattern = true;
    run.diagnosticText = diagnosticText;
    return run;
}

SearchStudioReplacePreviewRun makeInvalidPreviewRun(const QString &diagnosticText)
{
    SearchStudioReplacePreviewRun run;
    run.invalidPattern = true;
    run.diagnosticText = diagnosticText;
    return run;
}

SearchStudioImpactRun scanDocumentImpacts(const QStringList &relativeFiles,
    const QString &rootPath, const QString &query, const QString &mode)
{
    const SearchPatternValidation validation = validatePattern(query, mode);
    if (validation.invalid)
        return makeInvalidImpactRun(validation.diagnosticText);

    SearchStudioImpactRun run;
    run.workspaceAttempted = true;
    run.workspaceDataUsed = true;
    run.scannedFileCount = relativeFiles.size();

    for (const auto &relativeFile : relativeFiles)
    {
        const QString absolutePath = QDir(rootPath).filePath(relativeFile);
        const QStringList lines = readFileLines(absolutePath);
        if (lines.isEmpty())
            continue;

        SearchStudioDocumentImpact impact;
        impact.file = relativeFile;

        for (int index = 0; index < lines.size(); ++index)
        {
            const SearchLineMatch match = matchLine(lines.at(index), query, QString(), mode);
            if (! match.found)
                continue;

            if (impact.line == 0)
            {
                impact.line = index + 1;
                impact.context = lines.at(index).trimmed();
                if (impact.context.isEmpty())
                    impact.context = qs("(blank line)");
            }
            impact.count += match.count;
            run.totalMatchCount += match.count;
        }

        if (impact.line != 0)
            run.impacts.append(impact);
    }

    run.matchedDocumentCount = run.impacts.size();
    return run;
}

SearchStudioReplacePreviewRun scanReplacePreviewImpacts(
    const QStringList &relativeFiles, const QString &rootPath,
    const SearchStudioReplaceRequest &request)
{
    const SearchPatternValidation validation = validatePattern(request.query, request.mode);
    if (validation.invalid)
        return makeInvalidPreviewRun(validation.diagnosticText);

    SearchStudioReplacePreviewRun run;
    run.workspaceAttempted = true;
    run.workspaceDataUsed = true;
    run.scannedFileCount = relativeFiles.size();

    for (const auto &relativeFile : relativeFiles)
    {
        const QString absolutePath = QDir(rootPath).filePath(relativeFile);
        const QStringList lines = readFileLines(absolutePath);
        if (lines.isEmpty())
            continue;

        for (int index = 0; index < lines.size(); ++index)
        {
            const QString &line = lines.at(index);
            const SearchLineMatch match = matchLine(line, request.query,
                request.replacement, request.mode);
            if (! match.found)
                continue;

            SearchStudioReplacePreviewImpact impact;
            impact.file = relativeFile;
            impact.line = index + 1;
            impact.originalLine = line;
            impact.replacementLine = match.replacementLine;
            impact.matchedText = match.matchedText;
            impact.replacementText = match.replacementText;
            run.impacts.append(impact);
            run.totalMatchCount += match.count;
            break;
        }
    }

    run.matchedDocumentCount = run.impacts.size();
    return run;
}

QStringList collectDirectoryFiles(const QString &rootPath, const QString &directory)
{
    QString resolvedPath = directory.trimmed();
    if (resolvedPath.isEmpty() || resolvedPath == qs("."))
        resolvedPath = rootPath;
    else if (! QFileInfo(resolvedPath).isAbsolute())
        resolvedPath = QDir(rootPath).filePath(resolvedPath);

    QFileInfo info(resolvedPath);
    if (! info.exists())
        return {};

    QStringList files;
    if (info.isFile())
    {
        if (looksLikeSearchableTextFile(info))
            files.append(info.absoluteFilePath());
        return files;
    }

    QDirIterator iterator(resolvedPath, QDir::Files, QDirIterator::Subdirectories);
    while (iterator.hasNext())
    {
        iterator.next();
        const QFileInfo fileInfo = iterator.fileInfo();
        if (! looksLikeSearchableTextFile(fileInfo))
            continue;
        files.append(fileInfo.absoluteFilePath());
    }

    return files;
}

SearchStudioImpactRun scanDirectoryImpacts(const QString &rootPath,
    const SearchStudioFindInFilesRequest &request)
{
    const SearchPatternValidation validation = validatePattern(request.query, request.mode);
    if (validation.invalid)
        return makeInvalidImpactRun(validation.diagnosticText);

    SearchStudioImpactRun run;
    run.workspaceAttempted = true;
    run.workspaceDataUsed = true;

    const QStringList files = collectDirectoryFiles(rootPath, request.directory);
    run.scannedFileCount = files.size();

    for (const auto &absolutePath : files)
    {
        const QStringList lines = readFileLines(absolutePath);
        if (lines.isEmpty())
            continue;

        SearchStudioDocumentImpact impact;
        impact.file = relativePathForDisplay(rootPath, absolutePath);

        for (int index = 0; index < lines.size(); ++index)
        {
            const SearchLineMatch match = matchLine(lines.at(index), request.query,
                QString(), request.mode);
            if (! match.found)
                continue;

            if (impact.line == 0)
            {
                impact.line = index + 1;
                impact.context = lines.at(index).trimmed();
                if (impact.context.isEmpty())
                    impact.context = qs("(blank line)");
            }
            impact.count += match.count;
            run.totalMatchCount += match.count;
        }

        if (impact.line != 0)
            run.impacts.append(impact);
    }

    run.matchedDocumentCount = run.impacts.size();
    if (run.scannedFileCount == 0)
        run.diagnosticText = QString("No searchable files found under %1.")
            .formatArg(request.directory);
    return run;
}

void appendImpactRows(SearchStudioActionResult &result,
    SearchStudioActionKind actionKind, const QString &action,
    const QString &query, const QString &mode,
    const QList<SearchStudioDocumentImpact> &impacts, const QString &summaryPrefix)
{
    for (const auto &impact : impacts)
    {
        result.rows.append(makeResultSpec(actionKind,
            action,
            impactTarget(impact.file, impact.line),
            query,
            mode,
            QString("%1 %2 matches in this document.").formatArg(summaryPrefix).formatArg(impact.count),
            QString("%1 — %2:%3").formatArgs(action, impact.file).formatArg(impact.line),
            buildLinePreview(QString("%1 impact summary").formatArg(action),
                impact.file, query, impact.line, impact.context),
            true));
    }
}

void appendReplacePreviewRows(SearchStudioActionResult &result,
    SearchStudioActionKind actionKind, const QString &action,
    const QString &query, const QString &mode,
    const QList<SearchStudioReplacePreviewImpact> &impacts)
{
    for (const auto &impact : impacts)
    {
        const QString previewBody = QString(
            "Original line:\n- %1\n\n"
            "Replacement line:\n+ %2\n\n"
            "Matched segment diff:\n- %3\n+ %4\n\n"
            "Payload entered:\n%5\n\n"
            "Actual replacement text:\n%6")
            .formatArgs(impact.originalLine, impact.replacementLine,
                impact.matchedText, impact.replacementText,
                impact.replacementText, impact.replacementText);

        result.rows.append(makeResultSpec(actionKind,
            action,
            impactTarget(impact.file, impact.line),
            query,
            mode,
            QString("Would replace the representative match on line %1.").formatArg(impact.line),
            QString("%1 — %2:%3").formatArgs(action, impact.file).formatArg(impact.line),
            previewBody,
            true));
    }
}

template <typename Run>
QString normalizedRunSummary(const QString &defaultSummary, const Run &run,
    const QString &zeroHitSummary)
{
    if (run.invalidPattern)
        return qs("Search request is invalid; no result rows were generated.");
    if (run.prototypeFallbackUsed)
        return QString("%1 Showing prototype fallback rows after missing live checkout hits.")
            .formatArg(defaultSummary);
    if (run.workspaceDataUsed)
    {
        if (run.totalMatchCount == 0)
            return zeroHitSummary;
        return QString("%1 Matched %2 hits across %3 documents.")
            .formatArgs(defaultSummary, QString::number(run.totalMatchCount),
                QString::number(run.matchedDocumentCount));
    }
    return defaultSummary;
}

template <typename Run>
QString runProvenanceBody(const Run &run)
{
    QStringList lines;

    if (! run.sourceLabel.isEmpty())
        lines.append(QString("Source: %1").formatArg(run.sourceLabel));
    if (! run.scopeLabel.isEmpty())
        lines.append(QString("Scope: %1").formatArg(run.scopeLabel));
    if (! run.targetLabel.isEmpty())
        lines.append(QString("Target: %1").formatArg(run.targetLabel));
    if (! run.scannedRootPath.isEmpty())
        lines.append(QString("Scanned root: %1").formatArg(run.scannedRootPath));

    lines.append(QString("Files scanned: %1").formatArg(run.scannedFileCount));
    lines.append(QString("Matched documents: %1").formatArg(run.matchedDocumentCount));
    lines.append(QString("Total matches: %1").formatArg(run.totalMatchCount));
    lines.append(QString("Workspace attempted: %1").formatArg(run.workspaceAttempted ? qs("yes") : qs("no")));
    lines.append(QString("Workspace data used: %1").formatArg(run.workspaceDataUsed ? qs("yes") : qs("no")));
    lines.append(QString("Prototype fallback used: %1").formatArg(run.prototypeFallbackUsed ? qs("yes") : qs("no")));

    return lines.join(qs("\n"));
}

template <typename Run>
QString normalizedRunPreviewBody(const QString &defaultBody, const Run &run)
{
    const QString provenanceBody = runProvenanceBody(run);
    QStringList sections;

    if (! defaultBody.isEmpty())
        sections.append(defaultBody);
    if (! run.diagnosticText.isEmpty())
        sections.append(QString("Status:\n%1").formatArg(run.diagnosticText));
    if (! provenanceBody.isEmpty())
        sections.append(QString("Provenance:\n%1").formatArg(provenanceBody));

    return sections.join(qs("\n\n"));
}

template <typename Run>
void appendRunDiagnostics(SearchStudioActionResult &result,
    SearchStudioActionKind actionKind, SearchStudioTargetScope summaryScope,
    const QString &query, const QString &mode, const QString &statusAction,
    const Run &run)
{
    if (run.diagnosticText.isEmpty())
        return;

    result.activity.append(run.diagnosticText);
    result.rows.append(makeResultSpec(actionKind,
        SearchStudioResultKind::Summary,
        summaryScope,
        statusAction,
        QString(),
        query,
        mode,
        run.diagnosticText,
        QString("%1 Status").formatArg(statusAction),
        normalizedRunPreviewBody(run.diagnosticText, run),
        false));
}

template <typename Run>
void appendRunProvenance(SearchStudioActionResult &result,
    SearchStudioActionKind actionKind, SearchStudioTargetScope summaryScope,
    const QString &query, const QString &mode, const QString &statusAction,
    const Run &run)
{
    const QString provenanceBody = runProvenanceBody(run);
    if (provenanceBody.isEmpty())
        return;

    const QString summary = QString("Source=%1 | Scope=%2 | Files=%3 | Matches=%4")
        .formatArgs(run.sourceLabel.isEmpty() ? qs("unknown") : run.sourceLabel,
            run.scopeLabel.isEmpty() ? qs("unknown") : run.scopeLabel,
            QString::number(run.scannedFileCount),
            QString::number(run.totalMatchCount));

    result.rows.append(makeResultSpec(actionKind,
        SearchStudioResultKind::Summary,
        summaryScope,
        QString("%1 Provenance").formatArg(statusAction),
        run.targetLabel,
        query,
        mode,
        summary,
        QString("%1 Provenance").formatArg(statusAction),
        provenanceBody,
        false));
}

QString statusActionLabel(const QString &preferred, const QString &fallback)
{
    if (! preferred.isEmpty())
        return preferred;
    if (! fallback.isEmpty())
        return fallback;
    return qs("Search Status");
}

class PrototypeSearchStudioSearchService final : public SearchStudioSearchService
{
public:
    SearchStudioImpactRun buildFindImpactRows(
        const SearchStudioFindRequest &request) const override
    {
        SearchStudioImpactRun run = buildDocumentImpacts(request.query, request.mode,
            request.sessionScope, qs("representative find"));
        applyRunProvenance(run, qs("Prototype Fallback"),
            documentScopeLabel(request.sessionScope),
            request.sessionScope ? qs("Prototype document set") : prototypeDocuments().value(0),
            QString());
        return run;
    }

    SearchStudioImpactRun buildMarkImpactRows(
        const SearchStudioMarkRequest &request) const override
    {
        SearchStudioImpactRun run = buildDocumentImpacts(request.query, request.mode,
            request.sessionScope, qs("representative mark"));
        applyRunProvenance(run, qs("Prototype Fallback"),
            documentScopeLabel(request.sessionScope),
            request.sessionScope ? qs("Prototype document set") : prototypeDocuments().value(0),
            QString());
        return run;
    }

    SearchStudioImpactRun buildFindInFilesCaptureRows(
        const SearchStudioFindInFilesRequest &request) const override
    {
        SearchStudioImpactRun run = buildDocumentImpacts(request.query, request.mode,
            true, qs("representative directory-search"));
        applyRunProvenance(run, qs("Prototype Fallback"),
            directoryScopeLabel(), normalizedDirectoryTarget(request.directory),
            QString());
        return run;
    }

    SearchStudioReplacePreviewRun buildReplacePreviewRows(
        const SearchStudioReplaceRequest &request) const override
    {
        const SearchPatternValidation validation = validatePattern(request.query, request.mode);
        if (validation.invalid)
        {
            auto run = makeInvalidPreviewRun(validation.diagnosticText);
            run.prototypeFallbackUsed = true;
            applyRunProvenance(run, qs("Prototype Fallback"),
                documentScopeLabel(request.sessionScope),
                request.sessionScope ? qs("Prototype document set") : prototypeDocuments().value(0),
                QString());
            return run;
        }

        SearchStudioReplacePreviewRun run;
        const auto docs = prototypeDocuments();
        const int limit = request.sessionScope ? docs.size() : 1;

        run.prototypeFallbackUsed = true;
        run.scannedFileCount = limit;

        for (int index = 0; index < limit; ++index)
        {
            const QString &file = docs.at(index);
            const int line = sampleLineFor(request.query, index);
            SearchStudioReplacePreviewImpact impact;

            impact.file = file;
            impact.line = line;
            impact.originalLine = QString("const QString needle = \"%1\"; // line %2")
                .formatArg(request.query)
                .formatArg(line);
            impact.replacementLine = QString("const QString needle = \"%1\"; // line %2")
                .formatArg(request.replacement)
                .formatArg(line);
            impact.matchedText = request.query;
            impact.replacementText = normalizedReplacementText(request.replacement, request.mode);
            run.impacts.append(impact);
            run.totalMatchCount += sampleCountFor(request.query, index);
        }

        run.matchedDocumentCount = run.impacts.size();
        applyRunProvenance(run, qs("Prototype Fallback"),
            documentScopeLabel(request.sessionScope),
            request.sessionScope ? qs("Prototype document set") : prototypeDocuments().value(0),
            QString());
        return run;
    }

private:
    SearchStudioImpactRun buildDocumentImpacts(const QString &query,
        const QString &mode, bool sessionScope, const QString &contextLabel) const
    {
        const SearchPatternValidation validation = validatePattern(query, mode);
        if (validation.invalid)
        {
            auto run = makeInvalidImpactRun(validation.diagnosticText);
            run.prototypeFallbackUsed = true;
            return run;
        }

        SearchStudioImpactRun run;
        const auto docs = prototypeDocuments();
        const int limit = sessionScope ? docs.size() : 1;

        run.prototypeFallbackUsed = true;
        run.scannedFileCount = limit;

        for (int index = 0; index < limit; ++index)
        {
            const QString &file = docs.at(index);
            SearchStudioDocumentImpact impact;

            impact.file = file;
            impact.line = sampleLineFor(query, index);
            impact.count = sampleCountFor(query, index);
            impact.context = QString("line %1 contains a %2 hit for \"%3\".")
                .formatArg(impact.line)
                .formatArg(contextLabel)
                .formatArg(query);
            run.totalMatchCount += impact.count;
            run.impacts.append(impact);
        }

        run.matchedDocumentCount = run.impacts.size();
        return run;
    }
};

class WorkspaceSearchStudioSearchService final : public SearchStudioSearchService
{
public:
    SearchStudioImpactRun buildFindImpactRows(
        const SearchStudioFindRequest &request) const override
    {
        const QString rootPath = findWorkspaceRoot();
        if (rootPath.isEmpty())
            return {};

        QStringList files = prototypeDocuments();
        if (! request.sessionScope && ! files.isEmpty())
        {
            const QString activeFile = files.first();
            files.clear();
            files.append(activeFile);
        }

        SearchStudioImpactRun run = scanDocumentImpacts(files, rootPath,
            request.query, request.mode);
        applyRunProvenance(run, qs("Workspace Checkout"),
            documentScopeLabel(request.sessionScope),
            request.sessionScope ? qs("Tracked checkout set") : files.value(0),
            rootPath);
        return run;
    }

    SearchStudioImpactRun buildMarkImpactRows(
        const SearchStudioMarkRequest &request) const override
    {
        const QString rootPath = findWorkspaceRoot();
        if (rootPath.isEmpty())
            return {};

        QStringList files = prototypeDocuments();
        if (! request.sessionScope && ! files.isEmpty())
        {
            const QString activeFile = files.first();
            files.clear();
            files.append(activeFile);
        }

        SearchStudioImpactRun run = scanDocumentImpacts(files, rootPath,
            request.query, request.mode);
        applyRunProvenance(run, qs("Workspace Checkout"),
            documentScopeLabel(request.sessionScope),
            request.sessionScope ? qs("Tracked checkout set") : files.value(0),
            rootPath);
        return run;
    }

    SearchStudioImpactRun buildFindInFilesCaptureRows(
        const SearchStudioFindInFilesRequest &request) const override
    {
        const QString rootPath = findWorkspaceRoot();
        if (rootPath.isEmpty())
            return {};

        SearchStudioImpactRun run = scanDirectoryImpacts(rootPath, request);
        applyRunProvenance(run, qs("Workspace Checkout"),
            directoryScopeLabel(), normalizedDirectoryTarget(request.directory),
            rootPath);
        return run;
    }

    SearchStudioReplacePreviewRun buildReplacePreviewRows(
        const SearchStudioReplaceRequest &request) const override
    {
        const QString rootPath = findWorkspaceRoot();
        if (rootPath.isEmpty())
            return {};

        QStringList files = prototypeDocuments();
        if (! request.sessionScope && ! files.isEmpty())
        {
            const QString activeFile = files.first();
            files.clear();
            files.append(activeFile);
        }

        SearchStudioReplacePreviewRun run = scanReplacePreviewImpacts(files,
            rootPath, request);
        applyRunProvenance(run, qs("Workspace Checkout"),
            documentScopeLabel(request.sessionScope),
            request.sessionScope ? qs("Tracked checkout set") : files.value(0),
            rootPath);
        return run;
    }
};

class HybridSearchStudioSearchService final : public SearchStudioSearchService
{
public:
    SearchStudioImpactRun buildFindImpactRows(
        const SearchStudioFindRequest &request) const override
    {
        return preferWorkspaceImpact(workspace_.buildFindImpactRows(request),
            prototype_.buildFindImpactRows(request));
    }

    SearchStudioImpactRun buildMarkImpactRows(
        const SearchStudioMarkRequest &request) const override
    {
        return preferWorkspaceImpact(workspace_.buildMarkImpactRows(request),
            prototype_.buildMarkImpactRows(request));
    }

    SearchStudioImpactRun buildFindInFilesCaptureRows(
        const SearchStudioFindInFilesRequest &request) const override
    {
        return preferWorkspaceImpact(workspace_.buildFindInFilesCaptureRows(request),
            prototype_.buildFindInFilesCaptureRows(request));
    }

    SearchStudioReplacePreviewRun buildReplacePreviewRows(
        const SearchStudioReplaceRequest &request) const override
    {
        return preferWorkspacePreview(workspace_.buildReplacePreviewRows(request),
            prototype_.buildReplacePreviewRows(request));
    }

private:
    SearchStudioImpactRun preferWorkspaceImpact(const SearchStudioImpactRun &workspaceRun,
        const SearchStudioImpactRun &prototypeRun) const
    {
        if (workspaceRun.invalidPattern)
            return workspaceRun;
        if (! workspaceRun.impacts.isEmpty())
            return workspaceRun;
        if (! workspaceRun.workspaceAttempted)
        {
            SearchStudioImpactRun run = prototypeRun;
            run.diagnosticText = qs("Workspace checkout not detected; showing prototype fallback rows.");
            run.prototypeFallbackUsed = true;
            return run;
        }
        if (workspaceRun.scannedFileCount == 0)
            return workspaceRun;

        SearchStudioImpactRun run = prototypeRun;
        run.workspaceAttempted = true;
        run.prototypeFallbackUsed = true;
        inheritWorkspaceContext(run, workspaceRun);
        run.diagnosticText = QString("No live checkout hits were found across %1 scanned files; showing prototype fallback rows.")
            .formatArg(workspaceRun.scannedFileCount);
        return run;
    }

    SearchStudioReplacePreviewRun preferWorkspacePreview(
        const SearchStudioReplacePreviewRun &workspaceRun,
        const SearchStudioReplacePreviewRun &prototypeRun) const
    {
        if (workspaceRun.invalidPattern)
            return workspaceRun;
        if (! workspaceRun.impacts.isEmpty())
            return workspaceRun;
        if (! workspaceRun.workspaceAttempted)
        {
            SearchStudioReplacePreviewRun run = prototypeRun;
            run.diagnosticText = qs("Workspace checkout not detected; showing prototype fallback preview rows.");
            run.prototypeFallbackUsed = true;
            return run;
        }
        if (workspaceRun.scannedFileCount == 0)
            return workspaceRun;

        SearchStudioReplacePreviewRun run = prototypeRun;
        run.workspaceAttempted = true;
        run.prototypeFallbackUsed = true;
        inheritWorkspaceContext(run, workspaceRun);
        run.diagnosticText = QString("No live checkout replace-preview hits were found across %1 scanned files; showing prototype fallback preview rows.")
            .formatArg(workspaceRun.scannedFileCount);
        return run;
    }

    WorkspaceSearchStudioSearchService workspace_;
    PrototypeSearchStudioSearchService prototype_;
};

} // namespace

namespace SearchStudioBackend {

const SearchStudioSearchService &defaultSearchService()
{
    static const HybridSearchStudioSearchService service;
    return service;
}

SearchStudioActionResult executeFindAction(const SearchStudioFindRequest &request,
    const SearchStudioFindActionSpec &action)
{
    return executeFindAction(request, action, defaultSearchService());
}

SearchStudioActionResult executeFindAction(const SearchStudioFindRequest &request,
    const SearchStudioFindActionSpec &action,
    const SearchStudioSearchService &service)
{
    SearchStudioActionResult result;
    const SearchStudioImpactRun run = service.buildFindImpactRows(request);

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    appendRunDiagnostics(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.impactAction), run);
    appendRunProvenance(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.impactAction), run);
    if (action.includeImpactRows)
        appendImpactRows(result, action.actionKind, action.impactAction,
            request.query, request.mode, run.impacts, action.impactSummaryPrefix);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, QString(), request.query,
            request.mode,
            normalizedRunSummary(action.summaryText, run,
                qs("No live matches were found for this request.")),
            action.summaryPreviewTitle,
            normalizedRunPreviewBody(action.summaryPreviewBody, run),
            false));

    return result;
}

SearchStudioActionResult executeReplaceAction(const SearchStudioReplaceRequest &request,
    const SearchStudioReplaceActionSpec &action)
{
    return executeReplaceAction(request, action, defaultSearchService());
}

SearchStudioActionResult executeReplaceAction(const SearchStudioReplaceRequest &request,
    const SearchStudioReplaceActionSpec &action,
    const SearchStudioSearchService &service)
{
    SearchStudioActionResult result;
    const SearchStudioReplacePreviewRun run = service.buildReplacePreviewRows(request);

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    appendRunDiagnostics(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.rowAction), run);
    appendRunProvenance(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.rowAction), run);
    if (action.previewRows)
        appendReplacePreviewRows(result, action.actionKind, action.rowAction,
            request.query, request.mode, run.impacts);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, QString(), request.query,
            request.mode,
            normalizedRunSummary(action.summaryText, run,
                qs("No live replace-preview candidates were found for this request.")),
            action.summaryPreviewTitle,
            normalizedRunPreviewBody(action.summaryPreviewBody, run),
            false));

    return result;
}

SearchStudioActionResult executeMarkAction(const SearchStudioMarkRequest &request,
    const SearchStudioMarkActionSpec &action)
{
    return executeMarkAction(request, action, defaultSearchService());
}

SearchStudioActionResult executeMarkAction(const SearchStudioMarkRequest &request,
    const SearchStudioMarkActionSpec &action,
    const SearchStudioSearchService &service)
{
    SearchStudioActionResult result;
    const SearchStudioImpactRun run = service.buildMarkImpactRows(request);

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    appendRunDiagnostics(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.impactAction), run);
    appendRunProvenance(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.impactAction), run);
    if (action.includeImpactRows)
        appendImpactRows(result, action.actionKind, action.impactAction,
            request.query, request.mode, run.impacts, action.impactSummaryPrefix);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, QString(), request.query,
            request.mode,
            normalizedRunSummary(action.summaryText, run,
                qs("No live mark candidates were found for this request.")),
            action.summaryPreviewTitle,
            normalizedRunPreviewBody(action.summaryPreviewBody, run),
            false));

    return result;
}

SearchStudioActionResult executeFindInFilesAction(
    const SearchStudioFindInFilesRequest &request,
    const SearchStudioFindInFilesActionSpec &action)
{
    return executeFindInFilesAction(request, action, defaultSearchService());
}

SearchStudioActionResult executeFindInFilesAction(
    const SearchStudioFindInFilesRequest &request,
    const SearchStudioFindInFilesActionSpec &action,
    const SearchStudioSearchService &service)
{
    SearchStudioActionResult result;
    const SearchStudioImpactRun run = service.buildFindInFilesCaptureRows(request);

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    appendRunDiagnostics(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.captureAction), run);
    appendRunProvenance(result, action.actionKind, action.summaryScope,
        request.query, request.mode,
        statusActionLabel(action.summaryAction, action.captureAction), run);
    if (action.includeCaptureRows)
        appendImpactRows(result, action.actionKind, action.captureAction,
            request.query, request.mode, run.impacts, action.captureSummaryPrefix);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, request.directory,
            request.query, request.mode,
            normalizedRunSummary(action.summaryText, run,
                qs("No live directory-search hits were found for this request.")),
            action.summaryPreviewTitle,
            normalizedRunPreviewBody(action.summaryPreviewBody, run),
            false));

    return result;
}

SearchStudioActionResult makeActiveDocumentSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody)
{
    SearchStudioActionResult result;
    result.activity.append(activityMessage);
    result.rows.append(makeResultSpec(inferActionKind(action),
        SearchStudioResultKind::Summary,
        SearchStudioTargetScope::ActiveDocument, action, QString(), query, mode,
        summary, previewTitle, previewBody, false));
    return result;
}

SearchStudioActionResult makeSessionSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody)
{
    SearchStudioActionResult result;
    result.activity.append(activityMessage);
    result.rows.append(makeResultSpec(inferActionKind(action),
        SearchStudioResultKind::Summary,
        SearchStudioTargetScope::OpenDocuments, action, QString(), query, mode,
        summary, previewTitle, previewBody, false));
    return result;
}

SearchStudioActionResult makeCountResult(const SearchStudioFindRequest &request)
{
    SearchStudioFindActionSpec action;

    action.actionKind = SearchStudioActionKind::Count;
    action.activityMessage = request.sessionScope ?
        QString("[Count] Session | query=%1 | mode=%2").formatArgs(request.query, request.mode) :
        QString("[Count] query=%1 | mode=%2 | scope=active document").formatArgs(request.query, request.mode);
    action.impactAction = request.sessionScope ? qs("Session Count Impact") : qs("Count Impact");
    action.impactSummaryPrefix = qs("Counted");
    action.summaryAction = request.sessionScope ? qs("Count in Session") : qs("Count");
    action.summaryText = request.sessionScope ?
        qs("Counted representative matches across open documents.") :
        qs("Counted matches in the active document.");
    action.summaryPreviewTitle = request.sessionScope ? qs("Count in Session") : qs("Count — Active Document");
    action.summaryPreviewBody = request.sessionScope ?
        qs("Checkout-backed count rows are used when available, with prototype fallback preserving the BTK workflow shape.") :
        qs("Checkout-backed count rows are used when available for the active document, with prototype fallback otherwise.");
    action.summaryScope = request.sessionScope ?
        SearchStudioTargetScope::OpenDocuments : SearchStudioTargetScope::ActiveDocument;
    action.includeImpactRows = true;
    return executeFindAction(request, action);
}

SearchStudioActionResult makeCollectedHitsResult(const SearchStudioFindRequest &request)
{
    SearchStudioFindActionSpec action;

    action.actionKind = SearchStudioActionKind::CollectHits;
    action.activityMessage = request.sessionScope ?
        QString("[Results] Collected open-document hits for %1.").formatArg(request.query) :
        QString("[Results] Collected current-document hits for %1.").formatArg(request.query);
    action.impactAction = request.sessionScope ? qs("Session Hit") : qs("Document Hit");
    action.impactSummaryPrefix = qs("Collected");
    action.summaryAction = request.sessionScope ? qs("Collect Session Hits") : QString();
    action.summaryText = qs("Collected representative open-document hits.");
    action.summaryPreviewTitle = qs("Collect Session Hits");
    action.summaryPreviewBody = qs("Open-document hit collection now prefers live checkout data and falls back to prototype rows only when needed.");
    action.summaryScope = SearchStudioTargetScope::OpenDocuments;
    action.includeImpactRows = true;
    return executeFindAction(request, action);
}

SearchStudioActionResult makeReplacePreviewResult(const SearchStudioReplaceRequest &request,
    const QString &action)
{
    SearchStudioReplaceActionSpec spec;

    spec.actionKind = SearchStudioActionKind::ReplacePreview;
    spec.activityMessage = request.sessionScope ?
        qs("[Replace Preview] Session preview generated from checkout-backed rows when available.") :
        qs("[Replace Preview] Document preview generated from checkout-backed rows when available.");
    spec.rowAction = action;
    spec.previewRows = true;
    return executeReplaceAction(request, spec);
}

SearchStudioActionResult makeReplaceImpactResult(const SearchStudioReplaceRequest &request,
    const QString &action, const QString &summaryActionLabel,
    const QString &summaryTarget)
{
    SearchStudioReplaceActionSpec spec;

    spec.actionKind = SearchStudioActionKind::Replace;
    spec.activityMessage = request.sessionScope ?
        QString("[Replace] Replace in session | query=%1 | replacement=%2 | mode=%3")
            .formatArgs(request.query, request.replacement, request.mode) :
        QString("[Replace] Replace in document | query=%1 | replacement=%2 | mode=%3")
            .formatArgs(request.query, request.replacement, request.mode);
    spec.rowAction = action;
    spec.summaryAction = summaryActionLabel;
    spec.summaryText = request.sessionScope ?
        qs("Exploratory replace-in-session summary with per-document impact rows above.") :
        qs("Exploratory replace-in-document summary with impact rows above.");
    spec.summaryPreviewTitle = request.sessionScope ? qs("Replace in Session") : qs("Replace in Document");
    spec.summaryPreviewBody = request.sessionScope ?
        QString("Replacement payload: %1\nMode: %2\nTarget: %3\n\nPer-document impact rows above now prefer live checkout data, with future Geany-core services replacing the fallback path later.")
            .formatArgs(request.replacement, request.mode, summaryTarget) :
        QString("Replacement payload: %1\nMode: %2\nTarget: %3\n\nImpact rows above now prefer live checkout data while mirroring the current Search Studio model.")
            .formatArgs(request.replacement, request.mode, summaryTarget);
    spec.summaryScope = request.sessionScope ?
        SearchStudioTargetScope::OpenDocuments : SearchStudioTargetScope::ActiveDocument;
    spec.previewRows = true;
    return executeReplaceAction(request, spec);
}

SearchStudioActionResult makeFindInFilesResult(const SearchStudioFindInFilesRequest &request)
{
    SearchStudioFindInFilesActionSpec action;

    action.actionKind = SearchStudioActionKind::FindInFiles;
    action.activityMessage = QString("[Find in Files] query=%1 | directory=%2 | mode=%3")
        .formatArgs(request.query, request.directory, request.mode);
    action.captureAction = qs("Find in Files Hit");
    action.captureSummaryPrefix = qs("Captured");
    action.summaryAction = qs("Find in Files");
    action.summaryText = qs("Exploratory directory search launched with structured hit ingestion.");
    action.summaryPreviewTitle = qs("Find in Files");
    action.summaryPreviewBody = QString("Directory: %1\nMode: %2\n\nResults above now prefer live checkout scanning while still mirroring a future structured ripgrep-style ingestion path.")
        .formatArgs(request.directory, request.mode);
    action.summaryScope = SearchStudioTargetScope::Directory;
    action.includeCaptureRows = true;
    return executeFindInFilesAction(request, action);
}

SearchStudioActionResult makeMarkResult(const SearchStudioMarkRequest &request,
    const QString &action, const QString &summaryActionLabel,
    const QString &summaryTarget)
{
    SearchStudioMarkActionSpec spec;
    const QString bookmark = request.bookmarkLines ? qs("on") : qs("off");
    const QString purge = request.purgeBookmarks ? qs("on") : qs("off");

    spec.actionKind = SearchStudioActionKind::Mark;
    spec.activityMessage = request.sessionScope ?
        QString("[Mark] Session | query=%1 | mode=%2")
            .formatArgs(request.query, request.mode) :
        QString("[Mark] query=%1 | mode=%2 | bookmarks=%3 | purge=%4")
            .formatArgs(request.query, request.mode, bookmark, purge);
    spec.impactAction = action;
    spec.impactSummaryPrefix = qs("Marked");
    spec.summaryAction = summaryActionLabel;
    spec.summaryText = request.sessionScope ?
        QString("Exploratory session mark summary for %1 with per-document impact rows above.")
            .formatArg(summaryTarget) :
        QString("Marked representative matches in %1; bookmark-lines=%2; purge-first=%3.")
            .formatArgs(summaryTarget, bookmark, purge);
    spec.summaryPreviewTitle = request.sessionScope ? qs("Mark in Session") : qs("Mark");
    spec.summaryPreviewBody = request.sessionScope ?
        qs("Session mark impact rows above mirror the matured Search Studio behavior from the main tree while preferring live checkout data.") :
        QString("Exploratory active-document mark summary for %1 using checkout-backed rows when available.")
            .formatArg(summaryTarget);
    spec.summaryScope = request.sessionScope ?
        SearchStudioTargetScope::OpenDocuments : SearchStudioTargetScope::ActiveDocument;
    spec.includeImpactRows = true;
    return executeMarkAction(request, spec);
}

} // namespace SearchStudioBackend
