#include "ProfileManager.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include "DataPaths.h"

ProfileManager::ProfileManager(QObject *parent)
    : QObject(parent)
{
    load();
}

QString ProfileManager::playerName() const
{
    return m_playerName;
}

void ProfileManager::setPlayerName(const QString &name)
{
    if (name.trimmed().isEmpty() || name == m_playerName)
        return;
    m_playerName = name.trimmed();
    save();
    emit profileChanged();
}

int ProfileManager::rating() const
{
    return m_rating;
}

void ProfileManager::setRating(int rating)
{
    m_rating = rating;
    if (m_rating > m_bestRating)
        m_bestRating = m_rating;
    save();
    emit profileChanged();
}

int ProfileManager::bestRating() const
{
    return m_bestRating;
}

void ProfileManager::applyRatingChange(int delta)
{
    setRating(m_rating + delta);
}

void ProfileManager::load()
{
    QFile file(DataPaths::playerFile());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
    m_playerName = obj["name"].toString(QStringLiteral("Player"));
    m_rating = obj["rating"].toInt(1000);
    m_bestRating = obj["bestRating"].toInt(m_rating);
    if (m_bestRating < m_rating)
        m_bestRating = m_rating;
}

void ProfileManager::save()
{
    QJsonObject obj;
    obj["name"] = m_playerName;
    obj["rating"] = m_rating;
    obj["bestRating"] = m_bestRating;

    QFile file(DataPaths::playerFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}
