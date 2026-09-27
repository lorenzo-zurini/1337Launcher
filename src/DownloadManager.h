#pragma once

#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QUrl>

class QNetworkReply;

struct DownloadTask {
    QUrl url;
    QString path;
    QString sha1;        // optional, verified after download and used to validate existing files
    qint64 size = -1;    // optional, used to skip existing files quickly
    bool executable = false;
};

// Downloads many files in parallel. Files that already exist with the expected
// size/hash are skipped. Emits finished() exactly once.
class DownloadManager : public QObject {
    Q_OBJECT
public:
    explicit DownloadManager(QObject *parent = nullptr);
    ~DownloadManager() override;

    void add(const DownloadTask &task);
    void start();
    void abort();

    static bool fileIsValid(const DownloadTask &task);

signals:
    void progress(int done, int total);
    void finished(bool ok, const QString &error);

private:
    struct Active;

    void pump();
    void startTask(const DownloadTask &task, int attempt);
    void finish(bool ok, const QString &error);

    QList<DownloadTask> m_tasks;
    QSet<QString> m_paths;
    int m_next = 0;
    int m_done = 0;
    int m_running = 0;
    bool m_finished = false;
    QHash<QNetworkReply *, Active *> m_active;
};
