#include "Account.h"
#include "Paths.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QUuid>

bool Account::needsRefresh() const
{
    if (!isMicrosoft())
        return false;
    return accessToken.isEmpty() || !accessTokenExpiry.isValid()
        || accessTokenExpiry < QDateTime::currentDateTimeUtc().addSecs(10 * 60);
}

QString Account::displayName() const
{
    return isMicrosoft() ? username + QStringLiteral("  (Microsoft)") : username + QStringLiteral("  (Offline)");
}

QJsonObject Account::toJson() const
{
    QJsonObject o;
    o["type"] = isMicrosoft() ? "msa" : "offline";
    o["username"] = username;
    o["uuid"] = uuid;
    if (isMicrosoft()) {
        o["accessToken"] = accessToken;
        o["accessTokenExpiry"] = accessTokenExpiry.toString(Qt::ISODate);
        o["msaRefreshToken"] = msaRefreshToken;
    }
    return o;
}

Account Account::fromJson(const QJsonObject &o)
{
    Account a;
    a.type = o["type"].toString() == "msa" ? Microsoft : Offline;
    a.username = o["username"].toString();
    a.uuid = o["uuid"].toString();
    a.accessToken = o["accessToken"].toString();
    a.accessTokenExpiry = QDateTime::fromString(o["accessTokenExpiry"].toString(), Qt::ISODate);
    a.msaRefreshToken = o["msaRefreshToken"].toString();
    return a;
}

Account Account::offline(const QString &username)
{
    // Same as Java's UUID.nameUUIDFromBytes("OfflinePlayer:" + name), which vanilla servers use.
    QByteArray hash = QCryptographicHash::hash(("OfflinePlayer:" + username).toUtf8(), QCryptographicHash::Md5);
    hash[6] = char((hash[6] & 0x0f) | 0x30);
    hash[8] = char((hash[8] & 0x3f) | 0x80);

    Account a;
    a.type = Offline;
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
        if (m_accounts[i].type == account.type && m_accounts[i].uuid == account.uuid) {
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
    for (const QJsonValue &v : arr)
        m_accounts.append(Account::fromJson(v.toObject()));
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
    if (!file.commit())
        return false;
    // Tokens live in here; keep it private to the user.
    QFile::setPermissions(file.fileName(), QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    return true;
}
