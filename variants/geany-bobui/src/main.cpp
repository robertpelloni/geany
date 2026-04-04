#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>

namespace {

QWidget *makeFindTab()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);

    auto *form = new QFormLayout();
    auto *findWhat = new QComboBox();
    findWhat->setEditable(true);
    form->addRow("Find what:", findWhat);
    layout->addLayout(form);

    auto *searchMode = new QGroupBox("Search mode");
    auto *searchModeLayout = new QHBoxLayout(searchMode);
    searchModeLayout->addWidget(new QCheckBox("Normal"));
    searchModeLayout->addWidget(new QCheckBox("Extended"));
    searchModeLayout->addWidget(new QCheckBox("Regular expression"));
    layout->addWidget(searchMode);

    auto *filters = new QGroupBox("Find options");
    auto *grid = new QGridLayout(filters);
    grid->addWidget(new QCheckBox("Match case"), 0, 0);
    grid->addWidget(new QCheckBox("Match whole word only"), 0, 1);
    grid->addWidget(new QCheckBox("Wrap around"), 0, 2);
    grid->addWidget(new QCheckBox(". matches newline"), 1, 0);
    grid->addWidget(new QCheckBox("In selection"), 1, 1);
    grid->addWidget(new QCheckBox("Backward direction"), 1, 2);
    layout->addWidget(filters);

    auto *actions = new QHBoxLayout();
    actions->addWidget(new QPushButton("Find Next"));
    actions->addWidget(new QPushButton("Find Previous"));
    actions->addWidget(new QPushButton("Count"));
    actions->addWidget(new QPushButton("Find All in Current Document"));
    actions->addWidget(new QPushButton("Find All in All Open Documents"));
    layout->addLayout(actions);

    auto *preview = new QPlainTextEdit();
    preview->setPlaceholderText("Future live preview / result summary pane...");
    layout->addWidget(preview, 1);

    return page;
}

QWidget *makeReplaceTab()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);

    auto *form = new QFormLayout();
    auto *findWhat = new QComboBox();
    auto *replaceWith = new QComboBox();
    findWhat->setEditable(true);
    replaceWith->setEditable(true);
    form->addRow("Find what:", findWhat);
    form->addRow("Replace with:", replaceWith);
    layout->addLayout(form);

    auto *filters = new QGroupBox("Replace options");
    auto *grid = new QGridLayout(filters);
    grid->addWidget(new QCheckBox("Match case"), 0, 0);
    grid->addWidget(new QCheckBox("Match whole word only"), 0, 1);
    grid->addWidget(new QCheckBox("Wrap around"), 0, 2);
    grid->addWidget(new QCheckBox(". matches newline"), 1, 0);
    grid->addWidget(new QCheckBox("In selection"), 1, 1);
    grid->addWidget(new QCheckBox("Backward direction"), 1, 2);
    layout->addWidget(filters);

    auto *actions = new QHBoxLayout();
    actions->addWidget(new QPushButton("Find Next"));
    actions->addWidget(new QPushButton("Replace"));
    actions->addWidget(new QPushButton("Replace All"));
    actions->addWidget(new QPushButton("Replace All in Open Documents"));
    layout->addLayout(actions);

    auto *preview = new QPlainTextEdit();
    preview->setPlaceholderText("Future replacement diff preview...");
    layout->addWidget(preview, 1);

    return page;
}

