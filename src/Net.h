#pragma once

#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUrlQuery>

#include <functional>

namespace Net {

QNetworkAccessManager *nam();
QNetworkRequest request(const QUrl &url);

QNetworkReply *get(const QUrl &url, const QByteArray &bearer = {});
QNetworkReply *postJson(const QUrl &url, const QJsonObject &body, const QByteArray &bearer = {});
QNetworkReply *postForm(const QUrl &url, const QUrlQuery &form);

struct Result {
    bool ok = false;      // HTTP 2xx and no transport error
    int status = 0;       // HTTP status, 0 on transport error
    QByteArray body;
    QJsonObject json;     // body parsed as a JSON object, if it was one
    QString error;        // human readable error when !ok
};

// Calls `callback` once the reply finishes (only while `context` is alive) and deletes the reply.
void handle(QNetworkReply *reply, QObject *context, std::function<void(const Result &)> callback);

} // namespace Net
