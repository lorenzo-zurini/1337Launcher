#include "MicrosoftAuth.h"
#include "Net.h"
#include "Paths.h"

#include <QJsonArray>
#include <QUrlQuery>

namespace {
const QUrl kDeviceCodeUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/devicecode");
const QUrl kTokenUrl("https://login.microsoftonline.com/consumers/oauth2/v2.0/token");
const QUrl kXblUrl("https://user.auth.xboxlive.com/user/authenticate");
const QUrl kXstsUrl("https://xsts.auth.xboxlive.com/xsts/authorize");
const QUrl kMcLoginUrl("https://api.minecraftservices.com/authentication/login_with_xbox");
const QUrl kMcProfileUrl("https://api.minecraftservices.com/minecraft/profile");
const QString kScope = QStringLiteral("XboxLive.signin offline_access");

QString oauthError(const Net::Result &r)
{
    const QString desc = r.json["error_description"].toString();
    if (!desc.isEmpty())
        return desc.section('\n', 0, 0).section(QStringLiteral(" Trace ID:"), 0, 0).trimmed();
    return r.error;
}
} // namespace

MicrosoftAuth::MicrosoftAuth(QObject *parent)
    : QObject(parent)
    , m_clientId(clientId())
{
    m_pollTimer.setSingleShot(true);
    connect(&m_pollTimer, &QTimer::timeout, this, &MicrosoftAuth::pollDeviceCode);
}

QString MicrosoftAuth::clientId()
{
    const QString configured = Paths::settings().value("auth/clientId").toString().trimmed();
    if (!configured.isEmpty())
        return configured;
    return QStringLiteral(LAUNCHER_MSA_CLIENT_ID);
}

void MicrosoftAuth::cancel()
{
    m_cancelled = true;
    m_pollTimer.stop();
}

void MicrosoftAuth::fail(const QString &error)
{
    if (m_cancelled)
        return;
    m_cancelled = true;
    m_pollTimer.stop();
    emit failed(error);
}

void MicrosoftAuth::startDeviceLogin()
{
    if (m_clientId.isEmpty()) {
        fail(tr("No Microsoft application (client) ID is configured.\n\n"
                "Register an Azure application with \"Allow public client flows\" enabled "
                "and enter its client ID in Settings. See the README for details."));
        return;
    }

    m_account = Account();
    m_account.type = Account::Microsoft;
    emit status(tr("Requesting login code..."));

    QUrlQuery form;
    form.addQueryItem("client_id", m_clientId);
    form.addQueryItem("scope", kScope);
    Net::handle(Net::postForm(kDeviceCodeUrl, form), this, [this](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (!r.ok)
            return fail(tr("Could not start Microsoft login: %1").arg(oauthError(r)));

        m_deviceCode = r.json["device_code"].toString();
        m_pollInterval = qMax(1, r.json["interval"].toInt(5));
        m_deviceCodeExpiry = QDateTime::currentDateTimeUtc().addSecs(r.json["expires_in"].toInt(900));
        emit deviceCodeReady(r.json["user_code"].toString(), r.json["verification_uri"].toString());
        emit status(tr("Waiting for you to sign in in your browser..."));
        m_pollTimer.start(m_pollInterval * 1000);
    });
}

void MicrosoftAuth::pollDeviceCode()
{
    if (m_cancelled)
        return;
    if (QDateTime::currentDateTimeUtc() > m_deviceCodeExpiry)
        return fail(tr("The login code expired. Please try again."));

    QUrlQuery form;
    form.addQueryItem("grant_type", "urn:ietf:params:oauth:grant-type:device_code");
    form.addQueryItem("client_id", m_clientId);
    form.addQueryItem("device_code", m_deviceCode);
    Net::handle(Net::postForm(kTokenUrl, form), this, [this](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (r.ok)
            return onMsaToken(r.json);

        const QString error = r.json["error"].toString();
        if (error == "authorization_pending") {
            m_pollTimer.start(m_pollInterval * 1000);
        } else if (error == "slow_down") {
            m_pollInterval += 5;
            m_pollTimer.start(m_pollInterval * 1000);
        } else if (error == "authorization_declined") {
            fail(tr("Login was declined."));
        } else if (error == "expired_token") {
            fail(tr("The login code expired. Please try again."));
        } else {
            fail(tr("Microsoft login failed: %1").arg(oauthError(r)));
        }
    });
}

