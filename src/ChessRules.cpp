#include "ChessRules.h"

#include <QChar>

namespace {

bool inBoard(int row, int col)
{
    return row >= 0 && row < GameState::kBoardSize
        && col >= 0 && col < GameState::kBoardSize;
}

} // namespace

// 判断 (row,col) 是否被 byColor 方的棋子攻击。
// 通过反向扫描：检查该格是否在对方棋子（兵/马/象/车/后/王）的攻击范围内。
bool ChessRules::isSquareAttacked(const GameState &state, int row, int col, PieceColor byColor)
{
    // 兵：对方兵从 (row,col) 的斜前方攻击。若 byColor 是白方，白兵向上（row 减小），
    // 攻击 (row,col) 的白兵位于 (row+1, col±1)。
    const int pawnDir = (byColor == PieceColor::White) ? -1 : 1;
    for (int dc : {-1, 1}) {
        const int r = row + pawnDir;
        const int c = col + dc;
        if (inBoard(r, c)) {
            const ChessPiece &p = state.pieceAt(r, c);
            if (p.isValid() && p.type() == PieceType::Pawn && p.color() == byColor)
                return true;
        }
    }

    // 马
    const int knightDr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
    const int knightDc[8] = {-1, 1, -2, 2, -2, 2, -1, 1};
    for (int i = 0; i < 8; ++i) {
        const int r = row + knightDr[i];
        const int c = col + knightDc[i];
        if (inBoard(r, c)) {
            const ChessPiece &p = state.pieceAt(r, c);
            if (p.isValid() && p.type() == PieceType::Knight && p.color() == byColor)
                return true;
        }
    }

    // 王（相邻一格）
    const int kingDr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    const int kingDc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    for (int i = 0; i < 8; ++i) {
        const int r = row + kingDr[i];
        const int c = col + kingDc[i];
        if (inBoard(r, c)) {
            const ChessPiece &p = state.pieceAt(r, c);
            if (p.isValid() && p.type() == PieceType::King && p.color() == byColor)
                return true;
        }
    }

    // 滑动棋子：象/后（对角线），车/后（直线）
    const int diagDr[4] = {-1, -1, 1, 1};
    const int diagDc[4] = {-1, 1, -1, 1};
    for (int i = 0; i < 4; ++i) {
        int r = row + diagDr[i];
        int c = col + diagDc[i];
        while (inBoard(r, c)) {
            const ChessPiece &p = state.pieceAt(r, c);
            if (p.isValid()) {
                if (p.color() == byColor
                    && (p.type() == PieceType::Bishop || p.type() == PieceType::Queen))
                    return true;
                break; // 被棋子挡住
            }
            r += diagDr[i];
            c += diagDc[i];
        }
    }

    const int straightDr[4] = {-1, 1, 0, 0};
    const int straightDc[4] = {0, 0, -1, 1};
    for (int i = 0; i < 4; ++i) {
        int r = row + straightDr[i];
        int c = col + straightDc[i];
        while (inBoard(r, c)) {
            const ChessPiece &p = state.pieceAt(r, c);
            if (p.isValid()) {
                if (p.color() == byColor
                    && (p.type() == PieceType::Rook || p.type() == PieceType::Queen))
                    return true;
                break;
            }
            r += straightDr[i];
            c += straightDc[i];
        }
    }

    return false;
}

bool ChessRules::isInCheck(const GameState &state, PieceColor color)
{
    int kingRow, kingCol;
    if (!state.findKing(color, kingRow, kingCol))
        return false;
    const PieceColor opponent = (color == PieceColor::White) ? PieceColor::Black : PieceColor::White;
    return isSquareAttacked(state, kingRow, kingCol, opponent);
}

