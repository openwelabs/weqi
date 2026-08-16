#pragma once

#include <QObject>
#include <QString>

// 玩家资料管理：玩家名称、当前 Rating、最佳 Rating。
// 持久化到 profile/player.json。
class ProfileManager : public QObject
{
    Q_OBJECT

public:
    explicit ProfileManager(QObject *parent = nullptr);

    // 玩家名称
    QString playerName() const;
    void setPlayerName(const QString &name);

    // 当前 Rating（默认 1000）
    int rating() const;
    void setRating(int rating);

    // 最佳 Rating（历史最高，只增不减）
    int bestRating() const;

    // 记录一次 Rating 变化：更新当前 Rating 与最佳 Rating
    void applyRatingChange(int delta);

    // 重新加载 / 保存
    void load();
    void save();

signals:
    void profileChanged();

private:
    QString m_playerName = QStringLiteral("Player");
    int m_rating = 1000;
    int m_bestRating = 1000;
};
