#include "Account.h"
#include "Paths.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>

QJsonObject Account::toJson() const
{
    return QJsonObject{{"username", username}, {"uuid", uuid}};
}

Account Account::fromJson(const QJsonObject &o)
{
    Account a;
    a.username = o["username"].toString();
    a.uuid = o["uuid"].toString();
    return a;
}

Account Account::offline(const QString &username)
{
    // Same as Java's UUID.nameUUIDFromBytes("OfflinePlayer:" + name), which vanilla servers use.
    QByteArray hash = QCryptographicHash::hash(("OfflinePlayer:" + username).toUtf8(), QCryptographicHash::Md5);
    hash[6] = char((hash[6] & 0x0f) | 0x30);
    hash[8] = char((hash[8] & 0x3f) | 0x80);

    Account a;
    a.username = username;
    a.uuid = QUuid::fromRfc4122(hash).toString(QUuid::Id128);
    return a;
}

AccountStore &AccountStore::instance()
{
    static AccountStore store;
    return store;
}

int AccountStore::addOrUpdate(const Account &account)
{
    for (int i = 0; i < m_accounts.size(); ++i) {
        if (m_accounts[i].uuid == account.uuid) {
            m_accounts[i] = account;
            save();
            return i;
        }
    }
    m_accounts.append(account);
    save();
    return m_accounts.size() - 1;
}

void AccountStore::remove(int index)
{
    if (index < 0 || index >= m_accounts.size())
        return;
    m_accounts.removeAt(index);
    save();
}

void AccountStore::load()
{
    m_accounts.clear();
    QFile file(Paths::data() + "/accounts.json");
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonArray arr = QJsonDocument::fromJson(file.readAll()).object()["accounts"].toArray();
    for (const QJsonValue &v : arr) {
        const Account a = Account::fromJson(v.toObject());
        if (!a.username.isEmpty() && !a.uuid.isEmpty())
            m_accounts.append(a);
    }
}

bool AccountStore::save() const
{
    QJsonArray arr;
    for (const Account &a : m_accounts)
        arr.append(a.toJson());
    QJsonObject root;
    root["formatVersion"] = 1;
    root["accounts"] = arr;

    QSaveFile file(Paths::data() + "/accounts.json");
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(root).toJson());
    return file.commit();
}
