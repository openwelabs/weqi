#include "SettingsManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include "DataPaths.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QString SettingsManager::theme() const
{
    return m_theme;
}

void SettingsManager::setTheme(const QString &theme)
{
    if (theme == m_theme)
        return;
    m_theme = theme;
    save();
    emit settingsChanged();
}

bool SettingsManager::showMoveHints() const
{
    return m_showMoveHints;
}

void SettingsManager::setShowMoveHints(bool on)
{
    if (on == m_showMoveHints)
        return;
    m_showMoveHints = on;
    save();
    emit settingsChanged();
}

bool SettingsManager::animationsEnabled() const
{
    return m_animationsEnabled;
}

void SettingsManager::setAnimationsEnabled(bool on)
{
    if (on == m_animationsEnabled)
        return;
    m_animationsEnabled = on;
    save();
    emit settingsChanged();
}

void SettingsManager::load()
{
    QFile file(DataPaths::settingsFile());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    m_theme = obj["theme"].toString(QStringLiteral("dark"));
    m_showMoveHints = obj["showMoveHints"].toBool(true);
    m_animationsEnabled = obj["animationsEnabled"].toBool(true);
}

void SettingsManager::save()
{
    QJsonObject obj;
    obj["theme"] = m_theme;
    obj["showMoveHints"] = m_showMoveHints;
    obj["animationsEnabled"] = m_animationsEnabled;

    QFile file(DataPaths::settingsFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}
