#include "GameState.h"

GameState::GameState()
    : m_squares(kBoardSize * kBoardSize)
{
    reset();
}

void GameState::reset()
{
    for (auto &p : m_squares)
        p = ChessPiece();

    // 后排棋子顺序：车 马 象 后 王 象 马 车
    const PieceType backRank[8] = {
        PieceType::Rook, PieceType::Knight, PieceType::Bishop, PieceType::Queen,
        PieceType::King, PieceType::Bishop, PieceType::Knight, PieceType::Rook
    };

    // 黑方在顶部（row 0,1），白方在底部（row 6,7）
    for (int col = 0; col < kBoardSize; ++col) {
        m_squares[0 * kBoardSize + col] = ChessPiece(backRank[col], PieceColor::Black);
        m_squares[1 * kBoardSize + col] = ChessPiece(PieceType::Pawn, PieceColor::Black);
        m_squares[6 * kBoardSize + col] = ChessPiece(PieceType::Pawn, PieceColor::White);
        m_squares[7 * kBoardSize + col] = ChessPiece(backRank[col], PieceColor::White);
    }

    m_turn = PieceColor::White;
    m_castleWK = true;
    m_castleWQ = true;
    m_castleBK = true;
    m_castleBQ = true;
    m_enPassantRow = -1;
    m_enPassantCol = -1;
    m_halfmoveClock = 0;
    m_fullmoveNumber = 1;
}

ChessPiece GameState::pieceAt(int row, int col) const
{
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize)
        return ChessPiece();
    return m_squares[row * kBoardSize + col];
}

void GameState::setPiece(int row, int col, const ChessPiece &piece)
{
    if (row < 0 || row >= kBoardSize || col < 0 || col >= kBoardSize)
        return;
    m_squares[row * kBoardSize + col] = piece;
}

bool GameState::canCastleKingside(PieceColor color) const
{
    return color == PieceColor::White ? m_castleWK : m_castleBK;
}

bool GameState::canCastleQueenside(PieceColor color) const
{
    return color == PieceColor::White ? m_castleWQ : m_castleBQ;
}

void GameState::setCastlingRights(PieceColor color, bool kingside, bool queenside)
{
    if (color == PieceColor::White) {
        m_castleWK = kingside;
        m_castleWQ = queenside;
    } else {
        m_castleBK = kingside;
        m_castleBQ = queenside;
    }
}

void GameState::setEnPassant(int row, int col)
{
    m_enPassantRow = row;
    m_enPassantCol = col;
}

void GameState::clearEnPassant()
{
    m_enPassantRow = -1;
    m_enPassantCol = -1;
}

bool GameState::findKing(PieceColor color, int &row, int &col) const
{
    for (int r = 0; r < kBoardSize; ++r) {
        for (int c = 0; c < kBoardSize; ++c) {
            const ChessPiece &p = m_squares[r * kBoardSize + c];
            if (p.isValid() && p.type() == PieceType::King && p.color() == color) {
                row = r;
                col = c;
                return true;
            }
        }
    }
    return false;
}

QString GameState::toFen() const
{
    QString fen;
    for (int r = 0; r < kBoardSize; ++r) {
        int empty = 0;
        for (int c = 0; c < kBoardSize; ++c) {
            const ChessPiece &p = m_squares[r * kBoardSize + c];
            if (!p.isValid()) {
                ++empty;
                continue;
            }
            if (empty > 0) {
                fen += QString::number(empty);
                empty = 0;
            }
            QChar ch;
            switch (p.type()) {
            case PieceType::Pawn:   ch = 'p'; break;
            case PieceType::Knight: ch = 'n'; break;
            case PieceType::Bishop: ch = 'b'; break;
            case PieceType::Rook:   ch = 'r'; break;
            case PieceType::Queen:  ch = 'q'; break;
            case PieceType::King:   ch = 'k'; break;
            default: break;
            }
            if (p.color() == PieceColor::White)
                ch = ch.toUpper();
            fen += ch;
        }
        if (empty > 0)
            fen += QString::number(empty);
        if (r < kBoardSize - 1)
            fen += '/';
    }

    fen += m_turn == PieceColor::White ? " w " : " b ";

    QString castling;
    if (m_castleWK) castling += 'K';
    if (m_castleWQ) castling += 'Q';
    if (m_castleBK) castling += 'k';
    if (m_castleBQ) castling += 'q';
    fen += castling.isEmpty() ? "-" : castling;

    fen += ' ';
    if (m_enPassantRow >= 0 && m_enPassantCol >= 0) {
        fen += QChar('a' + m_enPassantCol);
        fen += QString::number(8 - m_enPassantRow);
    } else {
        fen += '-';
    }

    fen += ' ';
    fen += QString::number(m_halfmoveClock);
    fen += ' ';
    fen += QString::number(m_fullmoveNumber);

    return fen;
}

