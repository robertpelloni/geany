#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include "search_studio_backend.h"

namespace {

static QString qs(const char *text)
{
    return QString::fromUtf8(text);
}

struct ModeControls
{
    QRadioButton *normal = nullptr;
    QRadioButton *extended = nullptr;
    QRadioButton *regex = nullptr;
};

struct FindControls
{
    QComboBox *query = nullptr;
    ModeControls mode;
};

struct ReplaceControls
{
    QComboBox *query = nullptr;
    QComboBox *replacement = nullptr;
    ModeControls mode;
};

struct FindInFilesControls
{
    QComboBox *query = nullptr;
    QComboBox *filters = nullptr;
    QComboBox *directory = nullptr;
    ModeControls mode;
};

struct MarkControls
{
    QComboBox *query = nullptr;
    QCheckBox *bookmarkLines = nullptr;
    QCheckBox *purgeBookmarks = nullptr;
    ModeControls mode;
};

class SearchStudioDialog final : public QDialog
{
public:
    SearchStudioDialog()
    {
        setWindowTitle("Geany Search Studio (BTK Experimental)");
        resize(1240, 820);

        auto *layout = new QVBoxLayout(this);

        auto *hero = new QLabel(
            "<h2>Geany Search Studio</h2>"
            "<p>Experimental BTK-native search surface mirroring the matured Search Studio model from the main tree: "
            "dense actions, session-aware workflows, structured results, and a lower Activity / Results / Diff Preview navigator.</p>");
        hero->setWordWrap(true);
        layout->addWidget(hero);

        auto *splitter = new QSplitter(Qt::Vertical, this);
        splitter->setChildrenCollapsible(false);

        topTabs_ = new QTabWidget();
        topTabs_->addTab(createFindTab(), "Find");
        topTabs_->addTab(createReplaceTab(), "Replace");
        topTabs_->addTab(createFindInFilesTab(), "Find in Files");
        topTabs_->addTab(createFindInProjectsTab(), "Find in Projects");
        topTabs_->addTab(createMarkTab(), "Mark");
        splitter->addWidget(topTabs_);

        lowerTabs_ = new QTabWidget();
        lowerTabs_->addTab(createActivityPane(), "Activity");
        lowerTabs_->addTab(createResultsPane(), "Results");
        lowerTabs_->addTab(createPreviewPane(), "Diff Preview");
        splitter->addWidget(lowerTabs_);
        splitter->setStretchFactor(0, 3);
        splitter->setStretchFactor(1, 2);
        layout->addWidget(splitter, 1);

        auto *footer = new QHBoxLayout();
        footer->addWidget(new QLabel(
            "Prototype status: mirrored navigator/workbench behaviors first, Geany core wiring next."));
        footer->addStretch(1);
        auto *closeButton = new QPushButton("Close");
        connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
        footer->addWidget(closeButton);

        layout->addLayout(footer);

        auto *transparencyFrame = new QGroupBox("Transparency");
        auto *transLayout = new QVBoxLayout(transparencyFrame);
        transLayout->addWidget(new QCheckBox("Enable"));
        auto *transModes = new QHBoxLayout();
        transModes->addWidget(new QRadioButton("On losing focus"));
        transModes->addWidget(new QRadioButton("Always"));
        transLayout->addLayout(transModes);
        transLayout->addWidget(new QSlider(Qt::Horizontal));
        layout->addWidget(transparencyFrame, 0, Qt::AlignRight);

        appendActivity("[Studio] BTK Search Studio initialized with lower Activity / Results / Diff Preview navigation.");
        appendResult(
            "Studio",
            "Search Studio",
            "Initialization",
            "N/A",
            "Prototype shell is ready with session-aware Find / Replace / Mark action coverage.",
            "Search Studio",
            "Select a result row to inspect richer preview text. This BTK prototype mirrors the current Search Studio navigator model.",
            false);
        setPreview("Diff Preview", "Select a result row to inspect preview details.");
    }

private:
    enum DataRole
    {
        PreviewTitleRole = Qt::UserRole,
        PreviewBodyRole,
        NavigableRole,
        ActionKindRole,
        ResultKindRole,
        TargetScopeRole
    };

    QTabWidget *topTabs_ = nullptr;
    QTabWidget *lowerTabs_ = nullptr;
    QPlainTextEdit *activityView_ = nullptr;
    QTreeWidget *resultsView_ = nullptr;
    QPlainTextEdit *previewView_ = nullptr;