QWidget *makeFindInFilesTab()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);

    auto *form = new QFormLayout();
    auto *findWhat = new QComboBox();
    auto *filters = new QComboBox();
    auto *directory = new QComboBox();
    findWhat->setEditable(true);
    filters->setEditable(true);
    directory->setEditable(true);
    form->addRow("Find what:", findWhat);
    form->addRow("Filters:", filters);
    form->addRow("Directory:", directory);
    layout->addLayout(form);

    auto *scope = new QGroupBox("Scope");
    auto *scopeGrid = new QGridLayout(scope);
    scopeGrid->addWidget(new QCheckBox("Recursive"), 0, 0);
    scopeGrid->addWidget(new QCheckBox("Include hidden folders"), 0, 1);
    scopeGrid->addWidget(new QCheckBox("Project panel 1"), 1, 0);
    scopeGrid->addWidget(new QCheckBox("Project panel 2"), 1, 1);
    scopeGrid->addWidget(new QCheckBox("Project panel 3"), 1, 2);
    layout->addWidget(scope);

    auto *modes = new QGroupBox("Search mode");
    auto *modeLayout = new QHBoxLayout(modes);
    modeLayout->addWidget(new QCheckBox("Normal"));
    modeLayout->addWidget(new QCheckBox("Extended"));
    modeLayout->addWidget(new QCheckBox("Regular expression"));
    modeLayout->addWidget(new QCheckBox("Match case"));
    modeLayout->addWidget(new QCheckBox("Whole word"));
    modeLayout->addWidget(new QCheckBox("Invert match"));
    layout->addWidget(modes);

    auto *actions = new QHBoxLayout();
    actions->addWidget(new QPushButton("Find All"));
    actions->addWidget(new QPushButton("Replace in Files"));
    actions->addWidget(new QPushButton("Browse..."));
    layout->addLayout(actions);

    auto *preview = new QPlainTextEdit();
    preview->setPlaceholderText("Future search results / ripgrep-style stream view...");
    layout->addWidget(preview, 1);

    return page;
}

QWidget *makeMarkTab()
{
    auto *page = new QWidget();
    auto *layout = new QVBoxLayout(page);

    auto *form = new QFormLayout();
    auto *findWhat = new QComboBox();
    findWhat->setEditable(true);
    form->addRow("Mark what:", findWhat);
    layout->addLayout(form);

    auto *options = new QGroupBox("Mark options");
    auto *grid = new QGridLayout(options);
    grid->addWidget(new QCheckBox("Match case"), 0, 0);
    grid->addWidget(new QCheckBox("Whole word"), 0, 1);
    grid->addWidget(new QCheckBox("Bookmark line"), 0, 2);
    grid->addWidget(new QCheckBox("Purge for each search"), 1, 0);
    grid->addWidget(new QCheckBox(". matches newline"), 1, 1);
    grid->addWidget(new QCheckBox("Regular expression"), 1, 2);
    layout->addWidget(options);

    auto *actions = new QHBoxLayout();
    actions->addWidget(new QPushButton("Mark All"));
    actions->addWidget(new QPushButton("Clear Marks"));
    actions->addWidget(new QPushButton("Copy Marked Text"));
    layout->addLayout(actions);

    auto *preview = new QPlainTextEdit();
    preview->setPlaceholderText("Future mark summary / bookmarked line preview...");
    layout->addWidget(preview, 1);

    return page;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    QDialog dialog;
    dialog.setWindowTitle("Geany Search Studio (BobUI Experimental)");
    dialog.resize(1100, 760);

    auto *layout = new QVBoxLayout(&dialog);

    auto *hero = new QLabel(
        "<h2>Geany Search Studio</h2>"
        "<p>Experimental BobUI/Qt-native search surface inspired by Notepad++ and designed to go further with previews, scopes, and richer workflows.</p>");
    hero->setWordWrap(true);
    layout->addWidget(hero);

    auto *tabs = new QTabWidget();
    tabs->addTab(makeFindTab(), "Find");
    tabs->addTab(makeReplaceTab(), "Replace");
    tabs->addTab(makeFindInFilesTab(), "Find in Files");
    tabs->addTab(makeMarkTab(), "Mark");
    layout->addWidget(tabs, 1);

    auto *footer = new QHBoxLayout();
    auto *closeButton = new QPushButton("Close");
    footer->addWidget(new QLabel("Prototype status: UI exploration / control coverage first, engine wiring next."));
    footer->addStretch(1);
    footer->addWidget(closeButton, 0, Qt::AlignRight);
    QObject::connect(closeButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    layout->addLayout(footer);

    dialog.exec();
    return 0;
}
