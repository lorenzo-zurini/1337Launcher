#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>

struct Account {
    enum Type { Offline, Microsoft };

    Type type = Offline;
    QString username;
    QString uuid;                 // without dashes
    QString accessToken;          // Minecraft access token
    QDateTime accessTokenExpiry;
    QString msaRefreshToken;      // Microsoft OAuth refresh token

    bool isMicrosoft() const { return type == Microsoft; }
    bool needsRefresh() const;
    QString displayName() const;

    QJsonObject toJson() const;
    static Account fromJson(const QJsonObject &obj);
    static Account offline(const QString &username);
};

class AccountStore {
public:
    static AccountStore &instance();

    const QList<Account> &accounts() const { return m_accounts; }
    int addOrUpdate(const Account &account); // returns index
    void remove(int index);

    void load();
    bool save() const;

private:
    QList<Account> m_accounts;
};
