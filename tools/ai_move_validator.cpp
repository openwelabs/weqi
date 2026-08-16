// AI 走法验证器（测试辅助工具）。
// 从 stdin 读取一行 JSON：{"fen": "...", "move": "e2e4"}
// 用 ChessRules 判断该走法在当前局面下是否合法。
// 输出一行 JSON：{"legal": true} 或 {"legal": false, "reason": "..."}
//
// 用途：供 Python 自动化测试脚本调用，验证 AI 返回的走法是否合法。
// 不参与游戏逻辑，仅用于测试。

#include "GameState.h"
#include "ChessRules.h"
#include "Move.h"

#include <QCoreApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QTextStream>

namespace {

// 把 UCI 走法（如 "e2e4"）转换为 Move 结构
Move uciToMove(const QString &uci)
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

// 把 Move 结构转换为 UCI 走法字符串（如 "e2e4"、"e7e8q"）
QString moveToUci(const Move &move)
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

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QTextStream in(stdin);
    QTextStream out(stdout);

    const QString line = in.readLine();
    if (line.isEmpty()) {
        out << "{\"legal\": false, \"reason\": \"empty_input\"}\n";
        out.flush();
        return 1;
    }

    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        out << "{\"legal\": false, \"reason\": \"bad_json\"}\n";
        out.flush();
        return 1;
    }

    const QJsonObject obj = doc.object();
    const QString fen = obj.value(QStringLiteral("fen")).toString();
    const QString moveStr = obj.value(QStringLiteral("move")).toString();

    GameState state;
    if (!state.fromFen(fen)) {
        out << "{\"legal\": false, \"reason\": \"bad_fen\"}\n";
        out.flush();
        return 1;
    }

    // list_legal 模式：只输出当前方所有合法走法（UCI 格式），不验证具体走法。
    // 供自动化测试生成合法走法列表，传给 AI 让其从中选择。
    if (obj.value(QStringLiteral("list_legal")).toBool()) {
        const QVector<Move> allLegal = ChessRules::getAllLegalMoves(state, state.turn());
        QJsonArray arr;
        for (const Move &m : allLegal)
            arr.append(moveToUci(m));
        QJsonObject result;
        result[QStringLiteral("legal_moves")] = arr;
        out << QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)) << "\n";
        out.flush();
        return 0;
    }

    const Move move = uciToMove(moveStr);
    if (!move.isValid()) {
        out << "{\"legal\": false, \"reason\": \"bad_move_format\"}\n";
        out.flush();
        return 1;
    }

    // 检查来源格是否有当前方棋子
    const ChessPiece piece = state.pieceAt(move.fromRow, move.fromCol);
    if (piece.type() == PieceType::None || piece.color() != state.turn()) {
        out << "{\"legal\": false, \"reason\": \"no_piece_of_turn\"}\n";
        out.flush();
        return 1;
    }

    // 用规则引擎验证
    const QVector<Move> legalMoves = ChessRules::getLegalMoves(state, move.fromRow, move.fromCol);
    bool isLegal = false;
    for (const Move &m : legalMoves) {
        if (m.toRow == move.toRow && m.toCol == move.toCol) {
            if (move.isPromotion && m.promotion != move.promotion)
                continue;
            isLegal = true;
            break;
        }
    }

    if (isLegal) {
        // 应用走法，输出新 FEN（供自动化测试推进对局）
        const GameState newState = ChessRules::makeMove(state, move);
        QJsonObject result;
        result[QStringLiteral("legal")] = true;
        result[QStringLiteral("new_fen")] = newState.toFen();
        out << QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)) << "\n";
    } else {
        out << "{\"legal\": false, \"reason\": \"illegal_move\"}\n";
    }
    out.flush();
    return 0;
}
