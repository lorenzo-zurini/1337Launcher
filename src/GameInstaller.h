#pragma once

#include "DownloadManager.h"
#include "Instance.h"

#include <QJsonObject>
#include <QObject>
#include <QPointer>

#include <functional>

// Makes sure everything an instance needs to run is on disk: version JSON,
// client jar, libraries, assets, logging config and a matching Java runtime.
class GameInstaller : public QObject {
    Q_OBJECT
public:
    GameInstaller(const Instance &instance, QObject *parent = nullptr);

    void start();
    void cancel();

    const QJsonObject &version() const { return m_version; }
    QString javaPath() const { return m_javaPath; }

signals:
    void status(const QString &message);
    void progress(int done, int total);
    void finished(bool ok, const QString &error);

private:
    void fetch(const DownloadTask &task, std::function<void()> next,
               std::function<void(const QString &)> onError = {});
    void loadVersion();
    void loadAssetIndex();
    void loadJavaRuntime();
    void loadJavaManifest(const QJsonObject &manifestRef);
    void downloadAll();
    void postProcess();
    void fail(const QString &error);

    Instance m_instance;
    QJsonObject m_version;
    QJsonObject m_assetIndex;
    QJsonObject m_javaManifest;
    QString m_javaComponent;
    QString m_javaPath;
    QPointer<DownloadManager> m_current;
    bool m_done = false;
};