bool GameState::fromFen(const QString &fen)
{
    const QStringList parts = fen.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 4)
        return false;

    // 解析棋子布局
    const QStringList ranks = parts[0].split('/');
    if (ranks.size() != kBoardSize)
        return false;

    QVector<ChessPiece> squares(kBoardSize * kBoardSize);
    for (int r = 0; r < kBoardSize; ++r) {
        const QString &rank = ranks[r];
        int col = 0;
        for (const QChar &ch : rank) {
            if (ch.isDigit()) {
                col += ch.digitValue();
            } else {
                if (col >= kBoardSize)
                    return false;
                PieceType type;
                switch (ch.toLower().toLatin1()) {
                case 'p': type = PieceType::Pawn; break;
                case 'n': type = PieceType::Knight; break;
                case 'b': type = PieceType::Bishop; break;
                case 'r': type = PieceType::Rook; break;
                case 'q': type = PieceType::Queen; break;
                case 'k': type = PieceType::King; break;
                default: return false;
                }
                const PieceColor color = ch.isUpper() ? PieceColor::White : PieceColor::Black;
                squares[r * kBoardSize + col] = ChessPiece(type, color);
                ++col;
            }
        }
        if (col != kBoardSize)
            return false;
    }

    // 回合
    PieceColor turn = PieceColor::White;
    if (parts[1] == QStringLiteral("b"))
        turn = PieceColor::Black;
    else if (parts[1] != QStringLiteral("w"))
        return false;

    // 易位权限
    bool castleWK = false, castleWQ = false, castleBK = false, castleBQ = false;
    const QString castling = parts[2];
    if (castling != QStringLiteral("-")) {
        for (const QChar &ch : castling) {
            switch (ch.toLatin1()) {
            case 'K': castleWK = true; break;
            case 'Q': castleWQ = true; break;
            case 'k': castleBK = true; break;
            case 'q': castleBQ = true; break;
            default: return false;
            }
        }
    }

    // 吃过路兵目标格
    int epRow = -1, epCol = -1;
    const QString ep = parts[3];
    if (ep != QStringLiteral("-")) {
        if (ep.size() != 2)
            return false;
        const int c = ep[0].toLatin1() - 'a';
        const int r = 8 - ep[1].digitValue();
        if (c < 0 || c >= kBoardSize || r < 0 || r >= kBoardSize)
            return false;
        epCol = c;
        epRow = r;
    }

    // 半回合与完整回合数（可选）
    int halfmove = 0;
    int fullmove = 1;
    if (parts.size() >= 5)
        halfmove = parts[4].toInt();
    if (parts.size() >= 6)
        fullmove = parts[5].toInt();

    // 全部解析成功，提交
    m_squares = squares;
    m_turn = turn;
    m_castleWK = castleWK;
    m_castleWQ = castleWQ;
    m_castleBK = castleBK;
    m_castleBQ = castleBQ;
    m_enPassantRow = epRow;
    m_enPassantCol = epCol;
    m_halfmoveClock = halfmove;
    m_fullmoveNumber = fullmove;
    return true;
}

GameState GameState::copy() const
{
    GameState s;
    s.m_squares = m_squares;
    s.m_turn = m_turn;
    s.m_castleWK = m_castleWK;
    s.m_castleWQ = m_castleWQ;
    s.m_castleBK = m_castleBK;
    s.m_castleBQ = m_castleBQ;
    s.m_enPassantRow = m_enPassantRow;
    s.m_enPassantCol = m_enPassantCol;
    s.m_halfmoveClock = m_halfmoveClock;
    s.m_fullmoveNumber = m_fullmoveNumber;
    return s;
}
