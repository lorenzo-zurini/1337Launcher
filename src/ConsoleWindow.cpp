#include "ConsoleWindow.h"
#include "GameLauncher.h"

#include <QCloseEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QVBoxLayout>

ConsoleWindow::ConsoleWindow(const QString &title, GameLauncher *launcher)
    : m_launcher(launcher)
{
    launcher->setParent(this);
    setWindowTitle(title);
    resize(800, 500);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(10000);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    m_kill = new QPushButton(tr("Kill game"), this);
    auto *close = new QPushButton(tr("Close"), this);

    auto *buttons = new QHBoxLayout;
    buttons->addStretch();
    buttons->addWidget(m_kill);
    buttons->addWidget(close);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_log);
    layout->addLayout(buttons);

    connect(close, &QPushButton::clicked, this, &QWidget::close);
    connect(m_kill, &QPushButton::clicked, launcher, &GameLauncher::kill);
    connect(launcher, &GameLauncher::output, this, &ConsoleWindow::appendText);
    connect(launcher, &GameLauncher::exited, this, [this](int code) {
        if (!m_running)
            return;
        m_running = false;
        m_kill->setEnabled(false);
        appendText(tr("\nGame exited with code %1.\n").arg(code));
        if (!isVisible())
            deleteLater();
    });
}

void ConsoleWindow::appendText(const QString &text)
{
    QScrollBar *bar = m_log->verticalScrollBar();
    const bool atBottom = bar->value() == bar->maximum();
    m_log->moveCursor(QTextCursor::End);
    m_log->insertPlainText(text);
    if (atBottom)
        bar->setValue(bar->maximum());
}

void ConsoleWindow::closeEvent(QCloseEvent *event)
{
    // Keep the game running in the background; just hide the log.
    event->accept();
    if (!m_running)
        deleteLater();
}
