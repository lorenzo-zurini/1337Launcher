#include "Zip.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "miniz.h"

namespace Zip {

bool extract(const QString &zipPath, const QString &destDir, const QStringList &excludePrefixes, QString *error)
{
    QFile file(zipPath);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QString("Cannot open %1").arg(zipPath);
        return false;
    }
    const QByteArray data = file.readAll();

    mz_zip_archive zip{};
    if (!mz_zip_reader_init_mem(&zip, data.constData(), size_t(data.size()), 0)) {
        *error = QString("%1 is not a valid zip file").arg(zipPath);
        return false;
    }

    const QString root = QDir(destDir).absolutePath();
    bool ok = true;
    const mz_uint count = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < count && ok; ++i) {
        mz_zip_archive_file_stat stat;
        if (!mz_zip_reader_file_stat(&zip, i, &stat) || stat.m_is_directory)
            continue;
        const QString name = QString::fromUtf8(stat.m_filename);

        bool excluded = false;
        for (const QString &prefix : excludePrefixes)
            excluded |= name.startsWith(prefix);
        if (excluded)
            continue;

        const QString target = QDir::cleanPath(root + '/' + name);
        if (!target.startsWith(root + '/')) // zip-slip protection
            continue;

        size_t size = 0;
        void *buffer = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (!buffer) {
            *error = QString("Failed to extract %1 from %2").arg(name, zipPath);
            ok = false;
            break;
        }
        QDir().mkpath(QFileInfo(target).absolutePath());
        QFile out(target);
        if (!out.open(QIODevice::WriteOnly) || out.write(static_cast<const char *>(buffer), qint64(size)) != qint64(size)) {
            *error = QString("Cannot write %1").arg(target);
            ok = false;
        }
        mz_free(buffer);
    }
    mz_zip_reader_end(&zip);
    return ok;
}

} // namespace Zip
