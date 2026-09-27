#pragma once

#include <QList>
#include <QString>

struct Instance {
    QString name;
    QString versionId;
    QString versionUrl;
    QString versionSha1;

    QString dir() const;
    QString gameDir() const;
    QString nativesDir() const;

    bool save() const;
    bool remove() const;

    static QList<Instance> loadAll();
    static bool isValidName(const QString &name);
};
