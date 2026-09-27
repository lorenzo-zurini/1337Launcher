#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

// An offline player profile: just a name and the UUID derived from it.
struct Account {
    QString username;
    QString uuid; // without dashes

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
