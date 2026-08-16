#pragma once

#include <QObject>
#include <QVector>
#include <QString>
#include <QJsonObject>

// 一个 AI Provider 配置。
// API Key 属于用户私密数据，仅保存在系统用户数据目录，绝不写入 Git 仓库。
struct AIProvider
{
    QString id;          // 唯一标识
    QString name;        // 显示名称
    QString provider;    // 提供商类型（OpenAI Compatible / Local / Other）
    QString baseUrl;     // Base URL
    QString apiKey;      // API Key（私密）
    QString model;       // 模型名称

    QJsonObject toJson() const;
    static AIProvider fromJson(const QJsonObject &obj);
};

// AI Provider 管理：增删改查已配置的 AI 提供商。
// 持久化到 config/ai_providers.json。
class AIProviderManager : public QObject
{
    Q_OBJECT

public:
    explicit AIProviderManager(QObject *parent = nullptr);

    const QVector<AIProvider> &providers() const { return m_providers; }
    int count() const { return m_providers.size(); }
    const AIProvider *provider(int index) const;
    const AIProvider *providerById(const QString &id) const;

    // 添加 / 更新 / 删除
    void addProvider(const AIProvider &provider);
    void updateProvider(int index, const AIProvider &provider);
    void removeProvider(int index);

    // 重新加载 / 保存
    void load();
    void save();

signals:
    void providersChanged();

private:
    QVector<AIProvider> m_providers;
};
