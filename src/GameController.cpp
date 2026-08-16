#include "GameController.h"

#include "ChessRules.h"

GameController::GameController(QObject *parent)
    : QObject(parent)
{
    m_state.reset();
    m_positionHistory.append(m_state.toFen());
}

bool GameController::isInCheck() const
{
    return ChessRules::isInCheck(m_state, m_state.turn());
}

QVector<Move> GameController::legalMoves(int row, int col) const
{
    return ChessRules::getLegalMoves(m_state, row, col);
}

bool GameController::tryMove(int fromRow, int fromCol, int toRow, int toCol)
{
    if (m_result != Result::Ongoing)
        return false;

    // 找到匹配的合法走法
    const QVector<Move> moves = ChessRules::getLegalMoves(m_state, fromRow, fromCol);
    Move chosen;
    bool found = false;
    for (const Move &m : moves) {
        if (m.toRow == toRow && m.toCol == toCol) {
            chosen = m;
            found = true;
            break;
        }
    }
    if (!found)
        return false;

    // 升变走法：暂停，等待 UI 选择
    if (chosen.isPromotion) {
        m_pendingPromotion = chosen;
        m_hasPendingPromotion = true;
        emit promotionRequired(fromRow, fromCol, toRow, toCol);
        return false; // 尚未完成，UI 需等待
    }

    applyMove(chosen);
    return true;
}

void GameController::completePromotion(PieceType promotion)
{
    if (!m_hasPendingPromotion)
        return;
    m_pendingPromotion.promotion = promotion;
    const Move move = m_pendingPromotion;
    m_hasPendingPromotion = false;
    applyMove(move);
}

void GameController::applyMove(const Move &move)
{
    // 记录走子前状态与 SAN
    const QString san = ChessRules::moveToSan(m_state, move);
    m_history.push(move, m_state, san);

    // 应用走子
    m_state = ChessRules::makeMove(m_state, move);

    // 记录局面（用于重复检测）
    m_positionHistory.append(m_state.toFen());

    emit moveCommitted(move);

    // 检测结果
    evaluateResult();
    emit gameStateChanged();
}

void GameController::evaluateResult()
{
    const PieceColor turn = m_state.turn();
    const PieceColor opponent = (turn == PieceColor::White) ? PieceColor::Black : PieceColor::White;

    // 防御：若对方王已被吃（正常规则下不会发生），直接判定走子方获胜
    int kingRow, kingCol;
    if (!m_state.findKing(opponent, kingRow, kingCol)) {
        m_result = (turn == PieceColor::White) ? Result::WhiteWin : Result::BlackWin;
        m_resultReason = QStringLiteral("王被吃");
        emit gameOver(m_result, m_resultReason);
        return;
    }

    // 将死
    if (ChessRules::isCheckmate(m_state, turn)) {
        m_result = (turn == PieceColor::White) ? Result::BlackWin : Result::WhiteWin;
        m_resultReason = QStringLiteral("将死");
        emit gameOver(m_result, m_resultReason);
        return;
    }

    // 逼和
    if (ChessRules::isStalemate(m_state, turn)) {
        m_result = Result::Draw;
        m_resultReason = QStringLiteral("逼和（无子可动）");
        emit gameOver(m_result, m_resultReason);
        return;
    }

    // 五十回合规则
    if (m_state.halfmoveClock() >= 100) {
        m_result = Result::Draw;
        m_resultReason = QStringLiteral("五十回合规则");
        emit gameOver(m_result, m_resultReason);
        return;
    }

    // 三次重复
    if (isThreefoldRepetition()) {
        m_result = Result::Draw;
        m_resultReason = QStringLiteral("三次重复局面");
        emit gameOver(m_result, m_resultReason);
        return;
    }

    // 无子可胜
    if (isInsufficientMaterial()) {
        m_result = Result::Draw;
        m_resultReason = QStringLiteral("无子可胜");
        emit gameOver(m_result, m_resultReason);
        return;
    }

    m_result = Result::Ongoing;
    m_resultReason.clear();
}

