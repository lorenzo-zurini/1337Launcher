#include "MainWindow.h"
#include "ConsoleWindow.h"
#include "GameInstaller.h"
#include "GameLauncher.h"
#include "LoginDialog.h"
#include "MicrosoftAuth.h"
#include "NewInstanceDialog.h"
#include "Paths.h"
#include "SettingsDialog.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QDesktopServices>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QUrl>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("1337Launcher " LAUNCHER_VERSION));
    resize(440, 620);

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *logo = new QLabel(central);
    QPixmap pixmap(":/logo");
    logo->setPixmap(pixmap.scaledToWidth(400, Qt::SmoothTransformation));
    logo->setAlignment(Qt::AlignCenter);

    // Instances
    auto *instanceBox = new QGroupBox(tr("Instances"), central);
    m_instances = new QListWidget(instanceBox);
    m_newButton = new QPushButton(tr("New..."), instanceBox);
    m_deleteButton = new QPushButton(tr("Delete"), instanceBox);
    m_folderButton = new QPushButton(tr("Open folder"), instanceBox);
    auto *instanceButtons = new QHBoxLayout;
    instanceButtons->addWidget(m_newButton);
    instanceButtons->addWidget(m_folderButton);
    instanceButtons->addWidget(m_deleteButton);
    auto *instanceLayout = new QVBoxLayout(instanceBox);
    instanceLayout->addWidget(m_instances);
    instanceLayout->addLayout(instanceButtons);

    // Accounts
    auto *accountBox = new QGroupBox(tr("Account"), central);
    m_accounts = new QComboBox(accountBox);
    m_addMsButton = new QPushButton(tr("Add Microsoft..."), accountBox);
    m_addOfflineButton = new QPushButton(tr("Add offline..."), accountBox);
    m_removeAccountButton = new QPushButton(tr("Remove"), accountBox);
    auto *accountButtons = new QHBoxLayout;
    accountButtons->addWidget(m_addMsButton);
    accountButtons->addWidget(m_addOfflineButton);
    accountButtons->addWidget(m_removeAccountButton);
    auto *accountLayout = new QVBoxLayout(accountBox);
    accountLayout->addWidget(m_accounts);
    accountLayout->addLayout(accountButtons);

    // Bottom
    m_progress = new QProgressBar(central);
    m_progress->setTextVisible(true);
    m_progress->hide();
    m_status = new QLabel(central);
    m_status->setWordWrap(true);
    m_settingsButton = new QPushButton(tr("Settings"), central);
    m_playButton = new QPushButton(tr("PLAY"), central);
    QFont playFont = m_playButton->font();
    playFont.setBold(true);
    playFont.setPointSize(playFont.pointSize() + 3);
    m_playButton->setFont(playFont);
    m_playButton->setMinimumHeight(44);
    m_playButton->setDefault(true);

    auto *bottom = new QHBoxLayout;
    bottom->addWidget(m_settingsButton);
    bottom->addWidget(m_playButton, 1);

    auto *layout = new QVBoxLayout(central);
    layout->addWidget(logo);
    layout->addWidget(instanceBox, 1);
    layout->addWidget(accountBox);
    layout->addWidget(m_progress);
    layout->addWidget(m_status);
    layout->addLayout(bottom);

    connect(m_newButton, &QPushButton::clicked, this, &MainWindow::newInstance);
    connect(m_deleteButton, &QPushButton::clicked, this, &MainWindow::deleteInstance);
    connect(m_folderButton, &QPushButton::clicked, this, &MainWindow::openInstanceFolder);
    connect(m_addMsButton, &QPushButton::clicked, this, &MainWindow::addMicrosoftAccount);
    connect(m_addOfflineButton, &QPushButton::clicked, this, &MainWindow::addOfflineAccount);
    connect(m_removeAccountButton, &QPushButton::clicked, this, &MainWindow::removeAccount);
    connect(m_settingsButton, &QPushButton::clicked, this, &MainWindow::openSettings);
    connect(m_playButton, &QPushButton::clicked, this, &MainWindow::play);
    connect(m_instances, &QListWidget::currentRowChanged, this, &MainWindow::updateButtons);
    connect(m_instances, &QListWidget::itemDoubleClicked, this, &MainWindow::play);
    connect(m_accounts, &QComboBox::currentIndexChanged, this, [this](int index) {
        if (index >= 0)
            Paths::settings().setValue("ui/lastAccount", index);
        updateButtons();
    });

    reloadInstances(Paths::settings().value("ui/lastInstance").toString());
    reloadAccounts(Paths::settings().value("ui/lastAccount", 0).toInt());
}