// 生成 (row,col) 处棋子的伪合法走法（未过滤王安全）。
QVector<Move> ChessRules::generatePseudoLegalMoves(const GameState &state, int row, int col)
{
    QVector<Move> moves;
    const ChessPiece &piece = state.pieceAt(row, col);
    if (!piece.isValid())
        return moves;

    const PieceColor color = piece.color();
    const PieceColor opponent = (color == PieceColor::White) ? PieceColor::Black : PieceColor::White;

    auto addMove = [&](int tr, int tc, bool isCapture, PieceType capturedType,
                       int capRow, int capCol) {
        Move m;
        m.fromRow = row;
        m.fromCol = col;
        m.toRow = tr;
        m.toCol = tc;
        m.piece = piece.type();
        m.color = color;
        if (isCapture) {
            m.captured = capturedType;
            m.capturedRow = capRow;
            m.capturedCol = capCol;
        }
        moves.append(m);
    };

    switch (piece.type()) {
    case PieceType::Pawn: {
        const int dir = (color == PieceColor::White) ? -1 : 1;
        const int startRow = (color == PieceColor::White) ? 6 : 1;
        const int promoRow = (color == PieceColor::White) ? 0 : 7;

        // 前进一格
        const int r1 = row + dir;
        if (inBoard(r1, col) && !state.pieceAt(r1, col).isValid()) {
            if (r1 == promoRow) {
                // 升变：生成四种升变走法
                for (PieceType pt : {PieceType::Queen, PieceType::Rook,
                                     PieceType::Bishop, PieceType::Knight}) {
                    Move m;
                    m.fromRow = row; m.fromCol = col;
                    m.toRow = r1; m.toCol = col;
                    m.piece = PieceType::Pawn; m.color = color;
                    m.isPromotion = true; m.promotion = pt;
                    moves.append(m);
                }
            } else {
                addMove(r1, col, false, PieceType::None, -1, -1);
            }
            // 起始位置前进两格
            if (row == startRow) {
                const int r2 = row + 2 * dir;
                if (inBoard(r2, col) && !state.pieceAt(r2, col).isValid()
                    && !state.pieceAt(r1, col).isValid()) {
                    addMove(r2, col, false, PieceType::None, -1, -1);
                }
            }
        }

        // 斜向吃子
        for (int dc : {-1, 1}) {
            const int r = row + dir;
            const int c = col + dc;
            if (!inBoard(r, c))
                continue;
            const ChessPiece &target = state.pieceAt(r, c);
            if (target.isValid() && target.color() == opponent) {
                if (r == promoRow) {
                    for (PieceType pt : {PieceType::Queen, PieceType::Rook,
                                         PieceType::Bishop, PieceType::Knight}) {
                        Move m;
                        m.fromRow = row; m.fromCol = col;
                        m.toRow = r; m.toCol = c;
                        m.piece = PieceType::Pawn; m.color = color;
                        m.isPromotion = true; m.promotion = pt;
                        m.captured = target.type();
                        m.capturedRow = r; m.capturedCol = c;
                        moves.append(m);
                    }
                } else {
                    addMove(r, c, true, target.type(), r, c);
                }
            }
            // 吃过路兵：过路兵在 (row + dir, c)，兵斜向落到其越过的空格 (row + 2*dir, c)。
            // 与普通斜向吃子不同，目标格是过路兵身后的空格，故独立判断（可能同时生成两种走法）。
            if (state.enPassantRow() == row + 2 * dir && state.enPassantCol() == c) {
                const int destRow = row + 2 * dir;
                if (inBoard(destRow, c) && !state.pieceAt(destRow, c).isValid()) {
                    const ChessPiece &epPawn = state.pieceAt(r, c);
                    if (epPawn.isValid() && epPawn.type() == PieceType::Pawn
                        && epPawn.color() == opponent) {
                        Move m;
                        m.fromRow = row; m.fromCol = col;
                        m.toRow = destRow; m.toCol = c;
                        m.piece = PieceType::Pawn; m.color = color;
                        m.isEnPassant = true;
                        m.captured = PieceType::Pawn;
                        m.capturedRow = r; m.capturedCol = c; // 过路兵所在格
                        moves.append(m);
                    }
                }
            }
        }
        break;
    }

    case PieceType::Knight: {
        const int dr[8] = {-2, -2, -1, -1, 1, 1, 2, 2};
        const int dc[8] = {-1, 1, -2, 2, -2, 2, -1, 1};
        for (int i = 0; i < 8; ++i) {
            const int r = row + dr[i];
            const int c = col + dc[i];
            if (!inBoard(r, c))
                continue;
            const ChessPiece &target = state.pieceAt(r, c);
            if (!target.isValid())
                addMove(r, c, false, PieceType::None, -1, -1);
            else if (target.color() == opponent)
                addMove(r, c, true, target.type(), r, c);
        }
        break;
    }

    case PieceType::Bishop:
    case PieceType::Rook:
    case PieceType::Queen: {
        int dirs[8][2];
        int n = 0;
        if (piece.type() == PieceType::Bishop || piece.type() == PieceType::Queen) {
            dirs[n][0] = -1; dirs[n][1] = -1; ++n;
            dirs[n][0] = -1; dirs[n][1] =  1; ++n;
            dirs[n][0] =  1; dirs[n][1] = -1; ++n;
            dirs[n][0] =  1; dirs[n][1] =  1; ++n;
        }
        if (piece.type() == PieceType::Rook || piece.type() == PieceType::Queen) {
            dirs[n][0] = -1; dirs[n][1] =  0; ++n;
            dirs[n][0] =  1; dirs[n][1] =  0; ++n;
            dirs[n][0] =  0; dirs[n][1] = -1; ++n;
            dirs[n][0] =  0; dirs[n][1] =  1; ++n;
        }
        for (int i = 0; i < n; ++i) {
            int r = row + dirs[i][0];
            int c = col + dirs[i][1];
            while (inBoard(r, c)) {
                const ChessPiece &target = state.pieceAt(r, c);
                if (!target.isValid()) {
                    addMove(r, c, false, PieceType::None, -1, -1);
                } else {
                    if (target.color() == opponent)
                        addMove(r, c, true, target.type(), r, c);
                    break;
                }
                r += dirs[i][0];
                c += dirs[i][1];
            }
        }
        break;
    }

    case PieceType::King: {
        const int dr[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        const int dc[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int i = 0; i < 8; ++i) {
            const int r = row + dr[i];
            const int c = col + dc[i];
            if (!inBoard(r, c))
                continue;
            const ChessPiece &target = state.pieceAt(r, c);
            if (!target.isValid())
                addMove(r, c, false, PieceType::None, -1, -1);
            else if (target.color() == opponent)
                addMove(r, c, true, target.type(), r, c);
        }

        // 王车易位
        const int kingStartCol = 4;
        if (row == (color == PieceColor::White ? 7 : 0) && col == kingStartCol) {
            const PieceColor opponent2 = opponent;
            // 王翼易位 O-O
            if (state.canCastleKingside(color)) {
                const int rookCol = 7;
                const ChessPiece &rook = state.pieceAt(row, rookCol);
                const bool pathClear = !state.pieceAt(row, 5).isValid()
                                    && !state.pieceAt(row, 6).isValid();
                if (rook.isValid() && rook.type() == PieceType::Rook
                    && rook.color() == color && pathClear
                    && !isSquareAttacked(state, row, kingStartCol, opponent2)
                    && !isSquareAttacked(state, row, 5, opponent2)
                    && !isSquareAttacked(state, row, 6, opponent2)) {
                    Move m;
                    m.fromRow = row; m.fromCol = kingStartCol;
                    m.toRow = row; m.toCol = 6;
                    m.piece = PieceType::King; m.color = color;
                    m.isCastleKingside = true;
                    moves.append(m);
                }
            }
            // 后翼易位 O-O-O
            if (state.canCastleQueenside(color)) {
                const int rookCol = 0;
                const ChessPiece &rook = state.pieceAt(row, rookCol);
                const bool pathClear = !state.pieceAt(row, 1).isValid()
                                    && !state.pieceAt(row, 2).isValid()
                                    && !state.pieceAt(row, 3).isValid();
                if (rook.isValid() && rook.type() == PieceType::Rook
                    && rook.color() == color && pathClear
                    && !isSquareAttacked(state, row, kingStartCol, opponent2)
                    && !isSquareAttacked(state, row, 3, opponent2)
                    && !isSquareAttacked(state, row, 2, opponent2)) {
                    Move m;
                    m.fromRow = row; m.fromCol = kingStartCol;
                    m.toRow = row; m.toCol = 2;
                    m.piece = PieceType::King; m.color = color;
                    m.isCastleQueenside = true;
                    moves.append(m);
                }
            }
        }
        break;
    }

    default:
        break;
    }

    return moves;
}

// 判断一步棋是否合法：模拟走子后，自己的王不能被将军。
// 国际象棋中王不能被吃：目标格是对方王则非法（只能通过将死获胜）。
bool ChessRules::isMoveLegal(const GameState &state, const Move &move)
{
    const ChessPiece &target = state.pieceAt(move.toRow, move.toCol);
    if (target.isValid() && target.type() == PieceType::King
        && target.color() != move.color)
        return false;

    const GameState after = makeMove(state, move);
    return !isInCheck(after, move.color);
}

QVector<Move> ChessRules::getLegalMoves(const GameState &state, int row, int col)
{
    QVector<Move> result;
    const ChessPiece &piece = state.pieceAt(row, col);
    if (!piece.isValid() || piece.color() != state.turn())
        return result;

    const QVector<Move> pseudo = generatePseudoLegalMoves(state, row, col);
    for (const Move &m : pseudo) {
        if (isMoveLegal(state, m))
            result.append(m);
    }
    return result;
}

QVector<Move> ChessRules::getAllLegalMoves(const GameState &state, PieceColor color)
{
    QVector<Move> result;
    for (int r = 0; r < GameState::kBoardSize; ++r) {
        for (int c = 0; c < GameState::kBoardSize; ++c) {
            const ChessPiece &p = state.pieceAt(r, c);
            if (p.isValid() && p.color() == color) {
                const QVector<Move> moves = getLegalMoves(state, r, c);
                result += moves;
            }
        }
    }
    return result;
}

bool ChessRules::hasNoLegalMoves(const GameState &state, PieceColor color)
{
    return getAllLegalMoves(state, color).isEmpty();
}

bool ChessRules::isCheckmate(const GameState &state, PieceColor color)
{
    return isInCheck(state, color) && hasNoLegalMoves(state, color);
}

bool ChessRules::isStalemate(const GameState &state, PieceColor color)
{
    return !isInCheck(state, color) && hasNoLegalMoves(state, color);
}

// 应用一步棋，返回新状态。
GameState ChessRules::makeMove(const GameState &state, const Move &move)
{
    GameState next = state.copy();

    const ChessPiece moving = next.pieceAt(move.fromRow, move.fromCol);

    // 清除上一手的吃过路兵目标格
    next.clearEnPassant();

    // 半回合计数：兵移动或吃子时重置
    if (moving.type() == PieceType::Pawn || move.captured != PieceType::None)
        next.setHalfmoveClock(0);
    else
        next.setHalfmoveClock(state.halfmoveClock() + 1);

    // 王车易位
    if (move.isCastleKingside) {
        const int row = move.fromRow;
        // 王移动到 g 列（col 6）
        next.setPiece(row, 6, moving);
        next.setPiece(row, 4, ChessPiece());
        // 车从 h 列（col 7）移动到 f 列（col 5）
        const ChessPiece rook = next.pieceAt(row, 7);
        next.setPiece(row, 5, rook);
        next.setPiece(row, 7, ChessPiece());
    } else if (move.isCastleQueenside) {
        const int row = move.fromRow;
        // 王移动到 c 列（col 2）
        next.setPiece(row, 2, moving);
        next.setPiece(row, 4, ChessPiece());
        // 车从 a 列（col 0）移动到 d 列（col 3）
        const ChessPiece rook = next.pieceAt(row, 0);
        next.setPiece(row, 3, rook);
        next.setPiece(row, 0, ChessPiece());
    }
    // 吃过路兵
    else if (move.isEnPassant) {
        // 移动兵到目标格
        next.setPiece(move.toRow, move.toCol, moving);
        next.setPiece(move.fromRow, move.fromCol, ChessPiece());
        // 移除过路兵（位于 capturedRow, capturedCol）
        next.setPiece(move.capturedRow, move.capturedCol, ChessPiece());
    }
    // 升变
    else if (move.isPromotion) {
        next.setPiece(move.toRow, move.toCol, ChessPiece(move.promotion, moving.color()));
        next.setPiece(move.fromRow, move.fromCol, ChessPiece());
    }
    // 普通走子（含吃子）
    else {
        next.setPiece(move.toRow, move.toCol, moving);
        next.setPiece(move.fromRow, move.fromCol, ChessPiece());
    }

    // 设置吃过路兵目标格：兵从起始位置前进两格
    if (moving.type() == PieceType::Pawn
        && qAbs(move.toRow - move.fromRow) == 2) {
        const int epRow = (move.fromRow + move.toRow) / 2;
        next.setEnPassant(epRow, move.fromCol);
    }

    // 更新易位权限：王或车移动后失去对应权限
    if (moving.type() == PieceType::King) {
        next.setCastlingRights(moving.color(), false, false);
    }
    if (moving.type() == PieceType::Rook) {
        const int row = move.fromRow;
        const int col = move.fromCol;
        if (moving.color() == PieceColor::White) {
            if (row == 7 && col == 0) next.setCastlingRights(PieceColor::White, false, false);
            if (row == 7 && col == 7) next.setCastlingRights(PieceColor::White, false, false);
        } else {
            if (row == 0 && col == 0) next.setCastlingRights(PieceColor::Black, false, false);
            if (row == 0 && col == 7) next.setCastlingRights(PieceColor::Black, false, false);
        }
    }
    // 车被吃掉时也失去对应权限
    if (move.captured == PieceType::Rook) {
        const int row = move.toRow;
        const int col = move.toCol;
        const PieceColor capturedColor = (moving.color() == PieceColor::White)
                                             ? PieceColor::Black : PieceColor::White;
        if (capturedColor == PieceColor::White) {
            if (row == 7 && col == 0) next.setCastlingRights(PieceColor::White, false, false);
            if (row == 7 && col == 7) next.setCastlingRights(PieceColor::White, false, false);
        } else {
            if (row == 0 && col == 0) next.setCastlingRights(PieceColor::Black, false, false);
            if (row == 0 && col == 7) next.setCastlingRights(PieceColor::Black, false, false);
        }
    }

    // 切换回合
    next.setTurn((state.turn() == PieceColor::White) ? PieceColor::Black : PieceColor::White);

    // 完整回合数：黑方走完后递增
    if (state.turn() == PieceColor::Black)
        next.setFullmoveNumber(state.fullmoveNumber() + 1);

    return next;
}

// 生成一步棋的代数记谱（SAN）。
QString ChessRules::moveToSan(const GameState &state, const Move &move)
{
    Q_UNUSED(state);
    if (move.isCastleKingside)
        return QStringLiteral("O-O");
    if (move.isCastleQueenside)
        return QStringLiteral("O-O-O");

    QString san;

    // 棋子字母（兵无字母）
    if (move.piece != PieceType::Pawn) {
        switch (move.piece) {
        case PieceType::Knight: san += 'N'; break;
        case PieceType::Bishop: san += 'B'; break;
        case PieceType::Rook:   san += 'R'; break;
        case PieceType::Queen:  san += 'Q'; break;
        case PieceType::King:   san += 'K'; break;
        default: break;
        }
    }

    // 吃子标记
    if (move.captured != PieceType::None) {
        if (move.piece == PieceType::Pawn)
            san += QChar('a' + move.fromCol); // 兵吃子需标注来源列
        san += 'x';
    }

    // 目标格
    san += QChar('a' + move.toCol);
    san += QString::number(8 - move.toRow);

    // 升变
    if (move.isPromotion) {
        san += '=';
        switch (move.promotion) {
        case PieceType::Queen:  san += 'Q'; break;
        case PieceType::Rook:   san += 'R'; break;
        case PieceType::Bishop: san += 'B'; break;
        case PieceType::Knight: san += 'N'; break;
        default: break;
        }
    }

    return san;
}