bool GameController::isThreefoldRepetition() const
{
    const QString current = m_state.toFen();
    // 只比较棋子布局 + 回合 + 易位 + 吃过路兵（FEN 前四段），忽略计数
    const QString key = current.section(' ', 0, 3);
    int count = 0;
    for (const QString &fen : m_positionHistory) {
        if (fen.section(' ', 0, 3) == key)
            ++count;
    }
    return count >= 3;
}

bool GameController::isInsufficientMaterial() const
{
    int whiteMinor = 0; // 象或马
    int blackMinor = 0;
    bool whiteBishop = false;
    bool blackBishop = false;
    bool whiteKnight = false;
    bool blackKnight = false;
    int whitePawn = 0;
    int blackPawn = 0;
    int whiteRook = 0;
    int blackRook = 0;
    int whiteQueen = 0;
    int blackQueen = 0;

    for (int r = 0; r < GameState::kBoardSize; ++r) {
        for (int c = 0; c < GameState::kBoardSize; ++c) {
            const ChessPiece &p = m_state.pieceAt(r, c);
            if (!p.isValid())
                continue;
            const bool isWhite = (p.color() == PieceColor::White);
            switch (p.type()) {
            case PieceType::Pawn:
                isWhite ? ++whitePawn : ++blackPawn; break;
            case PieceType::Knight:
                isWhite ? (++whiteMinor, whiteKnight = true) : (++blackMinor, blackKnight = true);
                break;
            case PieceType::Bishop:
                isWhite ? (++whiteMinor, whiteBishop = true) : (++blackMinor, blackBishop = true);
                break;
            case PieceType::Rook:
                isWhite ? ++whiteRook : ++blackRook; break;
            case PieceType::Queen:
                isWhite ? ++whiteQueen : ++blackQueen; break;
            default: break;
            }
        }
    }

    // 王对王
    if (whiteMinor + blackMinor + whitePawn + blackPawn + whiteRook + blackRook
            + whiteQueen + blackQueen == 0)
        return true;

    // 王 + 单象/单马 对 王
    if (whiteMinor == 1 && blackMinor == 0
        && whitePawn + blackPawn + whiteRook + blackRook + whiteQueen + blackQueen == 0)
        return true;
    if (blackMinor == 1 && whiteMinor == 0
        && whitePawn + blackPawn + whiteRook + blackRook + whiteQueen + blackQueen == 0)
        return true;

    // 王 + 象 对 王 + 象（同色格象）
    if (whiteMinor == 1 && blackMinor == 1
        && whiteBishop && blackBishop
        && whitePawn + blackPawn + whiteRook + blackRook + whiteQueen + blackQueen == 0) {
        // 简化：同色格象视为无子可胜（此处不精确判断格色，保守处理）
        return true;
    }

    return false;
}

bool GameController::undo()
{
    if (m_history.isEmpty())
        return false;

    // 撤销最后一步，恢复走子前状态
    const GameState stateBefore = m_history.last()->stateBefore.copy();
    m_history.pop();
    m_state = stateBefore;

    // 移除对应的局面记录
    if (!m_positionHistory.isEmpty())
        m_positionHistory.removeLast();

    // 重置结果
    m_result = Result::Ongoing;
    m_resultReason.clear();
    m_hasPendingPromotion = false;

    emit undoPerformed();
    emit gameStateChanged();
    return true;
}

void GameController::newGame()
{
    m_state.reset();
    m_history.clear();
    m_positionHistory.clear();
    m_positionHistory.append(m_state.toFen());
    m_result = Result::Ongoing;
    m_resultReason.clear();
    m_hasPendingPromotion = false;

    emit gameReset();
    emit gameStateChanged();
}

bool GameController::loadFen(const QString &fen)
{
    GameState restored;
    if (!restored.fromFen(fen))
        return false;

    m_state = restored;
    m_history.clear();
    m_positionHistory.clear();
    m_positionHistory.append(m_state.toFen());
    m_result = Result::Ongoing;
    m_resultReason.clear();
    m_hasPendingPromotion = false;

    emit gameReset();
    emit gameStateChanged();
    return true;
}

QString GameController::statusText() const
{
    if (m_result != Result::Ongoing)
        return m_resultReason;

    if (isInCheck())
        return QStringLiteral("Check!");

    return QStringLiteral("进行中");
}
