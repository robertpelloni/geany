#pragma once

#include <QList>
#include <QString>
#include <QStringList>

struct SearchStudioResultRow
{
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
    QList<SearchStudioResultRow> rows;
};

struct SearchStudioFindRequest
{
    QString query;
    QString mode;
    bool sessionScope = false;
};

struct SearchStudioReplaceRequest
{
    QString query;
    QString replacement;
    QString mode;
    bool sessionScope = false;
};

struct SearchStudioMarkRequest
{
    QString query;
    QString mode;
    bool bookmarkLines = false;
    bool purgeBookmarks = false;
    bool sessionScope = false;
};

struct SearchStudioFindInFilesRequest
{
    QString query;
    QString directory;
    QString mode;
};

namespace SearchStudioBackend {

SearchStudioActionResult makeActiveDocumentSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody);

SearchStudioActionResult makeSessionSummary(const QString &activityMessage,
    const QString &action, const QString &query, const QString &mode,
    const QString &summary, const QString &previewTitle,
    const QString &previewBody);

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