void MainWindow::reloadInstances(const QString &select)
{
    m_instanceList = Instance::loadAll();
    m_instances->clear();
    int selectRow = m_instanceList.isEmpty() ? -1 : 0;
    for (int i = 0; i < m_instanceList.size(); ++i) {
        const Instance &inst = m_instanceList[i];
        m_instances->addItem(QString("%1   —   Minecraft %2").arg(inst.name, inst.versionId));
        if (inst.name == select)
            selectRow = i;
    }
    m_instances->setCurrentRow(selectRow);
    updateButtons();
}

void MainWindow::reloadAccounts(int select)
{
    const QList<Account> &accounts = AccountStore::instance().accounts();
    m_accounts->blockSignals(true);
    m_accounts->clear();
    for (const Account &a : accounts)
        m_accounts->addItem(a.displayName());
    if (accounts.isEmpty())
        m_accounts->setPlaceholderText(tr("No accounts — add one below"));
    m_accounts->setCurrentIndex(accounts.isEmpty() ? -1 : qBound(0, select, int(accounts.size()) - 1));
    m_accounts->blockSignals(false);
    if (m_accounts->currentIndex() >= 0)
        Paths::settings().setValue("ui/lastAccount", m_accounts->currentIndex());
    updateButtons();
}

void MainWindow::updateButtons()
{
    const bool hasInstance = m_instances->currentRow() >= 0;
    const bool hasAccount = m_accounts->currentIndex() >= 0;
    m_newButton->setEnabled(!m_busy);
    m_deleteButton->setEnabled(!m_busy && hasInstance);
    m_folderButton->setEnabled(hasInstance);
    m_instances->setEnabled(!m_busy);
    m_accounts->setEnabled(!m_busy);
    m_addMsButton->setEnabled(!m_busy);
    m_addOfflineButton->setEnabled(!m_busy);
    m_removeAccountButton->setEnabled(!m_busy && hasAccount);
    m_settingsButton->setEnabled(!m_busy);
    m_playButton->setEnabled(m_busy || (hasInstance && hasAccount));
    m_playButton->setText(m_busy ? tr("Cancel") : tr("PLAY"));
}

void MainWindow::setBusy(bool busy, const QString &status)
{
    m_busy = busy;
    m_status->setText(status);
    if (!busy)
        m_progress->hide();
    updateButtons();
}

void MainWindow::newInstance()
{
    NewInstanceDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    const Instance inst = dialog.instance();
    if (!inst.save()) {
        QMessageBox::warning(this, tr("Error"), tr("Could not create the instance folder."));
        return;
    }
    reloadInstances(inst.name);
}

void MainWindow::deleteInstance()
{
    const int row = m_instances->currentRow();
    if (row < 0)
        return;
    const Instance inst = m_instanceList[row];
    const auto answer = QMessageBox::question(
        this, tr("Delete instance"),
        tr("Delete \"%1\" including its worlds, mods and settings?\n\nThis cannot be undone.").arg(inst.name));
    if (answer != QMessageBox::Yes)
        return;
    inst.remove();
    reloadInstances();
}

void MainWindow::openInstanceFolder()
{
    const int row = m_instances->currentRow();
    if (row < 0)
        return;
    QDir().mkpath(m_instanceList[row].gameDir());
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_instanceList[row].gameDir()));
}

void MainWindow::addMicrosoftAccount()
{
    LoginDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    reloadAccounts(AccountStore::instance().addOrUpdate(dialog.account()));
}

void MainWindow::addOfflineAccount()
{
    bool ok = false;
    const QString name = QInputDialog::getText(this, tr("Offline account"),
                                               tr("Player name (3-16 characters: letters, numbers, _):"),
                                               QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok)
        return;
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9_]{3,16}$"));
    if (!re.match(name).hasMatch()) {
        QMessageBox::warning(this, tr("Invalid name"), tr("\"%1\" is not a valid player name.").arg(name));
        return;
    }
    reloadAccounts(AccountStore::instance().addOrUpdate(Account::offline(name)));
}

