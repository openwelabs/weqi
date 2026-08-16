#include "DataPaths.h"

#include <QStandardPaths>
#include <QDir>

namespace {

QString ensureDir(const QString &path)
{
    QDir().mkpath(path);
    return path;
}

} // namespace

QString DataPaths::appDataDir()
{
    return ensureDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
}

QString DataPaths::configDir()
{
    return ensureDir(appDataDir() + QStringLiteral("/config"));
}

QString DataPaths::gamesDir()
{
    return ensureDir(appDataDir() + QStringLiteral("/games"));
}

QString DataPaths::profileDir()
{
    return ensureDir(appDataDir() + QStringLiteral("/profile"));
}

QString DataPaths::savesDir()
{
    return ensureDir(appDataDir() + QStringLiteral("/saves"));
}

QString DataPaths::settingsFile()
{
    return configDir() + QStringLiteral("/settings.json");
}

QString DataPaths::aiProvidersFile()
{
    return configDir() + QStringLiteral("/ai_providers.json");
}

QString DataPaths::playerFile()
{
    return profileDir() + QStringLiteral("/player.json");
}

QString DataPaths::currentGameFile()
{
    return savesDir() + QStringLiteral("/current_game.json");
}
