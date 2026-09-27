#pragma once

#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUrl>

// Helpers for Mojang's version JSON format.
namespace VersionInfo {

QString osName();       // "windows", "osx" or "linux"
QString javaPlatform(); // key used in Mojang's java-runtime manifest

// Evaluates a "rules" array. Features (is_demo_user, has_custom_resolution, ...) default to false.
bool rulesAllow(const QJsonArray &rules, const QHash<QString, bool> &features = {});

struct Library {
    QString name;
    QString path;       // absolute path on disk
    QUrl url;
    QString sha1;
    qint64 size = -1;
    bool extract = false;           // old-style native library that must be unpacked
    QStringList extractExclude;
};

QList<Library> libraries(const QJsonObject &version);
QString mavenPath(const QString &name, const QString &classifier = {});

QString versionJsonPath(const QString &id);
QString clientJarPath(const QString &id);

QString javaComponent(const QJsonObject &version);
QString javaExecutable(const QString &component);

} // namespace VersionInfo
