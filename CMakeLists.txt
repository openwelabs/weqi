cmake_minimum_required(VERSION 3.16)

project(Weqi
    VERSION 0.1.0
    DESCRIPTION "A modern open-source desktop chess application"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# 现代、简洁的编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra)
endif()

find_package(Qt6 6.2 REQUIRED COMPONENTS Widgets LinguistTools)

qt_add_executable(Weqi
    src/main.cpp
    src/MainWindow.cpp
    src/MainWindow.h
    src/ChessBoard.cpp
    src/ChessBoard.h
    src/ChessPiece.cpp
    src/ChessPiece.h
    src/GameState.cpp
    src/GameState.h
    src/ChessRules.cpp
    src/ChessRules.h
    src/Move.h
    src/MoveHistory.cpp
    src/MoveHistory.h
    src/GameController.cpp
    src/GameController.h
    src/ui/UiTheme.cpp
    src/ui/UiTheme.h
    src/pages/Page.cpp
    src/pages/Page.h
    src/pages/HomePage.cpp
    src/pages/HomePage.h
    src/pages/NewGamePage.cpp
    src/pages/NewGamePage.h
    src/pages/AIOpponentPage.cpp
    src/pages/AIOpponentPage.h
    src/pages/AIVsAIPage.cpp
    src/pages/AIVsAIPage.h
    src/pages/GamePage.cpp
    src/pages/GamePage.h
    src/pages/HistoryPage.cpp
    src/pages/HistoryPage.h
    src/pages/SettingsPage.cpp
    src/pages/SettingsPage.h
    src/pages/AboutPage.cpp
    src/pages/AboutPage.h
    src/data/GameRecord.cpp
    src/data/GameRecord.h
    src/data/DataPaths.cpp
    src/data/DataPaths.h
    src/data/ProfileManager.cpp
    src/data/ProfileManager.h
    src/data/SettingsManager.cpp
    src/data/SettingsManager.h
    src/data/LanguageManager.cpp
    src/data/LanguageManager.h
    src/data/AIProviderManager.cpp
    src/data/AIProviderManager.h
    src/data/GameHistoryManager.cpp
    src/data/GameHistoryManager.h
    src/data/StatsManager.cpp
    src/data/StatsManager.h
    src/data/RatingSystem.cpp
    src/data/RatingSystem.h
    src/ai/AIManager.cpp
    src/ai/AIManager.h
    resources/resources.qrc
)

target_link_libraries(Weqi PRIVATE Qt6::Widgets)

# ---- 国际化 ----
# 用 lupdate 从源码提取 tr() 字符串生成 .ts，lrelease 编译为 .qm，
# 并嵌入到可执行文件资源（:/i18n/weqi_<lang>.qm）。
set(TS_FILES
    translations/weqi_zh_CN.ts
    translations/weqi_zh_TW.ts
    translations/weqi_en.ts
    translations/weqi_ja.ts
    translations/weqi_es.ts
    translations/weqi_uk.ts
    translations/weqi_ko.ts
)

qt_add_translations(Weqi
    TS_FILES ${TS_FILES}
    RESOURCE_PREFIX "/i18n"
    LUPDATE_OPTIONS -no-obsolete
)

# 头文件搜索路径（data/ 与 ui/ 子目录）
target_include_directories(Weqi PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/data
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/pages
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ai
)

# 让程序在 Linux 上正确显示窗口图标
set_target_properties(Weqi PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
