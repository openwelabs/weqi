#include "MoveHistory.h"

void MoveHistory::clear()
{
    m_records.clear();
}

bool MoveHistory::isEmpty() const
{
    return m_records.isEmpty();
}

int MoveHistory::count() const
{
    return m_records.size();
}

void MoveHistory::push(const Move &move, const GameState &stateBefore, const QString &san)
{
    MoveRecord rec;
    rec.move = move;
    rec.stateBefore = stateBefore.copy();
    rec.san = san;
    m_records.append(rec);
}

Move MoveHistory::pop()
{
    if (m_records.isEmpty())
        return Move();
    const MoveRecord rec = m_records.takeLast();
    return rec.move;
}

const MoveRecord *MoveHistory::last() const
{
    if (m_records.isEmpty())
        return nullptr;
    return &m_records.last();
}

const MoveRecord &MoveHistory::at(int index) const
{
    return m_records.at(index);
}

QString MoveHistory::toNotation() const
{
    QString text;
    for (int i = 0; i < m_records.size(); ++i) {
        const MoveRecord &rec = m_records.at(i);
        if (rec.move.color == PieceColor::White) {
            if (!text.isEmpty())
                text += ' ';
            text += QString::number(rec.move.fromRow >= 0
                                        ? (i / 2) + 1
                                        : 1);
            text += ". ";
            text += rec.san;
        } else {
            text += ' ';
            text += rec.san;
        }
    }
    return text;
}

QString MoveHistory::toPgn() const
{
    // 预留：后续可扩展为完整 PGN（含头信息、结果等）
    return toNotation();
}
