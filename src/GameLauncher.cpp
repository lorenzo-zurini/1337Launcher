#include "GameLauncher.h"
#include "Paths.h"
#include "VersionInfo.h"
#include "Zip.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>

namespace {
QString substitute(QString value, const QHash<QString, QString> &vars)
{
    static const QRegularExpression re(QStringLiteral("\\$\\{([A-Za-z0-9_]+)\\}"));
    QString result;
    qsizetype last = 0;
    auto it = re.globalMatch(value);
    while (it.hasNext()) {
        const auto m = it.next();
        result += value.mid(last, m.capturedStart() - last);
        const QString key = m.captured(1);
        result += vars.contains(key) ? vars.value(key) : m.captured(0);
        last = m.capturedEnd();
    }
    result += value.mid(last);
    return result;
}
} // namespace

GameLauncher::GameLauncher(const Instance &instance, const Account &account, const QJsonObject &version,
                           const QString &javaPath, QObject *parent)
    : QObject(parent)
    , m_instance(instance)
    , m_account(account)
    , m_version(version)
    , m_javaPath(javaPath)
{
    m_process.setProcessChannelMode(QProcess::MergedChannels);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit output(QString::fromLocal8Bit(m_process.readAllStandardOutput()));
    });
    connect(&m_process, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) { emit exited(code); });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err == QProcess::FailedToStart) {
            emit output(tr("Failed to start Java (%1): %2\n").arg(m_javaPath, m_process.errorString()));
            emit exited(-1);
        }
    });
}

void GameLauncher::kill()
{
    m_process.kill();
}

bool GameLauncher::extractNatives(QString *error)
{
    QDir(m_instance.nativesDir()).removeRecursively();
    QDir().mkpath(m_instance.nativesDir());
    for (const VersionInfo::Library &lib : VersionInfo::libraries(m_version)) {
        if (!lib.extract)
            continue;
        if (!Zip::extract(lib.path, m_instance.nativesDir(), lib.extractExclude, error))
            return false;
    }
    return true;
}

QStringList GameLauncher::expand(const QJsonArray &args, const QHash<QString, QString> &vars) const
{
    QStringList out;
    for (const QJsonValue &v : args) {
        if (v.isString()) {
            out << substitute(v.toString(), vars);
            continue;
        }
        const QJsonObject obj = v.toObject();
        if (!VersionInfo::rulesAllow(obj["rules"].toArray()))
            continue;
        const QJsonValue value = obj["value"];
        if (value.isArray()) {
            for (const QJsonValue &s : value.toArray())
                out << substitute(s.toString(), vars);
        } else {
            out << substitute(value.toString(), vars);
        }
    }
    return out;
}

QStringList GameLauncher::buildArguments()
{
    const QString id = m_version["id"].toString(m_instance.versionId);
#ifdef Q_OS_WIN
    const QString sep = ";";
#else
    const QString sep = ":";
#endif

    QStringList classpath;
    for (const VersionInfo::Library &lib : VersionInfo::libraries(m_version)) {
        if (!lib.extract && !classpath.contains(lib.path))
            classpath << lib.path;
    }
    classpath << VersionInfo::clientJarPath(id);

    const QJsonObject assetIndex = m_version["assetIndex"].toObject();
    const QString assetsId = assetIndex["id"].toString(m_version["assets"].toString());
    QString gameAssets = Paths::assets() + "/virtual/" + assetsId;
    if (QFileInfo::exists(m_instance.gameDir() + "/resources") && assetsId == "pre-1.6")
        gameAssets = m_instance.gameDir() + "/resources";

    const QHash<QString, QString> vars{
        {"auth_player_name", m_account.username},
        {"auth_uuid", m_account.uuid},
        {"auth_access_token", m_account.isMicrosoft() ? m_account.accessToken : QStringLiteral("0")},
        {"auth_session", m_account.isMicrosoft() ? QString("token:%1:%2").arg(m_account.accessToken, m_account.uuid)
                                                 : QStringLiteral("0")},
        {"auth_xuid", "0"},
        {"clientid", "0"},
        {"user_type", m_account.isMicrosoft() ? QStringLiteral("msa") : QStringLiteral("legacy")},
        {"user_properties", "{}"},
        {"version_name", id},
        {"version_type", m_version["type"].toString("release")},
        {"game_directory", m_instance.gameDir()},
        {"assets_root", Paths::assets()},
        {"game_assets", gameAssets},
        {"assets_index_name", assetsId},
        {"natives_directory", m_instance.nativesDir()},
        {"library_directory", Paths::libraries()},
        {"classpath_separator", sep},
        {"classpath", classpath.join(sep)},
        {"launcher_name", "1337Launcher"},
        {"launcher_version", LAUNCHER_VERSION},
    };

    QSettings &s = Paths::settings();
    QStringList args;
    args << QString("-Xms%1M").arg(s.value("java/minMemory", 512).toInt());
    args << QString("-Xmx%1M").arg(s.value("java/maxMemory", 4096).toInt());
    args << QProcess::splitCommand(s.value("java/extraArgs").toString());

    const QJsonObject arguments = m_version["arguments"].toObject();
    if (arguments.contains("jvm")) {
        args << expand(arguments["jvm"].toArray(), vars);
    } else {
        args << substitute("-Djava.library.path=${natives_directory}", vars) << "-cp" << vars["classpath"];
    }

    const QJsonObject logging = m_version["logging"].toObject()["client"].toObject();
    const QString logId = logging["file"].toObject()["id"].toString();
    if (!logId.isEmpty()) {
        QString arg = logging["argument"].toString();
        arg.replace("${path}", Paths::assets() + "/log_configs/" + logId);
        args << arg;
    }

    args << m_version["mainClass"].toString();

    if (arguments.contains("game")) {
        args << expand(arguments["game"].toArray(), vars);
    } else {
        for (const QString &a : m_version["minecraftArguments"].toString().split(' ', Qt::SkipEmptyParts))
            args << substitute(a, vars);
    }
    return args;
}

bool GameLauncher::start(QString *error)
{
    QDir().mkpath(m_instance.gameDir());
    if (!extractNatives(error))
        return false;

    const QStringList args = buildArguments();

    QString shown = m_javaPath + ' ' + args.join(' ');
    if (!m_account.accessToken.isEmpty())
        shown.replace(m_account.accessToken, "<access token>");
    emit output(tr("Launching: %1\n\n").arg(shown));

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (env.contains("APPIMAGE")) {
        // Don't leak the AppImage's bundled Qt/library setup into Java.
        for (const char *var : {"LD_LIBRARY_PATH", "QT_PLUGIN_PATH", "QML2_IMPORT_PATH"})
            env.remove(var);
    }
    m_process.setProcessEnvironment(env);
    m_process.setWorkingDirectory(m_instance.gameDir());
    m_process.setProgram(m_javaPath);
    m_process.setArguments(args);
    m_process.start();
    return true;
}
