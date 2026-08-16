#include "GameController.h"

#include "ChessRules.h"
#include "AIManager.h"

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

    // AI 思考期间禁止玩家走子
    if (m_aiThinking)
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

    // 若对局未结束且下一回合为 AI，触发 AI 请求
    if (m_result == Result::Ongoing)
        maybeTriggerAI();
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

    // 若 AI 正在思考，先取消
    if (m_aiThinking)
        cancelAI();

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

    // 若撤销后当前回合为 AI，重新触发
    if (m_result == Result::Ongoing)
        maybeTriggerAI();
    return true;
}

void GameController::newGame()
{
    // 若 AI 正在思考，先取消
    if (m_aiThinking)
        cancelAI();

    m_state.reset();
    m_history.clear();
    m_positionHistory.clear();
    m_positionHistory.append(m_state.toFen());
    m_result = Result::Ongoing;
    m_resultReason.clear();
    m_hasPendingPromotion = false;
    m_aiRetryCount = 0;

    emit gameReset();
    emit gameStateChanged();

    // 若白方为 AI，触发 AI 走第一步
    if (m_result == Result::Ongoing)
        maybeTriggerAI();
}

bool GameController::loadFen(const QString &fen)
{
    GameState restored;
    if (!restored.fromFen(fen))
        return false;

    // 若 AI 正在思考，先取消
    if (m_aiThinking)
        cancelAI();

    m_state = restored;
    m_history.clear();
    m_positionHistory.clear();
    m_positionHistory.append(m_state.toFen());
    m_result = Result::Ongoing;
    m_resultReason.clear();
    m_hasPendingPromotion = false;
    m_aiRetryCount = 0;

    emit gameReset();
    emit gameStateChanged();

    // 若当前回合为 AI，触发 AI 走法
    if (m_result == Result::Ongoing)
        maybeTriggerAI();
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

// ---- AI 集成 ----

void GameController::setAIManager(AIManager *manager)
{
    if (m_aiManager == manager)
        return;

    if (m_aiManager) {
        disconnect(m_aiManager, nullptr, this, nullptr);
    }

    m_aiManager = manager;
    if (m_aiManager) {
        connect(m_aiManager, &AIManager::moveReady,
                this, &GameController::onAIMoveReady);
        connect(m_aiManager, &AIManager::failed,
                this, &GameController::onAIFailed);
    }
}

void GameController::setAIGame(bool whiteIsAI, bool blackIsAI,
                               const AIProvider &whiteProvider,
                               const AIProvider &blackProvider)
{
    m_whiteIsAI = whiteIsAI;
    m_blackIsAI = blackIsAI;
    m_whiteProvider = whiteProvider;
    m_blackProvider = blackProvider;
    m_aiRetryCount = 0;
    m_aiThinking = false;
    m_aiThinkingName.clear();
    m_aiThinkingModel.clear();
}

bool GameController::isCurrentTurnAI() const
{
    if (m_result != Result::Ongoing)
        return false;
    if (m_state.turn() == PieceColor::White)
        return m_whiteIsAI;
    return m_blackIsAI;
}

void GameController::cancelAI()
{
    if (m_aiManager)
        m_aiManager->cancel();
    m_aiThinking = false;
    m_aiThinkingName.clear();
    m_aiThinkingModel.clear();
    m_aiRetryCount = 0;
    emit aiThinkingChanged(false, QString(), QString());
}

void GameController::retryAI()
{
    // 对局已结束或当前回合不是 AI，则不重试
    if (m_result != Result::Ongoing || !isCurrentTurnAI())
        return;
    if (!m_aiManager)
        return;

    // 重置重试计数并重新触发
    m_aiRetryCount = 0;
    maybeTriggerAI();
}

void GameController::maybeTriggerAI()
{
    // 对局已结束或当前回合不是 AI，则不触发
    if (m_result != Result::Ongoing || !isCurrentTurnAI())
        return;
    if (!m_aiManager)
        return;

    const AIProvider *provider = currentAIProvider();
    if (!provider)
        return;

    // 收集已走的 UCI 走法
    QStringList uciHistory;
    for (int i = 0; i < m_history.count(); ++i) {
        const MoveRecord &rec = m_history.at(i);
        uciHistory.append(moveToUci(rec.move));
    }

    // 生成当前方所有合法走法（UCI 格式），AI 必须从中选择一个
    QStringList legalMoves;
    const QVector<Move> allLegal = ChessRules::getAllLegalMoves(m_state, m_state.turn());
    for (const Move &m : allLegal)
        legalMoves.append(moveToUci(m));

    const QString turn = (m_state.turn() == PieceColor::White)
                             ? QStringLiteral("white")
                             : QStringLiteral("black");

    // 设置思考状态
    m_aiThinking = true;
    m_aiThinkingName = provider->name;
    m_aiThinkingModel = provider->model;
    emit aiThinkingChanged(true, m_aiThinkingName, m_aiThinkingModel);

    // 发起异步请求
    if (!m_aiManager->requestMove(*provider, m_state.toFen(), turn, uciHistory, legalMoves)) {
        // 请求发起失败（如 Python 未安装）
        m_aiThinking = false;
        m_aiThinkingName.clear();
        m_aiThinkingModel.clear();
        emit aiThinkingChanged(false, QString(), QString());
        emit aiRequestFailed(QStringLiteral("无法启动 AI 请求"));
    }
}

void GameController::onAIMoveReady(const QString &uciMove)
{
    m_aiThinking = false;
    m_aiThinkingName.clear();
    m_aiThinkingModel.clear();
    emit aiThinkingChanged(false, QString(), QString());

    // 对局已结束，忽略
    if (m_result != Result::Ongoing)
        return;

    // 将 UCI 走法转换为 Move 结构；若失败，尝试按短代数记法（SAN）解析
    Move move = uciToMove(uciMove);
    if (!move.isValid())
        move = resolveMoveFromSan(uciMove);
    if (!move.isValid()) {
        // 非法走法：重试
        ++m_aiRetryCount;
        if (m_aiRetryCount >= 3) {
            emit aiRequestFailed(QStringLiteral("AI 返回了无效的走法。"));
            m_aiRetryCount = 0;
            return;
        }
        maybeTriggerAI();
        return;
    }

    // 验证走法是否合法（通过 ChessRules）
    const QVector<Move> legalMoves = ChessRules::getLegalMoves(m_state, move.fromRow, move.fromCol);
    bool isLegal = false;
    for (const Move &m : legalMoves) {
        if (m.toRow == move.toRow && m.toCol == move.toCol) {
            // 升变走法需匹配升变目标
            if (move.isPromotion && m.promotion != move.promotion)
                continue;
            isLegal = true;
            break;
        }
    }

    if (!isLegal) {
        // 非法走法：重试
        ++m_aiRetryCount;
        if (m_aiRetryCount >= 3) {
            emit aiRequestFailed(QStringLiteral("AI 返回了无效的走法。"));
            m_aiRetryCount = 0;
            return;
        }
        maybeTriggerAI();
        return;
    }

    // 合法走法：执行
    m_aiRetryCount = 0;
    applyMove(move);
}

void GameController::onAIFailed(const QString &error)
{
    m_aiThinking = false;
    m_aiThinkingName.clear();
    m_aiThinkingModel.clear();
    emit aiThinkingChanged(false, QString(), QString());
    emit aiRequestFailed(error);
}

Move GameController::uciToMove(const QString &uci) const
{
    Move move;
    if (uci.size() < 4)
        return move;

    const QChar f1 = uci.at(0);
    const QChar r1 = uci.at(1);
    const QChar f2 = uci.at(2);
    const QChar r2 = uci.at(3);

    if (f1 < 'a' || f1 > 'h' || r1 < '1' || r1 > '8'
        || f2 < 'a' || f2 > 'h' || r2 < '1' || r2 > '8')
        return move;

    move.fromCol = f1.toLatin1() - 'a';
    move.fromRow = 7 - (r1.toLatin1() - '1');
    move.toCol = f2.toLatin1() - 'a';
    move.toRow = 7 - (r2.toLatin1() - '1');

    // 升变
    if (uci.size() >= 5) {
        const QChar promo = uci.at(4).toLower();
        switch (promo.toLatin1()) {
        case 'q': move.promotion = PieceType::Queen;  move.isPromotion = true; break;
        case 'r': move.promotion = PieceType::Rook;   move.isPromotion = true; break;
        case 'b': move.promotion = PieceType::Bishop; move.isPromotion = true; break;
        case 'n': move.promotion = PieceType::Knight; move.isPromotion = true; break;
        default: break;
        }
    }

    return move;
}

QString GameController::moveToUci(const Move &move) const
{
    QString uci;
    uci += QChar('a' + move.fromCol);
    uci += QChar('1' + (7 - move.fromRow));
    uci += QChar('a' + move.toCol);
    uci += QChar('1' + (7 - move.toRow));
    if (move.isPromotion) {
        switch (move.promotion) {
        case PieceType::Queen:  uci += 'q'; break;
        case PieceType::Rook:   uci += 'r'; break;
        case PieceType::Bishop: uci += 'b'; break;
        case PieceType::Knight: uci += 'n'; break;
        default: break;
        }
    }
    return uci;
}

Move GameController::resolveMoveFromSan(const QString &san) const
{
    if (san.isEmpty())
        return Move();

    // 规范化输入：转小写、去掉空格、去掉走法序号（如 "1."、"12."）、
    // 去掉将军/将杀/评注符号（+ # ! ?）、去掉升变等号。
    QString norm = san.toLower();
    norm.remove(QChar(' '));
    norm.remove(QChar('.'));
    norm.remove(QChar('+'));
    norm.remove(QChar('#'));
    norm.remove(QChar('!'));
    norm.remove(QChar('?'));
    norm.remove(QChar('='));

    // 生成当前回合所有合法走法，逐一转成 SAN 后匹配
    const QVector<Move> legalMoves = ChessRules::getAllLegalMoves(m_state, m_state.turn());
    for (const Move &m : legalMoves) {
        QString sanOfMove = ChessRules::moveToSan(m_state, m).toLower();
        sanOfMove.remove(QChar('='));
        if (sanOfMove == norm)
            return m;
    }

    return Move();
}

const AIProvider *GameController::currentAIProvider() const
{
    if (m_state.turn() == PieceColor::White)
        return m_whiteIsAI ? &m_whiteProvider : nullptr;
    return m_blackIsAI ? &m_blackProvider : nullptr;
}
