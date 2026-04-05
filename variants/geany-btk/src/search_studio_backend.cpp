#include "search_studio_backend.h"

namespace {

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

SearchStudioResultRow makeRow(const QString &action, const QString &target,
    const QString &query, const QString &mode, const QString &summary,
    const QString &previewTitle, const QString &previewBody, bool navigable)
{
    SearchStudioResultRow row;
    row.action = action;
    row.target = target;
    row.query = query;
    row.mode = mode;
    row.summary = summary;
    row.previewTitle = previewTitle;
    row.previewBody = previewBody;
    row.navigable = navigable;
    return row;
}

void appendImpactRows(SearchStudioActionResult &result, const QString &action,
    const QString &query, const QString &mode, const QString &summaryPrefix,
    bool sessionScope)
{
    const auto docs = prototypeDocuments();
    const int limit = sessionScope ? docs.size() : 1;

    for (int index = 0; index < limit; ++index)
    {
        const QString &file = docs.at(index);
        const int line = sampleLineFor(query, index);
        const int count = sampleCountFor(query, index);
        const QString context = QString("line %1 contains a representative %2 hit for \"%3\".")
            .formatArg(line)
            .formatArg(action.toLower())
            .formatArg(query);

        result.rows.append(makeRow(
            action,
            QString("%1:%2").formatArg(file).formatArg(line),
            query,
            mode,
            QString("%1 %2 matches in this document.").formatArg(summaryPrefix).formatArg(count),
            QString("%1 — %2:%3").formatArgs(action, file).formatArg(line),
            buildLinePreview(QString("%1 impact summary").formatArg(action), file, query, line, context),
            true));
    }
}

void appendReplacePreviewRows(SearchStudioActionResult &result, const QString &action,
    const QString &query, const QString &replaceText, const QString &mode,
    bool sessionScope)
{
    const auto docs = prototypeDocuments();
    const int limit = sessionScope ? docs.size() : 1;

    for (int index = 0; index < limit; ++index)
    {
        const QString &file = docs.at(index);
        const int line = sampleLineFor(query, index);
        const QString originalLine = QString("const QString needle = \"%1\"; // line %2")
            .formatArg(query)
            .formatArg(line);
        const QString replacementLine = QString("const QString needle = \"%1\"; // line %2")
            .formatArg(replaceText)
            .formatArg(line);
        const QString previewBody = QString(
            "Original line:\n- %1\n\n"
            "Replacement line:\n+ %2\n\n"
            "Matched segment diff:\n- %3\n+ %4\n\n"
            "Payload entered:\n%5\n\n"
            "Actual replacement text:\n%6")
            .formatArgs(originalLine, replacementLine, query, replaceText, replaceText, replaceText);

        result.rows.append(makeRow(
            action,
            QString("%1:%2").formatArg(file).formatArg(line),
            query,
            mode,
            QString("Would replace the representative match on line %1.").formatArg(line),
            QString("%1 — %2:%3").formatArgs(action, file).formatArg(line),
            previewBody,
            true));
    }
}

} // namespace

