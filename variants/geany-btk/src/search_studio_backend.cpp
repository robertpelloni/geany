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

void appendImpactRows(SearchStudioActionResult &result,
    SearchStudioActionKind actionKind, const QString &action,
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

        result.rows.append(makeResultSpec(actionKind,
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

void appendReplacePreviewRows(SearchStudioActionResult &result,
    SearchStudioActionKind actionKind, const QString &action,
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

        result.rows.append(makeResultSpec(actionKind,
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

SearchStudioActionResult executeFindAction(const SearchStudioFindRequest &request,
    const SearchStudioFindActionSpec &action)
{
    SearchStudioActionResult result;

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    if (action.includeImpactRows)
        appendImpactRows(result, action.actionKind, action.impactAction,
            request.query, request.mode, action.impactSummaryPrefix,
            request.sessionScope);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, QString(), request.query,
            request.mode, action.summaryText, action.summaryPreviewTitle,
            action.summaryPreviewBody, false));

    return result;
}

SearchStudioActionResult executeReplaceAction(const SearchStudioReplaceRequest &request,
    const SearchStudioReplaceActionSpec &action)
{
    SearchStudioActionResult result;

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    appendReplacePreviewRows(result, action.actionKind, action.rowAction,
        request.query, request.replacement, request.mode, request.sessionScope);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, QString(), request.query,
            request.mode, action.summaryText, action.summaryPreviewTitle,
            action.summaryPreviewBody, false));

    return result;
}

SearchStudioActionResult executeMarkAction(const SearchStudioMarkRequest &request,
    const SearchStudioMarkActionSpec &action)
{
    SearchStudioActionResult result;

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    if (action.includeImpactRows)
        appendImpactRows(result, action.actionKind, action.impactAction,
            request.query, request.mode, action.impactSummaryPrefix,
            request.sessionScope);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, QString(), request.query,
            request.mode, action.summaryText, action.summaryPreviewTitle,
            action.summaryPreviewBody, false));

    return result;
}

SearchStudioActionResult executeFindInFilesAction(
    const SearchStudioFindInFilesRequest &request,
    const SearchStudioFindInFilesActionSpec &action)
{
    SearchStudioActionResult result;

    if (! action.activityMessage.isEmpty())
        result.activity.append(action.activityMessage);
    if (action.includeCaptureRows)
        appendImpactRows(result, action.actionKind, action.captureAction,
            request.query, request.mode, action.captureSummaryPrefix, TRUE);
    if (! action.summaryAction.isEmpty())
        result.rows.append(makeResultSpec(action.actionKind, SearchStudioResultKind::Summary,
            action.summaryScope, action.summaryAction, request.directory,
            request.query, request.mode, action.summaryText,
            action.summaryPreviewTitle, action.summaryPreviewBody, false));

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
        qs("Prototype aggregate count across open documents with per-document impact rows above.") :
        qs("Prototype count summary for the active document.");
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
    action.summaryPreviewBody = qs("Prototype open-document hit collection summary.");
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
        qs("[Replace Preview] Prototype session preview generated.") :
        qs("[Replace Preview] Prototype document preview generated.");
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
        qs("Prototype replace-in-session summary with per-document impact rows above.") :
        qs("Prototype replace-in-document summary with impact rows above.");
    spec.summaryPreviewTitle = request.sessionScope ? qs("Replace in Session") : qs("Replace in Document");
    spec.summaryPreviewBody = request.sessionScope ?
        QString("Replacement payload: %1\nMode: %2\n\nPer-document impact rows above would be driven by Geany core later.")
            .formatArgs(request.replacement, request.mode) :
        QString("Replacement payload: %1\nMode: %2\n\nImpact rows above mirror the current Search Studio model.")
            .formatArgs(request.replacement, request.mode);
    spec.summaryScope = request.sessionScope ?
        SearchStudioTargetScope::OpenDocuments : SearchStudioTargetScope::ActiveDocument;
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
    action.summaryText = qs("Prototype directory search launched with structured hit ingestion.");
    action.summaryPreviewTitle = qs("Find in Files");
    action.summaryPreviewBody = QString("Directory: %1\nMode: %2\n\nResults above mirror a future structured ripgrep-style ingestion path.")
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
        qs("Prototype session mark summary with per-document impact rows above.") :
        QString("Marked representative matches; bookmark-lines=%1; purge-first=%2.")
            .formatArgs(bookmark, purge);
    spec.summaryPreviewTitle = request.sessionScope ? qs("Mark in Session") : qs("Mark");
    spec.summaryPreviewBody = request.sessionScope ?
        qs("Session mark impact rows above mirror the matured Search Studio behavior from the main tree.") :
        qs("Prototype active-document mark summary.");
    spec.summaryScope = request.sessionScope ?
        SearchStudioTargetScope::OpenDocuments : SearchStudioTargetScope::ActiveDocument;
    spec.includeImpactRows = true;
    return executeMarkAction(request, spec);
}

} // namespace SearchStudioBackend
