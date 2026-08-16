#pragma once

#include <QString>

// 统一管理用户数据目录。
// 数据存放在系统用户数据目录（QStandardPaths::AppDataLocation），
// 绝不写入项目根目录或 Git 仓库。
//
// Linux:   ~/.local/share/Weqi/
// Windows: %APPDATA%/Weqi/
// macOS:   ~/Library/Application Support/Weqi/
class DataPaths
{
public:
    // 应用数据根目录（自动创建）
    static QString appDataDir();

    // 各子目录（自动创建）
    static QString configDir();   // config/
    static QString gamesDir();    // games/
    static QString profileDir();  // profile/
    static QString savesDir();    // saves/

    // 具体文件路径
    static QString settingsFile();      // config/settings.json
    static QString aiProvidersFile();   // config/ai_providers.json
    static QString playerFile();        // profile/player.json
    static QString currentGameFile();   // saves/current_game.json
};
