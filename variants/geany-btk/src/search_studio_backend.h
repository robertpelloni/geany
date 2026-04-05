#pragma once

#include <QList>
#include <QString>
#include <QStringList>

enum class SearchStudioActionKind
{
    Studio,
    Find,
    Count,
    CollectHits,
    Replace,
    ReplacePreview,
    FindInFiles,
    Mark
};

enum class SearchStudioResultKind
{
    Summary,
    Impact,
    Preview,
    Capture
};

enum class SearchStudioTargetScope
{
    SearchStudio,
    ActiveDocument,
    OpenDocuments,
    Directory,
    ExplicitTarget
};

struct SearchStudioResultSpec
{
    SearchStudioActionKind actionKind = SearchStudioActionKind::Studio;
    SearchStudioResultKind kind = SearchStudioResultKind::Summary;
    SearchStudioTargetScope scope = SearchStudioTargetScope::ExplicitTarget;
    QString action;
    QString target;
    QString query;
    QString mode;
    QString summary;
    QString previewTitle;
    QString previewBody;
    bool navigable = false;
};

struct SearchStudioActionResult
{
    QStringList activity;
    QList<SearchStudioResultSpec> rows;
};

struct SearchStudioDocumentImpact
{
    QString file;
    int line = 0;
    int count = 0;
    QString context;
};

struct SearchStudioReplacePreviewImpact
{
    QString file;
    int line = 0;
    QString originalLine;
    QString replacementLine;
    QString matchedText;
    QString replacementText;
};

struct SearchStudioFindRequest
{
    QString query;
    QString mode;
    bool sessionScope = false;
};

struct SearchStudioFindActionSpec
{
    SearchStudioActionKind actionKind = SearchStudioActionKind::Find;
    QString activityMessage;
    QString impactAction;
    QString impactSummaryPrefix;
    QString summaryAction;
    QString summaryText;
    QString summaryPreviewTitle;
    QString summaryPreviewBody;
    SearchStudioTargetScope summaryScope = SearchStudioTargetScope::ActiveDocument;
    bool includeImpactRows = false;
};

struct SearchStudioReplaceRequest
{
    QString query;
    QString replacement;
    QString mode;
    bool sessionScope = false;
};

struct SearchStudioReplaceActionSpec
{
    SearchStudioActionKind actionKind = SearchStudioActionKind::Replace;
    QString activityMessage;
    QString rowAction;
    QString summaryAction;
    QString summaryText;
    QString summaryPreviewTitle;
    QString summaryPreviewBody;
    SearchStudioTargetScope summaryScope = SearchStudioTargetScope::ActiveDocument;
    bool previewRows = false;
};

struct SearchStudioMarkRequest
{
    QString query;
    QString mode;
    bool bookmarkLines = false;
    bool purgeBookmarks = false;
    bool sessionScope = false;
};

struct SearchStudioMarkActionSpec
{
    SearchStudioActionKind actionKind = SearchStudioActionKind::Mark;
    QString activityMessage;
    QString impactAction;
    QString impactSummaryPrefix;
    QString summaryAction;
    QString summaryText;
    QString summaryPreviewTitle;
    QString summaryPreviewBody;
    SearchStudioTargetScope summaryScope = SearchStudioTargetScope::ActiveDocument;
    bool includeImpactRows = false;
};

struct SearchStudioFindInFilesRequest
{
    QString query;
    QString directory;
    QString mode;
};

struct SearchStudioFindInFilesActionSpec
{
    SearchStudioActionKind actionKind = SearchStudioActionKind::FindInFiles;
    QString activityMessage;
    QString captureAction;
    QString captureSummaryPrefix;
    QString summaryAction;
    QString summaryText;
    QString summaryPreviewTitle;
    QString summaryPreviewBody;
    SearchStudioTargetScope summaryScope = SearchStudioTargetScope::Directory;
    bool includeCaptureRows = false;
};

class SearchStudioSearchService
{
public:
    virtual ~SearchStudioSearchService() = default;

    virtual QList<SearchStudioDocumentImpact> buildFindImpactRows(
        const SearchStudioFindRequest &request) const = 0;
    virtual QList<SearchStudioDocumentImpact> buildMarkImpactRows(
        const SearchStudioMarkRequest &request) const = 0;
    virtual QList<SearchStudioDocumentImpact> buildFindInFilesCaptureRows(
        const SearchStudioFindInFilesRequest &request) const = 0;
    virtual QList<SearchStudioReplacePreviewImpact> buildReplacePreviewRows(
        const SearchStudioReplaceRequest &request) const = 0;
};

namespace SearchStudioBackend {

const SearchStudioSearchService &defaultSearchService();

SearchStudioActionResult makeActiveDocumentSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody);

SearchStudioActionResult makeSessionSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody);

SearchStudioActionResult executeFindAction(const SearchStudioFindRequest &request,
    const SearchStudioFindActionSpec &action);
SearchStudioActionResult executeFindAction(const SearchStudioFindRequest &request,
    const SearchStudioFindActionSpec &action,
    const SearchStudioSearchService &service);

SearchStudioActionResult executeReplaceAction(const SearchStudioReplaceRequest &request,
    const SearchStudioReplaceActionSpec &action);
SearchStudioActionResult executeReplaceAction(const SearchStudioReplaceRequest &request,
    const SearchStudioReplaceActionSpec &action,
    const SearchStudioSearchService &service);

SearchStudioActionResult executeMarkAction(const SearchStudioMarkRequest &request,
    const SearchStudioMarkActionSpec &action);
SearchStudioActionResult executeMarkAction(const SearchStudioMarkRequest &request,
    const SearchStudioMarkActionSpec &action,
    const SearchStudioSearchService &service);

SearchStudioActionResult executeFindInFilesAction(
    const SearchStudioFindInFilesRequest &request,
    const SearchStudioFindInFilesActionSpec &action);
SearchStudioActionResult executeFindInFilesAction(
    const SearchStudioFindInFilesRequest &request,
    const SearchStudioFindInFilesActionSpec &action,
    const SearchStudioSearchService &service);

SearchStudioActionResult makeCountResult(const SearchStudioFindRequest &request);
SearchStudioActionResult makeCollectedHitsResult(const SearchStudioFindRequest &request);
SearchStudioActionResult makeReplacePreviewResult(const SearchStudioReplaceRequest &request,
    const QString &action);
SearchStudioActionResult makeReplaceImpactResult(const SearchStudioReplaceRequest &request,
    const QString &action, const QString &summaryActionLabel,
    const QString &summaryTarget);
SearchStudioActionResult makeFindInFilesResult(const SearchStudioFindInFilesRequest &request);
SearchStudioActionResult makeMarkResult(const SearchStudioMarkRequest &request,
    const QString &action, const QString &summaryActionLabel,
    const QString &summaryTarget);

} // namespace SearchStudioBackend
