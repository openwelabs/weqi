#include "AIProviderManager.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QUuid>

#include "DataPaths.h"

QJsonObject AIProvider::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    obj["provider"] = provider;
    obj["baseUrl"] = baseUrl;
    obj["apiKey"] = apiKey;
    obj["model"] = model;
    return obj;
}

AIProvider AIProvider::fromJson(const QJsonObject &obj)
{
    AIProvider p;
    p.id = obj["id"].toString();
    if (p.id.isEmpty())
        p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    p.name = obj["name"].toString();
    p.provider = obj["provider"].toString();
    p.baseUrl = obj["baseUrl"].toString();
    p.apiKey = obj["apiKey"].toString();
    p.model = obj["model"].toString();
    return p;
}

AIProviderManager::AIProviderManager(QObject *parent)
    : QObject(parent)
{
    load();
}

const AIProvider *AIProviderManager::provider(int index) const
{
    if (index < 0 || index >= m_providers.size())
        return nullptr;
    return &m_providers.at(index);
}

const AIProvider *AIProviderManager::providerById(const QString &id) const
{
    for (const AIProvider &p : m_providers) {
        if (p.id == id)
            return &p;
    }
    return nullptr;
}

void AIProviderManager::addProvider(const AIProvider &provider)
{
    AIProvider p = provider;
    if (p.id.isEmpty())
        p.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_providers.append(p);
    save();
    emit providersChanged();
}

void AIProviderManager::updateProvider(int index, const AIProvider &provider)
{
    if (index < 0 || index >= m_providers.size())
        return;
    m_providers[index] = provider;
    save();
    emit providersChanged();
}

void AIProviderManager::removeProvider(int index)
{
    if (index < 0 || index >= m_providers.size())
        return;
    m_providers.removeAt(index);
    save();
    emit providersChanged();
}

void AIProviderManager::load()
{
    m_providers.clear();

    QFile file(DataPaths::aiProvidersFile());
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
    for (const QJsonValue &v : arr) {
        if (v.isObject())
            m_providers.append(AIProvider::fromJson(v.toObject()));
    }
}

void AIProviderManager::save()
{
    QJsonArray arr;
    for (const AIProvider &p : m_providers)
        arr.append(p.toJson());

    QFile file(DataPaths::aiProvidersFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(arr).toJson(QJsonDocument::Indented));
}