void MicrosoftAuth::refresh(const Account &account)
{
    m_account = account;
    if (m_clientId.isEmpty())
        return fail(tr("No Microsoft application (client) ID is configured. Set it in Settings."));
    if (account.msaRefreshToken.isEmpty())
        return fail(tr("This account has no saved login. Please remove it and log in again."));

    emit status(tr("Refreshing Microsoft login..."));
    QUrlQuery form;
    form.addQueryItem("grant_type", "refresh_token");
    form.addQueryItem("client_id", m_clientId);
    form.addQueryItem("refresh_token", account.msaRefreshToken);
    form.addQueryItem("scope", kScope);
    Net::handle(Net::postForm(kTokenUrl, form), this, [this](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (!r.ok)
            return fail(tr("Your Microsoft login has expired, please log in again.\n(%1)").arg(oauthError(r)));
        onMsaToken(r.json);
    });
}

void MicrosoftAuth::onMsaToken(const QJsonObject &json)
{
    const QString refreshToken = json["refresh_token"].toString();
    if (!refreshToken.isEmpty())
        m_account.msaRefreshToken = refreshToken;
    xboxLiveAuth(json["access_token"].toString());
}

void MicrosoftAuth::xboxLiveAuth(const QString &msaAccessToken)
{
    emit status(tr("Signing in to Xbox Live..."));
    QJsonObject props{
        {"AuthMethod", "RPS"},
        {"SiteName", "user.auth.xboxlive.com"},
        {"RpsTicket", "d=" + msaAccessToken},
    };
    QJsonObject body{
        {"Properties", props},
        {"RelyingParty", "http://auth.xboxlive.com"},
        {"TokenType", "JWT"},
    };
    Net::handle(Net::postJson(kXblUrl, body), this, [this](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (!r.ok)
            return fail(tr("Xbox Live authentication failed: %1").arg(r.error));
        const QString token = r.json["Token"].toString();
        const QString uhs = r.json["DisplayClaims"].toObject()["xui"].toArray().at(0).toObject()["uhs"].toString();
        xstsAuth(token, uhs);
    });
}

void MicrosoftAuth::xstsAuth(const QString &xblToken, const QString &userHash)
{
    emit status(tr("Getting Xbox security token..."));
    QJsonObject props{
        {"SandboxId", "RETAIL"},
        {"UserTokens", QJsonArray{xblToken}},
    };
    QJsonObject body{
        {"Properties", props},
        {"RelyingParty", "rp://api.minecraftservices.com/"},
        {"TokenType", "JWT"},
    };
    Net::handle(Net::postJson(kXstsUrl, body), this, [this, userHash](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (!r.ok) {
            const qint64 xerr = r.json["XErr"].toVariant().toLongLong();
            switch (xerr) {
            case 2148916233LL:
                return fail(tr("This Microsoft account has no Xbox profile. Sign in once at minecraft.net and try again."));
            case 2148916235LL:
                return fail(tr("Xbox Live is not available in your country."));
            case 2148916236LL:
            case 2148916237LL:
                return fail(tr("This account needs adult verification on the Xbox website."));
            case 2148916238LL:
                return fail(tr("This is a child account; it must be added to a Microsoft family by an adult."));
            default:
                return fail(tr("Xbox security token request failed: %1").arg(r.error));
            }
        }
        minecraftLogin(r.json["Token"].toString(), userHash);
    });
}

void MicrosoftAuth::minecraftLogin(const QString &xstsToken, const QString &userHash)
{
    emit status(tr("Logging in to Minecraft..."));
    QJsonObject body{{"identityToken", QString("XBL3.0 x=%1;%2").arg(userHash, xstsToken)}};
    Net::handle(Net::postJson(kMcLoginUrl, body), this, [this](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (!r.ok) {
            if (r.status == 403)
                return fail(tr("Minecraft services rejected the login (HTTP 403).\n\n"
                               "The configured Azure application is probably not approved by Mojang "
                               "for the Minecraft API. See the README."));
            return fail(tr("Minecraft login failed: %1").arg(r.error));
        }
        m_account.accessToken = r.json["access_token"].toString();
        m_account.accessTokenExpiry = QDateTime::currentDateTimeUtc().addSecs(r.json["expires_in"].toInt(86400));
        fetchProfile();
    });
}

void MicrosoftAuth::fetchProfile()
{
    emit status(tr("Fetching Minecraft profile..."));
    Net::handle(Net::get(kMcProfileUrl, m_account.accessToken.toUtf8()), this, [this](const Net::Result &r) {
        if (m_cancelled)
            return;
        if (r.status == 404)
            return fail(tr("This Microsoft account does not own Minecraft: Java Edition, "
                           "or has not created a profile name yet."));
        if (!r.ok)
            return fail(tr("Could not fetch Minecraft profile: %1").arg(r.error));

        m_account.uuid = r.json["id"].toString();
        m_account.username = r.json["name"].toString();
        m_cancelled = true; // done; ignore anything else
        emit succeeded(m_account);
    });
}
