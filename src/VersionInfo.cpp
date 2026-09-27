#include "VersionInfo.h"
#include "Paths.h"

#include <QRegularExpression>
#include <QSysInfo>

namespace VersionInfo {

QString osName()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("osx");
#else
    return QStringLiteral("linux");
#endif
}

static bool is64Bit()
{
    return QSysInfo::WordSize == 64;
}

static bool isArm64()
{
    return QSysInfo::currentCpuArchitecture() == QLatin1String("arm64");
}

QString javaPlatform()
{
#if defined(Q_OS_WIN)
    if (isArm64())
        return "windows-arm64";
    return is64Bit() ? "windows-x64" : "windows-x86";
#elif defined(Q_OS_MACOS)
    return isArm64() ? "mac-os-arm64" : "mac-os";
#else
    if (isArm64())
        return "linux-arm64"; // not provided by Mojang; falls back to the system Java
    return is64Bit() ? "linux" : "linux-i386";
#endif
}

bool rulesAllow(const QJsonArray &rules, const QHash<QString, bool> &features)
{
    if (rules.isEmpty())
        return true;

    bool allowed = false;
    for (const QJsonValue &v : rules) {
        const QJsonObject rule = v.toObject();
        bool matches = true;

        const QJsonObject os = rule["os"].toObject();
        if (os.contains("name") && os["name"].toString() != osName())
            matches = false;
        if (os.contains("arch")) {
            const QString arch = os["arch"].toString();
            if ((arch == "x86" && is64Bit()) || (arch == "arm64" && !isArm64()))
                matches = false;
        }
        if (os.contains("version")) {
            const QRegularExpression re(os["version"].toString());
            if (!re.match(QSysInfo::kernelVersion()).hasMatch())
                matches = false;
        }

        const QJsonObject feats = rule["features"].toObject();
        for (auto it = feats.begin(); it != feats.end(); ++it) {
            if (features.value(it.key(), false) != it.value().toBool())
                matches = false;
        }

        if (matches)
            allowed = rule["action"].toString() == "allow";
    }
    return allowed;
}

QString mavenPath(const QString &name, const QString &classifier)
{
    QString coords = name;
    QString ext = "jar";
    const int at = coords.indexOf('@');
    if (at >= 0) {
        ext = coords.mid(at + 1);
        coords = coords.left(at);
    }
    const QStringList parts = coords.split(':');
    if (parts.size() < 3)
        return {};
    const QString group = QString(parts[0]).replace('.', '/');
    const QString artifact = parts[1];
    const QString version = parts[2];
    QString cls = classifier;
    if (cls.isEmpty() && parts.size() > 3)
        cls = parts[3];

    QString file = artifact + '-' + version;
    if (!cls.isEmpty())
        file += '-' + cls;
    return group + '/' + artifact + '/' + version + '/' + file + '.' + ext;
}

static Library fromArtifact(const QString &name, const QJsonObject &artifact, const QString &classifier,
                            const QString &baseUrl)
{
    Library lib;
    lib.name = name;
    QString rel = artifact["path"].toString();
    if (rel.isEmpty())
        rel = mavenPath(name, classifier);
    lib.path = Paths::libraries() + '/' + rel;
    lib.sha1 = artifact["sha1"].toString();
    lib.size = artifact.contains("size") ? artifact["size"].toVariant().toLongLong() : -1;
    const QString url = artifact["url"].toString();
    if (!url.isEmpty())
        lib.url = QUrl(url);
    else if (!artifact.contains("url")) // Very old JSONs: derive from the maven repo
        lib.url = QUrl(baseUrl + rel);
    return lib;
}

QList<Library> libraries(const QJsonObject &version)
{
    QList<Library> result;
    const QString arch = is64Bit() ? "64" : "32";

    for (const QJsonValue &v : version["libraries"].toArray()) {
        const QJsonObject lib = v.toObject();
        if (!rulesAllow(lib["rules"].toArray()))
            continue;

        const QString name = lib["name"].toString();
        QString baseUrl = lib["url"].toString("https://libraries.minecraft.net/");
        if (!baseUrl.endsWith('/'))
            baseUrl += '/';
        const QJsonObject downloads = lib["downloads"].toObject();
        const bool hasNatives = lib.contains("natives");

        // Classpath artifact
        if (downloads.contains("artifact")) {
            result.append(fromArtifact(name, downloads["artifact"].toObject(), {}, baseUrl));
        } else if (!downloads.contains("classifiers") && !hasNatives) {
            result.append(fromArtifact(name, QJsonObject(), {}, baseUrl));
        }

        // Old-style natives that need extracting (pre 1.19)
        if (hasNatives) {
            QString classifier = lib["natives"].toObject()[osName()].toString();
            if (classifier.isEmpty())
                continue;
            classifier.replace("${arch}", arch);
            const QJsonObject artifact = downloads["classifiers"].toObject()[classifier].toObject();
            Library native = fromArtifact(name, artifact, classifier, baseUrl);
            native.extract = true;
            for (const QJsonValue &ex : lib["extract"].toObject()["exclude"].toArray())
                native.extractExclude.append(ex.toString());
            result.append(native);
        }
    }
    return result;
}

QString versionJsonPath(const QString &id)
{
    return Paths::versions() + '/' + id + '/' + id + ".json";
}

QString clientJarPath(const QString &id)
{
    return Paths::versions() + '/' + id + '/' + id + ".jar";
}

QString javaComponent(const QJsonObject &version)
{
    const QString component = version["javaVersion"].toObject()["component"].toString();
    return component.isEmpty() ? QStringLiteral("jre-legacy") : component;
}

QString javaExecutable(const QString &component)
{
    const QString root = Paths::runtimes() + '/' + component;
#if defined(Q_OS_WIN)
    return root + "/bin/javaw.exe";
#elif defined(Q_OS_MACOS)
    return root + "/jre.bundle/Contents/Home/bin/java";
#else
    return root + "/bin/java";
#endif
}

} // namespace VersionInfo
