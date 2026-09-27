#include "DownloadManager.h"
#include "Net.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QNetworkReply>
#include <QSaveFile>

namespace {
constexpr int kMaxParallel = 8;
constexpr int kMaxAttempts = 3;

void makeExecutable(const QString &path)
{
    QFile::setPermissions(path, QFile::permissions(path) | QFileDevice::ExeOwner | QFileDevice::ExeGroup
                                    | QFileDevice::ExeOther | QFileDevice::ExeUser);
}
} // namespace

struct DownloadManager::Active {
    DownloadTask task;
    int attempt = 0;
    QSaveFile *file = nullptr;
    QCryptographicHash hash{QCryptographicHash::Sha1};
    bool writeError = false;
};

DownloadManager::DownloadManager(QObject *parent)
    : QObject(parent)
{
}

DownloadManager::~DownloadManager()
{
    m_finished = true;
    abort();
}

void DownloadManager::add(const DownloadTask &task)
{
    // The same file (e.g. an asset referenced twice) only needs downloading once.
    if (m_paths.contains(task.path))
        return;
    m_paths.insert(task.path);
    m_tasks.append(task);
}

bool DownloadManager::fileIsValid(const DownloadTask &task)
{
    QFileInfo info(task.path);
    if (!info.isFile())
        return false;
    if (task.size >= 0)
        return info.size() == task.size;
    if (!task.sha1.isEmpty()) {
        QFile f(task.path);
        if (!f.open(QIODevice::ReadOnly))
            return false;
        QCryptographicHash hash(QCryptographicHash::Sha1);
        hash.addData(&f);
        return hash.result().toHex() == task.sha1.toLatin1().toLower();
    }
    return true;
}

void DownloadManager::start()
{
    QList<DownloadTask> needed;
    for (const DownloadTask &task : std::as_const(m_tasks)) {
        if (fileIsValid(task)) {
            if (task.executable)
                makeExecutable(task.path);
        } else {
            needed.append(task);
        }
    }
    m_tasks = needed;
    m_next = 0;
    m_done = 0;

    if (m_tasks.isEmpty()) {
        // Emit asynchronously so callers can safely delete us from the slot.
        QMetaObject::invokeMethod(this, [this]() { finish(true, {}); }, Qt::QueuedConnection);
        return;
    }
    emit progress(0, m_tasks.size());
    pump();
}

void DownloadManager::abort()
{
    const auto active = m_active;
    m_active.clear();
    for (auto it = active.begin(); it != active.end(); ++it) {
        it.key()->disconnect(this);
        it.key()->abort();
        it.key()->deleteLater();
        if (it.value()->file)
            it.value()->file->cancelWriting();
        delete it.value()->file;
        delete it.value();
    }
    m_running = 0;
    finish(false, tr("Download cancelled"));
}

void DownloadManager::pump()
{
    while (!m_finished && m_running < kMaxParallel && m_next < m_tasks.size())
        startTask(m_tasks[m_next++], 1);
}

void DownloadManager::startTask(const DownloadTask &task, int attempt)
{
    QDir().mkpath(QFileInfo(task.path).absolutePath());

    auto *active = new Active;
    active->task = task;
    active->attempt = attempt;
    active->file = new QSaveFile(task.path);
    if (!active->file->open(QIODevice::WriteOnly)) {
        const QString err = tr("Cannot write %1: %2").arg(task.path, active->file->errorString());
        delete active->file;
        delete active;
        finish(false, err);
        return;
    }

    ++m_running;
    QNetworkReply *reply = Net::nam()->get(Net::request(task.url));
    m_active.insert(reply, active);

    connect(reply, &QNetworkReply::readyRead, this, [reply, active]() {
        const QByteArray chunk = reply->readAll();
        active->hash.addData(chunk);
        if (active->file->write(chunk) != chunk.size())
            active->writeError = true;
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        Active *active = m_active.take(reply);
        reply->deleteLater();
        if (!active)
            return;
        --m_running;

        const QByteArray rest = reply->readAll();
        active->hash.addData(rest);
        if (active->file->write(rest) != rest.size())
            active->writeError = true;

        const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString error;
        if (reply->error() != QNetworkReply::NoError)
            error = reply->errorString();
        else if (status < 200 || status >= 300)
            error = QString("HTTP %1").arg(status);
        else if (active->writeError)
            error = tr("write error: %1").arg(active->file->errorString());
        else if (!active->task.sha1.isEmpty()
                 && active->hash.result().toHex() != active->task.sha1.toLatin1().toLower())
            error = tr("checksum mismatch");

        if (error.isEmpty() && !active->file->commit())
            error = tr("could not save file: %1").arg(active->file->errorString());
        else if (!error.isEmpty())
            active->file->cancelWriting();

        const DownloadTask task = active->task;
        const int attempt = active->attempt;
        delete active->file;
        delete active;

        if (m_finished)
            return;

        if (!error.isEmpty()) {
            if (attempt < kMaxAttempts)
                return startTask(task, attempt + 1);
            return finish(false, tr("Failed to download %1: %2").arg(task.url.toString(), error));
        }

        if (task.executable)
            makeExecutable(task.path);
        ++m_done;
        emit progress(m_done, m_tasks.size());
        if (m_done == m_tasks.size())
            finish(true, {});
        else
            pump();
    });
}

void DownloadManager::finish(bool ok, const QString &error)
{
    if (m_finished)
        return;
    m_finished = true;
    if (!ok) {
        // Stop anything still in flight.
        const auto active = m_active;
        m_active.clear();
        for (auto it = active.begin(); it != active.end(); ++it) {
            it.key()->disconnect(this);
            it.key()->abort();
            it.key()->deleteLater();
            it.value()->file->cancelWriting();
            delete it.value()->file;
            delete it.value();
        }
        m_running = 0;
    }
    emit finished(ok, error);
}
