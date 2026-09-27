#pragma once

#include <QWidget>

class GameLauncher;
class QPlainTextEdit;
class QPushButton;

// Shows the game's output. Owns the launcher; deletes itself once the game
// has exited and the window is closed.
class ConsoleWindow : public QWidget {
    Q_OBJECT
public:
    ConsoleWindow(const QString &title, GameLauncher *launcher);

    bool isRunning() const { return m_running; }
    void appendText(const QString &text);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    GameLauncher *m_launcher;
    QPlainTextEdit *m_log;
    QPushButton *m_kill;
    bool m_running = true;
};