namespace SearchStudioBackend {

SearchStudioActionResult makeActiveDocumentSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody)
{
    SearchStudioActionResult result;
    result.activity.append(activityMessage);
    result.rows.append(makeRow(action, qs("Active Document"), query, mode,
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
    result.rows.append(makeRow(action, qs("Open Documents"), query, mode,
        summary, previewTitle, previewBody, false));
    return result;
}

SearchStudioActionResult makeCountResult(const SearchStudioFindRequest &request)
{
    SearchStudioActionResult result;

    if (request.sessionScope)
    {
        result.activity.append(QString("[Count] Session | query=%1 | mode=%2")
            .formatArgs(request.query, request.mode));
        appendImpactRows(result, qs("Session Count Impact"), request.query,
            request.mode, qs("Counted"), true);
        result.rows.append(makeRow(qs("Count in Session"), qs("Open Documents"),
            request.query, request.mode,
            qs("Counted representative matches across open documents."),
            qs("Count in Session"),
            qs("Prototype aggregate count across open documents with per-document impact rows above."),
            false));
    }
    else
    {
        result.activity.append(QString("[Count] query=%1 | mode=%2 | scope=active document")
            .formatArgs(request.query, request.mode));
        result.rows.append(makeRow(qs("Count"), qs("Active Document"),
            request.query, request.mode,
            qs("Counted matches in the active document."),
            qs("Count — Active Document"),
            qs("Prototype count summary for the active document."),
            false));
        appendImpactRows(result, qs("Count Impact"), request.query,
            request.mode, qs("Counted"), false);
    }

    return result;
}

SearchStudioActionResult makeCollectedHitsResult(const SearchStudioFindRequest &request)
{
    SearchStudioActionResult result;

    if (request.sessionScope)
    {
        result.activity.append(QString("[Results] Collected open-document hits for %1.")
            .formatArg(request.query));
        appendImpactRows(result, qs("Session Hit"), request.query,
            request.mode, qs("Collected"), true);
        result.rows.append(makeRow(qs("Collect Session Hits"), qs("Open Documents"),
            request.query, request.mode,
            qs("Collected representative open-document hits."),
            qs("Collect Session Hits"),
            qs("Prototype open-document hit collection summary."),
            false));
    }
    else
    {
        result.activity.append(QString("[Results] Collected current-document hits for %1.")
            .formatArg(request.query));
        appendImpactRows(result, qs("Document Hit"), request.query,
            request.mode, qs("Collected"), false);
    }

    return result;
}

SearchStudioActionResult makeReplacePreviewResult(const SearchStudioReplaceRequest &request,
    const QString &action)
{
    SearchStudioActionResult result;

    if (request.sessionScope)
        result.activity.append(qs("[Replace Preview] Prototype session preview generated."));
    else
        result.activity.append(qs("[Replace Preview] Prototype document preview generated."));

    appendReplacePreviewRows(result, action, request.query,
        request.replacement, request.mode, request.sessionScope);
    return result;
}

SearchStudioActionResult makeReplaceImpactResult(const SearchStudioReplaceRequest &request,
    const QString &action, const QString &summaryActionLabel,
    const QString &summaryTarget)
{
    SearchStudioActionResult result;

    if (request.sessionScope)
    {
        result.activity.append(QString("[Replace] Replace in session | query=%1 | replacement=%2 | mode=%3")
            .formatArgs(request.query, request.replacement, request.mode));
        appendReplacePreviewRows(result, action, request.query,
            request.replacement, request.mode, true);
        result.rows.append(makeRow(summaryActionLabel, summaryTarget, request.query,
            request.mode,
            qs("Prototype replace-in-session summary with per-document impact rows above."),
            qs("Replace in Session"),
            QString("Replacement payload: %1\nMode: %2\n\nPer-document impact rows above would be driven by Geany core later.")
                .formatArgs(request.replacement, request.mode),
            false));
    }
    else
    {
        result.activity.append(QString("[Replace] Replace in document | query=%1 | replacement=%2 | mode=%3")
            .formatArgs(request.query, request.replacement, request.mode));
        appendReplacePreviewRows(result, action, request.query,
            request.replacement, request.mode, false);
        result.rows.append(makeRow(summaryActionLabel, summaryTarget, request.query,
            request.mode,
            qs("Prototype replace-in-document summary with impact rows above."),
            qs("Replace in Document"),
            QString("Replacement payload: %1\nMode: %2\n\nImpact rows above mirror the current Search Studio model.")
                .formatArgs(request.replacement, request.mode),
            false));
    }

    return result;
}

SearchStudioActionResult makeFindInFilesResult(const SearchStudioFindInFilesRequest &request)
{
    SearchStudioActionResult result;

    result.activity.append(QString("[Find in Files] query=%1 | directory=%2 | mode=%3")
        .formatArgs(request.query, request.directory, request.mode));
    appendImpactRows(result, qs("Find in Files Hit"), request.query,
        request.mode, qs("Captured"), true);
    result.rows.append(makeRow(qs("Find in Files"), request.directory,
        request.query, request.mode,
        qs("Prototype directory search launched with structured hit ingestion."),
        qs("Find in Files"),
        QString("Directory: %1\nMode: %2\n\nResults above mirror a future structured ripgrep-style ingestion path.")
            .formatArgs(request.directory, request.mode),
        false));

    return result;
}

SearchStudioActionResult makeMarkResult(const SearchStudioMarkRequest &request,
    const QString &action, const QString &summaryActionLabel,
    const QString &summaryTarget)
{
    SearchStudioActionResult result;

    if (request.sessionScope)
    {
        result.activity.append(QString("[Mark] Session | query=%1 | mode=%2")
            .formatArgs(request.query, request.mode));
        appendImpactRows(result, action, request.query, request.mode, qs("Marked"), true);
        result.rows.append(makeRow(summaryActionLabel, summaryTarget, request.query,
            request.mode,
            qs("Prototype session mark summary with per-document impact rows above."),
            qs("Mark in Session"),
            qs("Session mark impact rows above mirror the matured Search Studio behavior from the main tree."),
            false));
    }
    else
    {
        const QString bookmark = request.bookmarkLines ? qs("on") : qs("off");
        const QString purge = request.purgeBookmarks ? qs("on") : qs("off");
        result.activity.append(QString("[Mark] query=%1 | mode=%2 | bookmarks=%3 | purge=%4")
            .formatArgs(request.query, request.mode, bookmark, purge));
        appendImpactRows(result, action, request.query, request.mode, qs("Marked"), false);
        result.rows.append(makeRow(summaryActionLabel, summaryTarget, request.query,
            request.mode,
            QString("Marked representative matches; bookmark-lines=%1; purge-first=%2.")
                .formatArgs(bookmark, purge),
            qs("Mark"),
            qs("Prototype active-document mark summary."),
            false));
    }

    return result;
}

} // namespace SearchStudioBackend
