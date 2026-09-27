#pragma once

#include "Account.h"
#include "Instance.h"

#include <QJsonObject>
#include <QObject>
#include <QProcess>

// Builds the command line for a version and runs the game.
class GameLauncher : public QObject {
    Q_OBJECT
public:
    GameLauncher(const Instance &instance, const Account &account, const QJsonObject &version,
                 const QString &javaPath, QObject *parent = nullptr);

    bool start(QString *error);
    void kill();
    bool isRunning() const { return m_process.state() != QProcess::NotRunning; }

signals:
    void output(const QString &text);
    void exited(int exitCode);

private:
    bool extractNatives(QString *error);
    QStringList buildArguments();
    QStringList expand(const QJsonArray &args, const QHash<QString, QString> &vars) const;

    Instance m_instance;
    Account m_account;
    QJsonObject m_version;
    QString m_javaPath;
    QProcess m_process;
};
