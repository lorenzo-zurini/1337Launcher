#pragma once

#include "Account.h"

#include <QObject>
#include <QTimer>

// Microsoft account login for Minecraft:
//   OAuth 2.0 device code flow (or refresh token)
//   -> Xbox Live user token -> XSTS token -> Minecraft access token -> profile.
class MicrosoftAuth : public QObject {
    Q_OBJECT
public:
    explicit MicrosoftAuth(QObject *parent = nullptr);

    static QString clientId();

    // Interactive login; emits deviceCodeReady() and then succeeded()/failed().
    void startDeviceLogin();
    // Non-interactive: refresh an existing Microsoft account's tokens.
    void refresh(const Account &account);
    void cancel();

signals:
    void deviceCodeReady(const QString &userCode, const QString &verificationUri);
    void status(const QString &message);
    void succeeded(const Account &account);
    void failed(const QString &error);

private:
    void pollDeviceCode();
    void onMsaToken(const QJsonObject &json);
    void xboxLiveAuth(const QString &msaAccessToken);
    void xstsAuth(const QString &xblToken, const QString &userHash);
    void minecraftLogin(const QString &xstsToken, const QString &userHash);
    void fetchProfile();
    void fail(const QString &error);

    QString m_clientId;
    QString m_deviceCode;
    int m_pollInterval = 5;
    QDateTime m_deviceCodeExpiry;
    QTimer m_pollTimer;
    bool m_cancelled = false;
    Account m_account;
};