    FindControls find_;
    ReplaceControls replace_;
    FindInFilesControls fif_;
    FindInFilesControls fip_;
    MarkControls mark_;

    static QString textOrPlaceholder(const QString &text, const QString &fallback)
    {
        return text.trimmed().isEmpty() ? fallback : text.trimmed();
    }

    QString modeName(const ModeControls &mode) const
    {
        if (mode.regex != nullptr && mode.regex->isChecked())
            return "Regex";
        if (mode.extended != nullptr && mode.extended->isChecked())
            return "Extended";
        return "Normal";
    }

    void appendActivity(const QString &message)
    {
        if (activityView_->toPlainText().isEmpty())
            activityView_->setPlainText(message);
        else
            activityView_->appendPlainText(message);
        lowerTabs_->setCurrentWidget(activityView_);
    }

    void setPreview(const QString &title, const QString &body)
    {
        previewView_->setPlainText(QString("%1\n\n%2").formatArgs(title, body));
    }

    void appendResult(const QString &action, const QString &target, const QString &query,
        const QString &mode, const QString &summary, const QString &previewTitle,
        const QString &previewBody, bool navigable)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, action);
        item->setText(1, target);
        item->setText(2, query);
        item->setText(3, mode);
        item->setText(4, summary);
        item->setData(0, PreviewTitleRole, previewTitle);
        item->setData(0, PreviewBodyRole, previewBody);
        item->setData(0, NavigableRole, navigable);
        resultsView_->insertTopLevelItem(0, item);
        lowerTabs_->setCurrentWidget(resultsView_);
    }

    void appendResult(const SearchStudioResultSpec &row)
    {
        auto *item = new QTreeWidgetItem();
        item->setText(0, row.action);
        item->setText(1, row.target);
        item->setText(2, row.query);
        item->setText(3, row.mode);
        item->setText(4, row.summary);
        item->setData(0, PreviewTitleRole, row.previewTitle);
        item->setData(0, PreviewBodyRole, row.previewBody);
        item->setData(0, NavigableRole, row.navigable);
        item->setData(0, ActionKindRole, static_cast<int>(row.actionKind));
        item->setData(0, ResultKindRole, static_cast<int>(row.kind));
        item->setData(0, TargetScopeRole, static_cast<int>(row.scope));
        resultsView_->insertTopLevelItem(0, item);
        lowerTabs_->setCurrentWidget(resultsView_);
    }

    void applyActionResult(const SearchStudioActionResult &result)
    {
        for (const auto &message : result.activity)
            appendActivity(message);
        for (const auto &row : result.rows)
            appendResult(row);
    }

    QGroupBox *createModeGroup(ModeControls &mode)
    {
        auto *group = new QGroupBox("Search mode");
        auto *layout = new QHBoxLayout(group);
        mode.normal = new QRadioButton("Normal");
        mode.extended = new QRadioButton("Extended");
        mode.regex = new QRadioButton("Regular expression");
        mode.normal->setChecked(true);
        layout->addWidget(mode.normal);
        layout->addWidget(mode.extended);
        layout->addWidget(mode.regex);
        layout->addStretch(1);
        return group;
    }

    QWidget *createActivityPane()
    {
        activityView_ = new QPlainTextEdit();
        activityView_->setReadOnly(true);
        activityView_->setPlaceholderText("Prototype activity log...");
        return activityView_;
    }

    QWidget *createResultsPane()
    {
        resultsView_ = new QTreeWidget();
        resultsView_->setColumnCount(5);
        resultsView_->setHeaderLabels({"Action", "Target", "Query", "Mode", "Summary"});
        resultsView_->setRootIsDecorated(false);
        resultsView_->setAlternatingRowColors(true);
        connect(resultsView_, &QTreeWidget::itemSelectionChanged, this, [this]() {
            const auto items = resultsView_->selectedItems();
            if (items.isEmpty())
            {
                setPreview("Diff Preview", "Select a result row to inspect preview details.");
                return;
            }

            auto *item = items.first();
            setPreview(item->data(0, PreviewTitleRole).toString(), item->data(0, PreviewBodyRole).toString());
            lowerTabs_->setCurrentWidget(previewView_);
        });
        connect(resultsView_, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item, int) {
            if (item == nullptr)
                return;

            const bool navigable = item->data(0, NavigableRole).toBool();
            lowerTabs_->setCurrentWidget(previewView_);
            if (!navigable)
            {
                appendActivity(QString("[Results] %1 on %2 is informational in the prototype; inspect Diff Preview.")
                    .formatArgs(item->text(0), item->text(1)));
                return;
            }

            appendActivity(QString("[Results] Prototype navigation would jump to %1.").formatArg(item->text(1)));
        });
        return resultsView_;
    }

    QWidget *createPreviewPane()
    {
        previewView_ = new QPlainTextEdit();
        previewView_->setReadOnly(true);
        previewView_->setPlaceholderText("Prototype diff / preview surface...");
        return previewView_;
    }

    QWidget *createFindTab()
    {
        auto *page = new QWidget();
        auto *layout = new QVBoxLayout(page);

        auto *form = new QFormLayout();
        find_.query = new QComboBox();
        find_.query->setEditable(true);
        form->addRow("Find what:", find_.query);
        layout->addLayout(form);
        layout->addWidget(createModeGroup(find_.mode));

        auto *options = new QGroupBox("Find options");
        auto *grid = new QGridLayout(options);
        grid->addWidget(new QCheckBox("Match case"), 0, 0);
        grid->addWidget(new QCheckBox("Match whole word only"), 0, 1);
        grid->addWidget(new QCheckBox("Wrap around"), 0, 2);
        grid->addWidget(new QCheckBox(". matches newline"), 1, 0);
        grid->addWidget(new QCheckBox("In selection"), 1, 1);
        grid->addWidget(new QCheckBox("Backward direction"), 1, 2);
        layout->addWidget(options);

        auto *actions = new QHBoxLayout();
        auto addFindButton = [&](const QString &label, const QString &activity, const QString &summary) {
            auto *button = new QPushButton(label);
            connect(button, &QPushButton::clicked, this, [this, activity, summary]() {
                const QString query = textOrPlaceholder(find_.query->currentText(), "current selection");
                const QString mode = modeName(find_.mode);
                SearchStudioFindRequest request;
                SearchStudioFindActionSpec action;

                request.query = query;
                request.mode = mode;
                action.actionKind = SearchStudioActionKind::Find;
                action.activityMessage = QString("[Find] %1 | query=%2 | mode=%3").formatArgs(activity, query, mode);
                action.summaryAction = activity;
                action.summaryText = summary;
                action.summaryPreviewTitle = QString("%1 — Active Document").formatArg(activity);
                action.summaryPreviewBody = QString("Prototype action: %1\n\nQuery: %2\nMode: %3\n\nThis BTK surface mirrors the current Search Studio workflow but is not yet wired to Geany core services.")
                    .formatArgs(activity, query, mode);
                action.summaryScope = SearchStudioTargetScope::ActiveDocument;
                applyActionResult(SearchStudioBackend::executeFindAction(request, action));
            });
            actions->addWidget(button);
        };

        addFindButton("Find Next", "Find", "Found next occurrence in the active document.");
        addFindButton("Find Previous", "Find Previous", "Found previous occurrence in the active document.");

        auto *countButton = new QPushButton("Count");
        connect(countButton, &QPushButton::clicked, this, [this]() {
            SearchStudioFindRequest request;
            request.query = textOrPlaceholder(find_.query->currentText(), "needle");
            request.mode = modeName(find_.mode);
            applyActionResult(SearchStudioBackend::makeCountResult(request));
        });
        actions->addWidget(countButton);

        auto *countSessionButton = new QPushButton("Count Session");
        connect(countSessionButton, &QPushButton::clicked, this, [this]() {
            SearchStudioFindRequest request;
            request.query = textOrPlaceholder(find_.query->currentText(), "needle");
            request.mode = modeName(find_.mode);
            request.sessionScope = true;
            applyActionResult(SearchStudioBackend::makeCountResult(request));
        });
        actions->addWidget(countSessionButton);

        auto *markButton = new QPushButton("Mark / Bookmark");
        connect(markButton, &QPushButton::clicked, this, [this]() {
            topTabs_->setCurrentIndex(4);
            appendActivity("[Find] Routed to Mark workflow from the Find tab.");
        });
        actions->addWidget(markButton);

        auto *collectDocButton = new QPushButton("Find All in current document");
        connect(collectDocButton, &QPushButton::clicked, this, [this]() {
            SearchStudioFindRequest request;
            request.query = textOrPlaceholder(find_.query->currentText(), "needle");
            request.mode = modeName(find_.mode);
            applyActionResult(SearchStudioBackend::makeCollectedHitsResult(request));
        });
        actions->addWidget(collectDocButton);

        auto *collectSessionButton = new QPushButton("Find All in all opened documents");
        connect(collectSessionButton, &QPushButton::clicked, this, [this]() {
            SearchStudioFindRequest request;
            request.query = textOrPlaceholder(find_.query->currentText(), "needle");
            request.mode = modeName(find_.mode);
            request.sessionScope = true;
            applyActionResult(SearchStudioBackend::makeCollectedHitsResult(request));
        });
        actions->addWidget(collectSessionButton);

        auto *clearResultsButton = new QPushButton("Clear Results");
        connect(clearResultsButton, &QPushButton::clicked, this, [this]() {
            resultsView_->clear();
            setPreview("Diff Preview", "Select a result row to inspect preview details.");
            appendActivity("[Results] Cleared structured results pane.");
            appendResult(
                "Results",
                "Search Studio",
                "Clear",
                "N/A",
                "Cleared structured result rows.",
                "Search Studio",
                "Structured results were cleared; future actions will repopulate the navigator.",
                false);
        });
        actions->addWidget(clearResultsButton);

        auto *focusResultsButton = new QPushButton("Focus Results");
        connect(focusResultsButton, &QPushButton::clicked, this, [this]() {
            resultsView_->setFocus();
            lowerTabs_->setCurrentIndex(1);
            appendActivity("[Results] Focused the Search Studio results navigator.");
        });
        actions->addWidget(focusResultsButton);

        auto *nextResultButton = new QPushButton("Next Result");
        connect(nextResultButton, &QPushButton::clicked, this, [this]() {
            int rowCount = resultsView_->topLevelItemCount();
            if (rowCount > 0) {
                auto *current = resultsView_->currentItem();
                int nextIndex = 0;
                if (current) {
                    nextIndex = (resultsView_->indexOfTopLevelItem(current) + 1) % rowCount;
                }
                auto *nextItem = resultsView_->topLevelItem(nextIndex);
                resultsView_->setCurrentItem(nextItem);
                lowerTabs_->setCurrentIndex(1);
            }
        });
        actions->addWidget(nextResultButton);

        auto *prevResultButton = new QPushButton("Previous Result");
        connect(prevResultButton, &QPushButton::clicked, this, [this]() {
            int rowCount = resultsView_->topLevelItemCount();
            if (rowCount > 0) {
                auto *current = resultsView_->currentItem();
                int prevIndex = rowCount - 1;
                if (current) {
                    prevIndex = (resultsView_->indexOfTopLevelItem(current) - 1 + rowCount) % rowCount;
                }
                auto *prevItem = resultsView_->topLevelItem(prevIndex);
                resultsView_->setCurrentItem(prevItem);
                lowerTabs_->setCurrentIndex(1);
            }
        });
        actions->addWidget(prevResultButton);

        auto *classicButton = new QPushButton("Open classic Find dialog");
        connect(classicButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Find] Prototype bridge would open the classic Find dialog with synchronized state.");
        });
        actions->addWidget(classicButton);

        layout->addLayout(actions);
        return page;
    }

    QWidget *createReplaceTab()
    {
        auto *page = new QWidget();
        auto *layout = new QVBoxLayout(page);

        auto *form = new QFormLayout();
        replace_.query = new QComboBox();
        replace_.replacement = new QComboBox();
        replace_.query->setEditable(true);
        replace_.replacement->setEditable(true);

        auto *swapButton = new QPushButton();
        swapButton->setIcon(QIcon::fromTheme("view-refresh"));
        swapButton->setToolTip("Swap Find and Replace text");
        connect(swapButton, &QPushButton::clicked, this, [this]() {
            QString findText = replace_.query->currentText();
            QString replaceText = replace_.replacement->currentText();
            replace_.query->setCurrentText(replaceText);
            replace_.replacement->setCurrentText(findText);
        });

        auto *findRow = new QHBoxLayout();
        findRow->addWidget(replace_.query, 1);
        findRow->addWidget(swapButton);

        form->addRow("Find what:", findRow);
        form->addRow("Replace with:", replace_.replacement);
        layout->addLayout(form);
        layout->addWidget(createModeGroup(replace_.mode));

        auto *options = new QGroupBox("Replace options");
        auto *grid = new QGridLayout(options);
        grid->addWidget(new QCheckBox("Match case"), 0, 0);
        grid->addWidget(new QCheckBox("Match whole word only"), 0, 1);
        grid->addWidget(new QCheckBox("Wrap around"), 0, 2);
        grid->addWidget(new QCheckBox(". matches newline"), 1, 0);
        grid->addWidget(new QCheckBox("In selection"), 1, 1);
        grid->addWidget(new QCheckBox("Backward direction"), 1, 2);
        layout->addWidget(options);

        auto *actions = new QHBoxLayout();
        auto addReplaceButton = [&](const QString &label, const QString &activity, const QString &summary) {
            auto *button = new QPushButton(label);
            connect(button, &QPushButton::clicked, this, [this, activity, summary]() {
                const QString query = textOrPlaceholder(replace_.query->currentText(), "needle");
                const QString replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
                const QString mode = modeName(replace_.mode);
                SearchStudioReplaceRequest request;
                SearchStudioReplaceActionSpec action;

                request.query = query;
                request.replacement = replacement;
                request.mode = mode;
                action.actionKind = SearchStudioActionKind::Replace;
                action.activityMessage = QString("[Replace] %1 | query=%2 | replacement=%3 | mode=%4")
                    .formatArgs(activity, query, replacement, mode);
                action.summaryAction = activity;
                action.summaryText = summary;
                action.summaryPreviewTitle = QString("%1 — Active Document").formatArg(activity);
                action.summaryPreviewBody = QString("Query: %1\nReplacement: %2\nMode: %3\n\nPrototype action coverage only; engine wiring comes next.")
                    .formatArgs(query, replacement, mode);
                action.summaryScope = SearchStudioTargetScope::ActiveDocument;
                action.previewRows = false;
                applyActionResult(SearchStudioBackend::executeReplaceAction(request, action));
            });
            actions->addWidget(button);
        };

        addReplaceButton("Find Next", "Replace/Find", "Found the next candidate match from the Replace tab.");
        addReplaceButton("Replace", "Replace", "Replaced the current match in the active document.");
        addReplaceButton("Replace & Find", "Replace & Find", "Replaced the current match and advanced to the next one.");

        auto *replaceDocButton = new QPushButton("Replace in Document");
        connect(replaceDocButton, &QPushButton::clicked, this, [this]() {
            SearchStudioReplaceRequest request;
            request.query = textOrPlaceholder(replace_.query->currentText(), "needle");
            request.replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            request.mode = modeName(replace_.mode);
            applyActionResult(SearchStudioBackend::makeReplaceImpactResult(request,
                qs("Replace Impact"), qs("Replace in Document"), qs("Active Document")));
        });
        actions->addWidget(replaceDocButton);

        auto *replaceSessionButton = new QPushButton("Replace All in All Opened Documents");
        connect(replaceSessionButton, &QPushButton::clicked, this, [this]() {
            SearchStudioReplaceRequest request;
            request.query = textOrPlaceholder(replace_.query->currentText(), "needle");
            request.replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            request.mode = modeName(replace_.mode);
            request.sessionScope = true;
            applyActionResult(SearchStudioBackend::makeReplaceImpactResult(request,
                qs("Session Replace Impact"), qs("Replace All in All Opened Documents"), qs("Open Documents")));
        });
        actions->addWidget(replaceSessionButton);

        auto *previewDocButton = new QPushButton("Preview in Document");
        connect(previewDocButton, &QPushButton::clicked, this, [this]() {
            SearchStudioReplaceRequest request;
            request.query = textOrPlaceholder(replace_.query->currentText(), "needle");
            request.replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            request.mode = modeName(replace_.mode);
            applyActionResult(SearchStudioBackend::makeReplacePreviewResult(request,
                qs("Replace Preview")));
        });
        actions->addWidget(previewDocButton);

        auto *previewSessionButton = new QPushButton("Preview in Session");
        connect(previewSessionButton, &QPushButton::clicked, this, [this]() {
            SearchStudioReplaceRequest request;
            request.query = textOrPlaceholder(replace_.query->currentText(), "needle");
            request.replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            request.mode = modeName(replace_.mode);
            request.sessionScope = true;
            applyActionResult(SearchStudioBackend::makeReplacePreviewResult(request,
                qs("Replace Preview Session")));
        });
        actions->addWidget(previewSessionButton);

        auto *classicButton = new QPushButton("Open classic Replace dialog");
        connect(classicButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Replace] Prototype bridge would open the classic Replace dialog with synchronized state.");
        });
        actions->addWidget(classicButton);

        layout->addLayout(actions);
        return page;
    }

    QWidget *createFindInFilesTab()
    {
        auto *page = new QWidget();
        auto *layout = new QVBoxLayout(page);

        auto *form = new QFormLayout();
        fif_.query = new QComboBox();
        fif_.filters = new QComboBox();
        fif_.directory = new QComboBox();
        fif_.query->setEditable(true);
        fif_.filters->setEditable(true);
        fif_.directory->setEditable(true);
        fif_.directory->setCurrentText(".");
        form->addRow("Find what:", fif_.query);
        form->addRow("Filters:", fif_.filters);
        form->addRow("Directory:", fif_.directory);
        layout->addLayout(form);
        layout->addWidget(createModeGroup(fif_.mode));

        auto *options = new QGroupBox("Find in Files options");
        auto *grid = new QGridLayout(options);
        grid->addWidget(new QCheckBox("Recursive"), 0, 0);
        grid->addWidget(new QCheckBox("In hidden folders"), 0, 1);
        grid->addWidget(new QCheckBox("Match case"), 0, 2);
        grid->addWidget(new QCheckBox("Whole word"), 1, 0);
        grid->addWidget(new QCheckBox("Invert match"), 1, 1);
        grid->addWidget(new QCheckBox("Use project patterns"), 1, 2);
        layout->addWidget(options);

        auto *actions = new QHBoxLayout();
        auto *findAllButton = new QPushButton("Find All");
        connect(findAllButton, &QPushButton::clicked, this, [this]() {
            SearchStudioFindInFilesRequest request;
            request.query = textOrPlaceholder(fif_.query->currentText(), "needle");
            request.directory = textOrPlaceholder(fif_.directory->currentText(), ".");
            request.mode = modeName(fif_.mode);
            applyActionResult(SearchStudioBackend::makeFindInFilesResult(request));
        });
        actions->addWidget(findAllButton);

        auto *replaceFilesButton = new QPushButton("Replace in Files");
        connect(replaceFilesButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Find in Files] Prototype replace-in-files entry point reserved for future Geany-core wiring.");
        });
        actions->addWidget(replaceFilesButton);

        auto *browseButton = new QPushButton("Browse...");
        connect(browseButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Find in Files] Prototype directory browser would open here.");
        });
        actions->addWidget(browseButton);

        auto *classicButton = new QPushButton("Open classic Find in Files");
        connect(classicButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Find in Files] Prototype bridge would open the classic Find in Files dialog.");
        });
        actions->addWidget(classicButton);

        layout->addLayout(actions);
        return page;
    }

    QWidget *createFindInProjectsTab()
    {
        auto *page = new QWidget();
        auto *layout = new QVBoxLayout(page);

        auto *form = new QFormLayout();
        fip_.query = new QComboBox();
        fip_.filters = new QComboBox();
        fip_.directory = new QComboBox();
        fip_.query->setEditable(true);
        fip_.filters->setEditable(true);
        fip_.directory->setEditable(true);
        fip_.directory->setCurrentText("Project Root");
        fip_.directory->setEnabled(false);
        form->addRow("Find what:", fip_.query);
        form->addRow("Filters:", fip_.filters);
        form->addRow("Project root:", fip_.directory);
        layout->addLayout(form);
        layout->addWidget(createModeGroup(fip_.mode));

        auto *options = new QGroupBox("Find in Projects options");
        auto *grid = new QGridLayout(options);
        grid->addWidget(new QCheckBox("Project Panel 1"), 0, 0);
        grid->addWidget(new QCheckBox("Project Panel 2"), 0, 1);
        grid->addWidget(new QCheckBox("Project Panel 3"), 0, 2);
        grid->addWidget(new QCheckBox("Match case"), 1, 0);
        grid->addWidget(new QCheckBox("Whole word"), 1, 1);
        grid->addWidget(new QCheckBox("Recursive"), 1, 2);
        layout->addWidget(options);

        auto *actions = new QHBoxLayout();
        auto *findAllButton = new QPushButton("Find All");
        connect(findAllButton, &QPushButton::clicked, this, [this]() {
            SearchStudioFindInFilesRequest request;
            request.query = textOrPlaceholder(fip_.query->currentText(), "needle");
            request.directory = "Project";
            request.mode = modeName(fip_.mode);
            applyActionResult(SearchStudioBackend::makeFindInFilesResult(request));
        });
        actions->addWidget(findAllButton);

        layout->addLayout(actions);
        return page;
    }

    QWidget *createMarkTab()
    {
        auto *page = new QWidget();
        auto *layout = new QVBoxLayout(page);

        auto *form = new QFormLayout();
        mark_.query = new QComboBox();
        mark_.query->setEditable(true);
        form->addRow("Mark what:", mark_.query);
        layout->addLayout(form);
        layout->addWidget(createModeGroup(mark_.mode));

        auto *options = new QGroupBox("Mark options");
        auto *grid = new QGridLayout(options);
        grid->addWidget(new QCheckBox("Match case"), 0, 0);
        grid->addWidget(new QCheckBox("Whole word"), 0, 1);
        mark_.bookmarkLines = new QCheckBox("Bookmark matching lines");
        mark_.purgeBookmarks = new QCheckBox("Purge existing bookmarks first");
        grid->addWidget(mark_.bookmarkLines, 0, 2);
        grid->addWidget(mark_.purgeBookmarks, 1, 0);
        grid->addWidget(new QCheckBox(". matches newline"), 1, 1);
        grid->addWidget(new QCheckBox("Regular expression"), 1, 2);
        layout->addWidget(options);

        auto *actions = new QHBoxLayout();
        auto *markNowButton = new QPushButton("Mark now");
        connect(markNowButton, &QPushButton::clicked, this, [this]() {
            SearchStudioMarkRequest request;
            request.query = textOrPlaceholder(mark_.query->currentText(), "needle");
            request.mode = modeName(mark_.mode);
            request.bookmarkLines = mark_.bookmarkLines->isChecked();
            request.purgeBookmarks = mark_.purgeBookmarks->isChecked();
            applyActionResult(SearchStudioBackend::makeMarkResult(request,
                qs("Mark Impact"), qs("Mark"), qs("Active Document")));
        });
        actions->addWidget(markNowButton);

        auto *markSessionButton = new QPushButton("Mark Session");
        connect(markSessionButton, &QPushButton::clicked, this, [this]() {
            SearchStudioMarkRequest request;
            request.query = textOrPlaceholder(mark_.query->currentText(), "needle");
            request.mode = modeName(mark_.mode);
            request.sessionScope = true;
            applyActionResult(SearchStudioBackend::makeMarkResult(request,
                qs("Session Mark Impact"), qs("Mark in Session"), qs("Open Documents")));
        });
        actions->addWidget(markSessionButton);

        auto *clearButton = new QPushButton("Clear Marks");
        connect(clearButton, &QPushButton::clicked, this, [this]() {
            applyActionResult(SearchStudioBackend::makeActiveDocumentSummary(
                qs("[Mark] Cleared active-document marks in the prototype."),
                qs("Mark"),
                qs("Clear"),
                qs("N/A"),
                qs("Cleared prototype active-document marks."),
                qs("Mark"),
                qs("Prototype active-document mark state cleared.")));
        });
        actions->addWidget(clearButton);

        auto *clearSessionButton = new QPushButton("Clear Session Marks");
        connect(clearSessionButton, &QPushButton::clicked, this, [this]() {
            applyActionResult(SearchStudioBackend::makeSessionSummary(
                qs("[Mark] Cleared session marks in the prototype."),
                qs("Mark in Session"),
                qs("Clear"),
                qs("N/A"),
                qs("Cleared prototype session marks across open documents."),
                qs("Mark in Session"),
                qs("Prototype session mark state cleared.")));
        });
        actions->addWidget(clearSessionButton);

        layout->addLayout(actions);
        return page;
    }
};

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    SearchStudioDialog dialog;
    dialog.exec();
    return 0;
}
