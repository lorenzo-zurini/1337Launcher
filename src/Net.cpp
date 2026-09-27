#include "Net.h"

#include <QCoreApplication>
#include <QJsonDocument>

namespace Net {

QNetworkAccessManager *nam()
{
    static QNetworkAccessManager *manager = new QNetworkAccessManager(QCoreApplication::instance());
    return manager;
}

QNetworkRequest request(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QByteArray("1337Launcher/" LAUNCHER_VERSION));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(60000);
    return req;
}

static void setBearer(QNetworkRequest &req, const QByteArray &bearer)
{
    if (!bearer.isEmpty())
        req.setRawHeader("Authorization", "Bearer " + bearer);
}

QNetworkReply *get(const QUrl &url, const QByteArray &bearer)
{
    QNetworkRequest req = request(url);
    req.setRawHeader("Accept", "application/json");
    setBearer(req, bearer);
    return nam()->get(req);
}

QNetworkReply *postJson(const QUrl &url, const QJsonObject &body, const QByteArray &bearer)
{
    QNetworkRequest req = request(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    setBearer(req, bearer);
    return nam()->post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
}

QNetworkReply *postForm(const QUrl &url, const QUrlQuery &form)
{
    QNetworkRequest req = request(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    req.setRawHeader("Accept", "application/json");
    return nam()->post(req, form.toString(QUrl::FullyEncoded).toUtf8());
}

void handle(QNetworkReply *reply, QObject *context, std::function<void(const Result &)> callback)
{
    QObject::connect(reply, &QNetworkReply::finished, context, [reply, callback]() {
        reply->deleteLater();
        Result r;
        r.status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        r.body = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(r.body);
        if (doc.isObject())
            r.json = doc.object();
        r.ok = r.status >= 200 && r.status < 300;
        if (!r.ok) {
            if (r.status == 0)
                r.error = reply->errorString();
            else
                r.error = QString("HTTP %1 from %2").arg(r.status).arg(reply->url().host());
        }
        callback(r);
    });
}

} // namespace Net
