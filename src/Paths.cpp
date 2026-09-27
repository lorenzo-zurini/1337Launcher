#include "Paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

#include <memory>

namespace {
QString g_data;
std::unique_ptr<QSettings> g_settings;
} // namespace

namespace Paths {

void init()
{
    // A "portable.txt" file next to the executable keeps all data beside it.
    // Otherwise use the per-user data directory (the AppImage mount is read-only).
    const QString appDir = QCoreApplication::applicationDirPath();
    if (QFile::exists(appDir + "/portable.txt"))
        g_data = appDir + "/data";
    else
        g_data = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    for (const QString &dir : {data(), instances(), libraries(), assets(), versions(), runtimes()})
        QDir().mkpath(dir);

    g_settings = std::make_unique<QSettings>(g_data + "/settings.ini", QSettings::IniFormat);
}

QString data() { return g_data; }
QString instances() { return g_data + "/instances"; }
QString libraries() { return g_data + "/libraries"; }
QString assets() { return g_data + "/assets"; }
QString versions() { return g_data + "/versions"; }
QString runtimes() { return g_data + "/runtimes"; }

QSettings &settings() { return *g_settings; }

} // namespace Paths
