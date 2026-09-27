#include "GameInstaller.h"
#include "Paths.h"
#include "VersionInfo.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>

#ifdef Q_OS_UNIX
#include <unistd.h>
#endif

namespace {
const QString kJavaRuntimeIndex = QStringLiteral(
    "https://launchermeta.mojang.com/v1/products/java-runtime/2ec0cc96c44e5a76b9c8b7c39df7210883d12871/all.json");
const QString kResourcesUrl = QStringLiteral("https://resources.download.minecraft.net/");

QJsonObject readJson(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return {};
    return QJsonDocument::fromJson(f.readAll()).object();
}

DownloadTask taskFrom(const QJsonObject &download, const QString &path)
{
    DownloadTask t;
    t.url = QUrl(download["url"].toString());
    t.path = path;
    t.sha1 = download["sha1"].toString();
    if (download.contains("size"))
        t.size = download["size"].toVariant().toLongLong();
    return t;
}
} // namespace

GameInstaller::GameInstaller(const Instance &instance, QObject *parent)
    : QObject(parent)
    , m_instance(instance)
{
}

void GameInstaller::start()
{
    loadVersion();
}

void GameInstaller::cancel()
{
    if (m_current)
        m_current->abort();
    fail(tr("Cancelled"));
}

void GameInstaller::fail(const QString &error)
{
    if (m_done)
        return;
    m_done = true;
    emit finished(false, error);
}

void GameInstaller::fetch(const DownloadTask &task, std::function<void()> next,
                          std::function<void(const QString &)> onError)
{
    auto *dm = new DownloadManager(this);
    m_current = dm;
    dm->add(task);
    connect(dm, &DownloadManager::finished, this, [this, dm, next, onError](bool ok, const QString &error) {
        dm->deleteLater();
        if (m_done)
            return;
        if (!ok)
            return onError ? onError(error) : fail(error);
        next();
    });
    dm->start();
}

void GameInstaller::loadVersion()
{
    emit status(tr("Checking version %1...").arg(m_instance.versionId));
    DownloadTask task;
    task.url = QUrl(m_instance.versionUrl);
    task.path = VersionInfo::versionJsonPath(m_instance.versionId);
    task.sha1 = m_instance.versionSha1;
    fetch(task, [this, task]() {
        m_version = readJson(task.path);
        if (m_version.isEmpty() || m_version["mainClass"].toString().isEmpty())
            return fail(tr("The version file %1 is invalid.").arg(task.path));
        loadAssetIndex();
    });
}

void GameInstaller::loadAssetIndex()
{
    const QJsonObject ref = m_version["assetIndex"].toObject();
    if (ref.isEmpty())
        return loadJavaRuntime();

    emit status(tr("Checking asset index..."));
    const QString path = Paths::assets() + "/indexes/" + ref["id"].toString() + ".json";
    fetch(taskFrom(ref, path), [this, path]() {
        m_assetIndex = readJson(path);
        loadJavaRuntime();
    });
}

void GameInstaller::loadJavaRuntime()
{
    const QString custom = Paths::settings().value("java/path").toString().trimmed();
    if (!custom.isEmpty()) {
        m_javaPath = custom;
        return downloadAll();
    }

    m_javaComponent = VersionInfo::javaComponent(m_version);
    emit status(tr("Checking Java runtime (%1)...").arg(m_javaComponent));

    // Always refresh the index, but fall back to the cached copy when offline.
    const QString cached = Paths::runtimes() + "/all.json";
    DownloadTask index;
    index.url = QUrl(kJavaRuntimeIndex);
    index.path = cached + ".new";
    QFile::remove(index.path);

    auto useIndex = [this, cached]() {
        const QJsonArray candidates =
            readJson(cached)[VersionInfo::javaPlatform()].toObject()[m_javaComponent].toArray();
        if (candidates.isEmpty()) {
            // Mojang does not ship this runtime for this platform: use the system Java.
            emit status(tr("No bundled Java for this platform, using system Java."));
            m_javaPath = "java";
            return downloadAll();
        }
        loadJavaManifest(candidates.first().toObject()["manifest"].toObject());
    };
    fetch(index, [index, cached, useIndex]() {
        QFile::remove(cached);
        QFile::rename(index.path, cached);
        useIndex();
    }, [this, cached, useIndex](const QString &error) {
        if (!QFile::exists(cached))
            return fail(error);
        useIndex();
    });
}

void GameInstaller::loadJavaManifest(const QJsonObject &manifestRef)
{
    const QString path = Paths::runtimes() + '/' + m_javaComponent + ".manifest.json";
    fetch(taskFrom(manifestRef, path), [this, path]() {
        m_javaManifest = readJson(path);
        m_javaPath = VersionInfo::javaExecutable(m_javaComponent);
        downloadAll();
    });
}

