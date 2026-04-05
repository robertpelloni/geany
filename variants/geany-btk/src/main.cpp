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
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

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
        NavigableRole
    };

    QTabWidget *topTabs_ = nullptr;
    QTabWidget *lowerTabs_ = nullptr;
    QPlainTextEdit *activityView_ = nullptr;
    QTreeWidget *resultsView_ = nullptr;
    QPlainTextEdit *previewView_ = nullptr;

    FindControls find_;
    ReplaceControls replace_;
    FindInFilesControls fif_;
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

    QStringList prototypeDocuments() const
    {
        return {"src/search.c", "src/document.c", "src/prefs.c", "data/geany.glade"};
    }

    int sampleCountFor(const QString &query, int index) const
    {
        const int base = query.trimmed().isEmpty() ? 2 : query.trimmed().size();
        return base + index + 1;
    }

    int sampleLineFor(const QString &query, int index) const
    {
        const int base = query.trimmed().isEmpty() ? 12 : query.trimmed().size() * 3;
        return base + (index * 17);
    }

    QString buildLinePreview(const QString &kind, const QString &file, const QString &query,
        int line, const QString &context) const
    {
        return QString(
            "%1\n\n"
            "File: %2\n"
            "Query: %3\n"
            "Line: %4\n\n"
            "Context:\n%5")
            .formatArgs(kind, file, query, QString::number(line), context);
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

    void appendImpactRows(const QString &action, const QString &query, const QString &mode,
        const QString &summaryPrefix, bool sessionScope)
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
            appendResult(
                action,
                QString("%1:%2").formatArg(file).formatArg(line),
                query,
                mode,
                QString("%1 %2 matches in this document.").formatArg(summaryPrefix).formatArg(count),
                QString("%1 — %2:%3").formatArgs(action, file).formatArg(line),
                buildLinePreview(QString("%1 impact summary").formatArg(action), file, query, line, context),
                true);
        }
    }

    void appendReplacePreviewRows(const QString &action, const QString &query,
        const QString &replaceText, const QString &mode, bool sessionScope)
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

            appendResult(
                action,
                QString("%1:%2").formatArg(file).formatArg(line),
                query,
                mode,
                QString("Would replace the representative match on line %1.").formatArg(line),
                QString("%1 — %2:%3").formatArgs(action, file).formatArg(line),
                previewBody,
                true);
        }
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
                appendActivity(QString("[Find] %1 | query=%2 | mode=%3").formatArgs(activity, query, mode));
                appendResult(
                    activity,
                    "Active Document",
                    query,
                    mode,
                    summary,
                    QString("%1 — Active Document").formatArg(activity),
                    QString("Prototype action: %1\n\nQuery: %2\nMode: %3\n\nThis BTK surface mirrors the current Search Studio workflow but is not yet wired to Geany core services.")
                        .formatArgs(activity, query, mode),
                    false);
            });
            actions->addWidget(button);
        };

        addFindButton("Find Next", "Find", "Found next occurrence in the active document.");
        addFindButton("Find Previous", "Find Previous", "Found previous occurrence in the active document.");

        auto *countButton = new QPushButton("Count");
        connect(countButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(find_.query->currentText(), "needle");
            const QString mode = modeName(find_.mode);
            appendActivity(QString("[Count] query=%1 | mode=%2 | scope=active document").formatArgs(query, mode));
            appendResult(
                "Count",
                "Active Document",
                query,
                mode,
                "Counted matches in the active document.",
                "Count — Active Document",
                "Prototype count summary for the active document.",
                false);
            appendImpactRows("Count Impact", query, mode, "Counted", false);
        });
        actions->addWidget(countButton);

        auto *countSessionButton = new QPushButton("Count Session");
        connect(countSessionButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(find_.query->currentText(), "needle");
            const QString mode = modeName(find_.mode);
            appendActivity(QString("[Count] Session | query=%1 | mode=%2").formatArgs(query, mode));
            appendImpactRows("Session Count Impact", query, mode, "Counted", true);
            appendResult(
                "Count in Session",
                "Open Documents",
                query,
                mode,
                "Counted representative matches across open documents.",
                "Count in Session",
                "Prototype aggregate count across open documents with per-document impact rows above.",
                false);
        });
        actions->addWidget(countSessionButton);

        auto *markButton = new QPushButton("Mark / Bookmark");
        connect(markButton, &QPushButton::clicked, this, [this]() {
            topTabs_->setCurrentIndex(3);
            appendActivity("[Find] Routed to Mark workflow from the Find tab.");
        });
        actions->addWidget(markButton);

        auto *collectDocButton = new QPushButton("Collect Document Hits");
        connect(collectDocButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(find_.query->currentText(), "needle");
            const QString mode = modeName(find_.mode);
            appendActivity(QString("[Results] Collected current-document hits for %1.").formatArg(query));
            appendImpactRows("Document Hit", query, mode, "Collected", false);
        });
        actions->addWidget(collectDocButton);

        auto *collectSessionButton = new QPushButton("Collect Session Hits");
        connect(collectSessionButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(find_.query->currentText(), "needle");
            const QString mode = modeName(find_.mode);
            appendActivity(QString("[Results] Collected open-document hits for %1.").formatArg(query));
            appendImpactRows("Session Hit", query, mode, "Collected", true);
            appendResult(
                "Collect Session Hits",
                "Open Documents",
                query,
                mode,
                "Collected representative open-document hits.",
                "Collect Session Hits",
                "Prototype open-document hit collection summary.",
                false);
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
        form->addRow("Find what:", replace_.query);
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
                appendActivity(QString("[Replace] %1 | query=%2 | replacement=%3 | mode=%4")
                    .formatArgs(activity, query, replacement, mode));
                appendResult(
                    activity,
                    "Active Document",
                    query,
                    mode,
                    summary,
                    QString("%1 — Active Document").formatArg(activity),
                    QString("Query: %1\nReplacement: %2\nMode: %3\n\nPrototype action coverage only; engine wiring comes next.")
                        .formatArgs(query, replacement, mode),
                    false);
            });
            actions->addWidget(button);
        };

        addReplaceButton("Find Next", "Replace/Find", "Found the next candidate match from the Replace tab.");
        addReplaceButton("Replace", "Replace", "Replaced the current match in the active document.");
        addReplaceButton("Replace & Find", "Replace & Find", "Replaced the current match and advanced to the next one.");

        auto *replaceDocButton = new QPushButton("Replace in Document");
        connect(replaceDocButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(replace_.query->currentText(), "needle");
            const QString replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            const QString mode = modeName(replace_.mode);
            appendActivity(QString("[Replace] Replace in document | query=%1 | replacement=%2 | mode=%3")
                .formatArgs(query, replacement, mode));
            appendReplacePreviewRows("Replace Impact", query, replacement, mode, false);
            appendResult(
                "Replace in Document",
                "Active Document",
                query,
                mode,
                "Prototype replace-in-document summary with impact rows above.",
                "Replace in Document",
                QString("Replacement payload: %1\nMode: %2\n\nImpact rows above mirror the current Search Studio model.")
                    .formatArgs(replacement, mode),
                false);
        });
        actions->addWidget(replaceDocButton);

        auto *replaceSessionButton = new QPushButton("Replace in Session");
        connect(replaceSessionButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(replace_.query->currentText(), "needle");
            const QString replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            const QString mode = modeName(replace_.mode);
            appendActivity(QString("[Replace] Replace in session | query=%1 | replacement=%2 | mode=%3")
                .formatArgs(query, replacement, mode));
            appendReplacePreviewRows("Session Replace Impact", query, replacement, mode, true);
            appendResult(
                "Replace in Session",
                "Open Documents",
                query,
                mode,
                "Prototype replace-in-session summary with per-document impact rows above.",
                "Replace in Session",
                QString("Replacement payload: %1\nMode: %2\n\nPer-document impact rows above would be driven by Geany core later.")
                    .formatArgs(replacement, mode),
                false);
        });
        actions->addWidget(replaceSessionButton);

        auto *previewDocButton = new QPushButton("Preview in Document");
        connect(previewDocButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(replace_.query->currentText(), "needle");
            const QString replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            appendActivity("[Replace Preview] Prototype document preview generated.");
            appendReplacePreviewRows("Replace Preview", query, replacement, modeName(replace_.mode), false);
        });
        actions->addWidget(previewDocButton);

        auto *previewSessionButton = new QPushButton("Preview in Session");
        connect(previewSessionButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(replace_.query->currentText(), "needle");
            const QString replacement = textOrPlaceholder(replace_.replacement->currentText(), "replacement");
            appendActivity("[Replace Preview] Prototype session preview generated.");
            appendReplacePreviewRows("Replace Preview Session", query, replacement, modeName(replace_.mode), true);
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
        grid->addWidget(new QCheckBox("Match case"), 0, 1);
        grid->addWidget(new QCheckBox("Whole word"), 0, 2);
        grid->addWidget(new QCheckBox("Invert match"), 1, 0);
        grid->addWidget(new QCheckBox("Use project patterns"), 1, 1);
        grid->addWidget(new QCheckBox("Use extra grep options"), 1, 2);
        layout->addWidget(options);

        auto *actions = new QHBoxLayout();
        auto *findAllButton = new QPushButton("Find All");
        connect(findAllButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(fif_.query->currentText(), "needle");
            const QString directory = textOrPlaceholder(fif_.directory->currentText(), ".");
            const QString mode = modeName(fif_.mode);
            appendActivity(QString("[Find in Files] query=%1 | directory=%2 | mode=%3").formatArgs(query, directory, mode));
            appendImpactRows("Find in Files Hit", query, mode, "Captured", true);
            appendResult(
                "Find in Files",
                directory,
                query,
                mode,
                "Prototype directory search launched with structured hit ingestion.",
                "Find in Files",
                QString("Directory: %1\nMode: %2\n\nResults above mirror a future structured ripgrep-style ingestion path.")
                    .formatArgs(directory, mode),
                false);
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
            const QString query = textOrPlaceholder(mark_.query->currentText(), "needle");
            const QString mode = modeName(mark_.mode);
            const QString bookmark = mark_.bookmarkLines->isChecked() ? qs("on") : qs("off");
            const QString purge = mark_.purgeBookmarks->isChecked() ? qs("on") : qs("off");
            appendActivity(QString("[Mark] query=%1 | mode=%2 | bookmarks=%3 | purge=%4")
                .formatArgs(query, mode, bookmark, purge));
            appendImpactRows("Mark Impact", query, mode, "Marked", false);
            appendResult(
                "Mark",
                "Active Document",
                query,
                mode,
                QString("Marked representative matches; bookmark-lines=%1; purge-first=%2.")
                    .formatArgs(bookmark, purge),
                "Mark",
                "Prototype active-document mark summary.",
                false);
        });
        actions->addWidget(markNowButton);

        auto *markSessionButton = new QPushButton("Mark Session");
        connect(markSessionButton, &QPushButton::clicked, this, [this]() {
            const QString query = textOrPlaceholder(mark_.query->currentText(), "needle");
            const QString mode = modeName(mark_.mode);
            appendActivity(QString("[Mark] Session | query=%1 | mode=%2").formatArgs(query, mode));
            appendImpactRows("Session Mark Impact", query, mode, "Marked", true);
            appendResult(
                "Mark in Session",
                "Open Documents",
                query,
                mode,
                "Prototype session mark summary with per-document impact rows above.",
                "Mark in Session",
                "Session mark impact rows above mirror the matured Search Studio behavior from the main tree.",
                false);
        });
        actions->addWidget(markSessionButton);

        auto *clearButton = new QPushButton("Clear Marks");
        connect(clearButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Mark] Cleared active-document marks in the prototype.");
            appendResult(
                "Mark",
                "Active Document",
                "Clear",
                "N/A",
                "Cleared prototype active-document marks.",
                "Mark",
                "Prototype active-document mark state cleared.",
                false);
        });
        actions->addWidget(clearButton);

        auto *clearSessionButton = new QPushButton("Clear Session Marks");
        connect(clearSessionButton, &QPushButton::clicked, this, [this]() {
            appendActivity("[Mark] Cleared session marks in the prototype.");
            appendResult(
                "Mark in Session",
                "Open Documents",
                "Clear",
                "N/A",
                "Cleared prototype session marks across open documents.",
                "Mark in Session",
                "Prototype session mark state cleared.",
                false);
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