void MainWindow::removeAccount()
{
    const int index = m_accounts->currentIndex();
    if (index < 0)
        return;
    AccountStore::instance().remove(index);
    reloadAccounts(index - 1);
}

void MainWindow::openSettings()
{
    SettingsDialog dialog(this);
    dialog.exec();
}

void MainWindow::play()
{
    if (m_busy) {
        // Acts as a cancel button while busy.
        if (m_auth) {
            m_auth->disconnect(this);
            m_auth->cancel();
            m_auth->deleteLater();
        }
        if (m_installer) {
            m_installer->disconnect(this);
            m_installer->cancel();
            m_installer->deleteLater();
        }
        setBusy(false, tr("Cancelled."));
        return;
    }

    const int row = m_instances->currentRow();
    const int accountIndex = m_accounts->currentIndex();
    if (row < 0 || accountIndex < 0)
        return;
    const Instance instance = m_instanceList[row];
    const Account account = AccountStore::instance().accounts().at(accountIndex);
    Paths::settings().setValue("ui/lastInstance", instance.name);

    if (!account.needsRefresh())
        return install(instance, account);

    setBusy(true, tr("Refreshing login..."));
    auto *auth = new MicrosoftAuth(this);
    m_auth = auth;
    connect(auth, &MicrosoftAuth::status, m_status, &QLabel::setText);
    connect(auth, &MicrosoftAuth::succeeded, this, [this, auth, instance](const Account &refreshed) {
        auth->deleteLater();
        const int index = AccountStore::instance().addOrUpdate(refreshed);
        reloadAccounts(index);
        install(instance, refreshed);
    });
    connect(auth, &MicrosoftAuth::failed, this, [this, auth](const QString &error) {
        auth->deleteLater();
        setBusy(false, tr("Login failed."));
        QMessageBox::warning(this, tr("Login failed"), error);
    });
    auth->refresh(account);
}

void MainWindow::install(const Instance &instance, const Account &account)
{
    setBusy(true, tr("Preparing %1...").arg(instance.name));
    auto *installer = new GameInstaller(instance, this);
    m_installer = installer;
    connect(installer, &GameInstaller::status, m_status, &QLabel::setText);
    connect(installer, &GameInstaller::progress, this, [this](int done, int total) {
        m_progress->setMaximum(total);
        m_progress->setValue(done);
        m_progress->setFormat(tr("%v / %m files"));
        m_progress->show();
    });
    connect(installer, &GameInstaller::finished, this, [this, installer, instance, account](bool ok, const QString &error) {
        installer->deleteLater();
        if (!ok) {
            setBusy(false, tr("Failed."));
            QMessageBox::warning(this, tr("Could not prepare the game"), error);
            return;
        }
        launch(instance, account, installer);
    });
    installer->start();
}

void MainWindow::launch(const Instance &instance, const Account &account, GameInstaller *installer)
{
    auto *launcher = new GameLauncher(instance, account, installer->version(), installer->javaPath());
    auto *console = new ConsoleWindow(tr("%1 — Minecraft %2").arg(instance.name, instance.versionId), launcher);
    QString error;
    if (!launcher->start(&error)) {
        delete console;
        setBusy(false, tr("Failed."));
        QMessageBox::warning(this, tr("Could not launch the game"), error);
        return;
    }
    m_consoles.append(console);
    console->show();
    setBusy(false, tr("Launched %1 as %2.").arg(instance.name, account.username));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    int running = 0;
    for (const auto &console : std::as_const(m_consoles)) {
        if (console && console->isRunning())
            ++running;
    }
    if (running > 0) {
        const auto answer = QMessageBox::question(
            this, tr("Game running"),
            tr("Minecraft is still running. Closing the launcher will stop it. Close anyway?"));
        if (answer != QMessageBox::Yes) {
            event->ignore();
            return;
        }
    }
    for (const auto &console : std::as_const(m_consoles))
        delete console.data();
    event->accept();
}