void GameInstaller::downloadAll()
{
    emit status(tr("Downloading game files..."));
    auto *dm = new DownloadManager(this);
    m_current = dm;

    const QString id = m_version["id"].toString(m_instance.versionId);

    // Client jar
    const QJsonObject downloads = m_version["downloads"].toObject();
    if (downloads.contains("client"))
        dm->add(taskFrom(downloads["client"].toObject(), VersionInfo::clientJarPath(id)));

    // Libraries
    for (const VersionInfo::Library &lib : VersionInfo::libraries(m_version)) {
        DownloadTask t;
        t.url = lib.url;
        t.path = lib.path;
        t.sha1 = lib.sha1;
        t.size = lib.size;
        if (!t.url.isEmpty())
            dm->add(t);
    }

    // Logging configuration
    const QJsonObject logFile = m_version["logging"].toObject()["client"].toObject()["file"].toObject();
    if (!logFile.isEmpty())
        dm->add(taskFrom(logFile, Paths::assets() + "/log_configs/" + logFile["id"].toString()));

    // Assets
    const QJsonObject objects = m_assetIndex["objects"].toObject();
    for (auto it = objects.begin(); it != objects.end(); ++it) {
        const QJsonObject obj = it.value().toObject();
        const QString hash = obj["hash"].toString();
        if (hash.size() < 2)
            continue;
        DownloadTask t;
        t.url = QUrl(kResourcesUrl + hash.left(2) + '/' + hash);
        t.path = Paths::assets() + "/objects/" + hash.left(2) + '/' + hash;
        t.sha1 = hash;
        t.size = obj["size"].toVariant().toLongLong();
        dm->add(t);
    }

    // Java runtime files
    const QString javaRoot = Paths::runtimes() + '/' + m_javaComponent;
    const QJsonObject files = m_javaManifest["files"].toObject();
    for (auto it = files.begin(); it != files.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        const QString path = javaRoot + '/' + it.key();
        const QString type = entry["type"].toString();
        if (type == "directory") {
            QDir().mkpath(path);
        } else if (type == "file") {
            DownloadTask t = taskFrom(entry["downloads"].toObject()["raw"].toObject(), path);
            t.executable = entry["executable"].toBool();
            dm->add(t);
        }
    }

    connect(dm, &DownloadManager::progress, this, &GameInstaller::progress);
    connect(dm, &DownloadManager::finished, this, [this, dm](bool ok, const QString &error) {
        dm->deleteLater();
        if (m_done)
            return;
        if (!ok)
            return fail(error);
        postProcess();
    });
    dm->start();
}

void GameInstaller::postProcess()
{
    // Symlinks in the Java runtime (only used on Unix-like systems)
#ifdef Q_OS_UNIX
    const QString javaRoot = Paths::runtimes() + '/' + m_javaComponent;
    const QJsonObject files = m_javaManifest["files"].toObject();
    for (auto it = files.begin(); it != files.end(); ++it) {
        const QJsonObject entry = it.value().toObject();
        if (entry["type"].toString() != "link")
            continue;
        const QString path = javaRoot + '/' + it.key();
        if (QFileInfo(path).isSymLink() || QFileInfo::exists(path))
            continue;
        QDir().mkpath(QFileInfo(path).absolutePath());
        if (::symlink(entry["target"].toString().toLocal8Bit().constData(), path.toLocal8Bit().constData()) != 0)
            qWarning("Could not create symlink %s", qPrintable(path));
    }
#endif

    // Legacy asset layouts (pre-1.7.2 versions)
    const bool virtualAssets = m_assetIndex["virtual"].toBool();
    const bool mapToResources = m_assetIndex["map_to_resources"].toBool();
    if (virtualAssets || mapToResources) {
        emit status(tr("Preparing legacy assets..."));
        const QString target = mapToResources
            ? m_instance.gameDir() + "/resources"
            : Paths::assets() + "/virtual/" + m_version["assetIndex"].toObject()["id"].toString();
        const QJsonObject objects = m_assetIndex["objects"].toObject();
        for (auto it = objects.begin(); it != objects.end(); ++it) {
            const QString hash = it.value().toObject()["hash"].toString();
            const QString dest = target + '/' + it.key();
            if (QFile::exists(dest))
                continue;
            QDir().mkpath(QFileInfo(dest).absolutePath());
            QFile::copy(Paths::assets() + "/objects/" + hash.left(2) + '/' + hash, dest);
        }
    }

    if (!m_javaPath.isEmpty() && m_javaPath != "java" && !QFileInfo::exists(m_javaPath))
        return fail(tr("Java was not found at %1").arg(m_javaPath));

    m_done = true;
    emit finished(true, {});
}
