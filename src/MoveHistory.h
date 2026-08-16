#pragma once

#include <QVector>
#include <QString>

#include "GameState.h"
#include "Move.h"

// 一步棋及其走子前的状态快照。
// 保存完整状态以便 Undo 时精确恢复（棋子位置、回合、易位权限、吃过路兵、计数）。
struct MoveRecord
{
    Move move;
    GameState stateBefore; // 走子前的完整状态
    QString san;           // 代数记谱
};

// 棋局历史：记录每一步、支持 Undo、生成棋谱文本。
// 为以后 PGN 导出、AI 复盘、AI 分析预留接口。
class MoveHistory
{
public:
    void clear();
    bool isEmpty() const;
    int count() const;

    // 追加一步（记录走子前状态与 SAN）
    void push(const Move &move, const GameState &stateBefore, const QString &san);

    // 撤销最后一步，返回被撤销的走法；无历史时返回无效 Move
    Move pop();

    // 最近一步（用于高亮）
    const MoveRecord *last() const;

    // 按索引访问（用于棋谱面板显示）
    const MoveRecord &at(int index) const;

    // 生成棋谱文本（如 "1. e4 e5 2. Nf3 Nc6"）
    QString toNotation() const;

    // 生成 PGN 格式文本（预留接口）
    QString toPgn() const;

private:
    QVector<MoveRecord> m_records;
};
