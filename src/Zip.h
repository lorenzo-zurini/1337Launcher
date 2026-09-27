#pragma once

#include <QString>
#include <QStringList>

namespace Zip {

// Extracts all files of `zipPath` into `destDir`, skipping entries starting with any `excludePrefixes`.
bool extract(const QString &zipPath, const QString &destDir, const QStringList &excludePrefixes, QString *error);

} // namespace Zip
