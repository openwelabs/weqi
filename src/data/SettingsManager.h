#pragma once

#include <QObject>
#include <QString>

// 应用设置管理：通用偏好设置。
// 持久化到 config/settings.json。
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    explicit SettingsManager(QObject *parent = nullptr);

    // 主题（预留）
    QString theme() const;
    void setTheme(const QString &theme);

    // 是否显示走法提示
    bool showMoveHints() const;
    void setShowMoveHints(bool on);

    // 动画开关
    bool animationsEnabled() const;
    void setAnimationsEnabled(bool on);

    // 界面语言（"system" 表示跟随系统，或语言代码如 "zh-CN"/"en"/"ja"）
    QString language() const;
    void setLanguage(const QString &lang);

    // 重新加载 / 保存
    void load();
    void save();

signals:
    void settingsChanged();

private:
    QString m_theme = QStringLiteral("dark");
    bool m_showMoveHints = true;
    bool m_animationsEnabled = true;
    QString m_language = QStringLiteral("system");
};
