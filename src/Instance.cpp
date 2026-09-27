#include "Instance.h"
#include "Paths.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>

QString Instance::dir() const { return Paths::instances() + '/' + name; }
QString Instance::gameDir() const { return dir() + "/.minecraft"; }
QString Instance::nativesDir() const { return dir() + "/natives"; }

bool Instance::save() const
{
    QDir().mkpath(gameDir());
    QJsonObject o{
        {"name", name},
        {"versionId", versionId},
        {"versionUrl", versionUrl},
        {"versionSha1", versionSha1},
    };
    QSaveFile file(dir() + "/instance.json");
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(QJsonDocument(o).toJson());
    return file.commit();
}

bool Instance::remove() const
{
    if (name.isEmpty())
        return false;
    return QDir(dir()).removeRecursively();
}

QList<Instance> Instance::loadAll()
{
    QList<Instance> result;
    const QDir root(Paths::instances());
    for (const QString &entry : root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase)) {
        QFile file(root.filePath(entry) + "/instance.json");
        if (!file.open(QIODevice::ReadOnly))
            continue;
        const QJsonObject o = QJsonDocument::fromJson(file.readAll()).object();
        Instance inst;
        inst.name = entry;
        inst.versionId = o["versionId"].toString();
        inst.versionUrl = o["versionUrl"].toString();
        inst.versionSha1 = o["versionSha1"].toString();
        if (!inst.versionId.isEmpty())
            result.append(inst);
    }
    return result;
}

bool Instance::isValidName(const QString &name)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9 ._-]{0,63}$"));
    return re.match(name).hasMatch() && !name.endsWith('.') && !name.endsWith(' ');
}
