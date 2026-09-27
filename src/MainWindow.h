#pragma once

#include "Account.h"
#include "Instance.h"

#include <QMainWindow>
#include <QPointer>

class ConsoleWindow;
class GameInstaller;
class MicrosoftAuth;
class QComboBox;
class QLabel;
class QListWidget;
class QProgressBar;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void reloadInstances(const QString &select = {});
    void reloadAccounts(int select = -1);
    void updateButtons();
    void setBusy(bool busy, const QString &status = {});

    void newInstance();
    void deleteInstance();
    void openInstanceFolder();
    void addMicrosoftAccount();
    void addOfflineAccount();
    void removeAccount();
    void openSettings();

    void play();
    void install(const Instance &instance, const Account &account);
    void launch(const Instance &instance, const Account &account, GameInstaller *installer);

    QListWidget *m_instances;
    QPushButton *m_newButton;
    QPushButton *m_deleteButton;
    QPushButton *m_folderButton;
    QComboBox *m_accounts;
    QPushButton *m_addMsButton;
    QPushButton *m_addOfflineButton;
    QPushButton *m_removeAccountButton;
    QProgressBar *m_progress;
    QLabel *m_status;
    QPushButton *m_settingsButton;
    QPushButton *m_playButton;

    QList<Instance> m_instanceList;
    bool m_busy = false;
    QPointer<GameInstaller> m_installer;
    QPointer<MicrosoftAuth> m_auth;
    QList<QPointer<ConsoleWindow>> m_consoles;
};
