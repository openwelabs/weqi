// 核心走法逻辑测试：验证 C++ 本地合法走法计算、走子执行、回合切换与特殊规则。
// 通过 GameController（公开接口）与 ChessRules（规则引擎）进行测试。

#include "GameController.h"
#include "ChessRules.h"
#include "GameState.h"

#include <QtTest/QtTest>

class TestMovement : public QObject
{
    Q_OBJECT

private slots:
    // 基础走法
    void whitePawnMovesTwoFromStart();
    void turnAlternatesAfterMove();
    void blackCanMoveAfterWhite();
    void illegalMoveRejected();
    void knightMoves();
    void pawnCapture();

    // 特殊规则
    void castlingKingside();
    void castlingQueenside();
    void castlingBlockedByCheck();
    void enPassant();
    void promotionFlow();

    // 状态
    void undoRestoresState();
    void newGameResets();

    // 安全
    void checkDetection();
    void checkmateDetection();
    void pinnedPieceCannotMove();
    void kingCannotMoveIntoCheck();
    void kingCannotBeCaptured();

    // 功能
    void fullGameFlow();
    void resolveSanMove();
};

// 白兵从起始位置（row 6）可前进两格到 row 4
void TestMovement::whitePawnMovesTwoFromStart()
{
    GameController c;
    const QVector<Move> moves = c.legalMoves(6, 4); // 白兵 e2
    bool hasTwoStep = false;
    for (const Move &m : moves) {
        if (m.toRow == 4 && m.toCol == 4)
            hasTwoStep = true;
    }
    QVERIFY(hasTwoStep);
}

// 白方走子后回合切换为黑方
void TestMovement::turnAlternatesAfterMove()
{
    GameController c;
    QCOMPARE(c.currentTurn(), PieceColor::White);

    // 白兵 e2 -> e4
    QVERIFY(c.tryMove(6, 4, 4, 4));

    QCOMPARE(c.currentTurn(), PieceColor::Black);
    // 棋子已移动
    QVERIFY(c.state().pieceAt(4, 4).isValid());
    QVERIFY(!c.state().pieceAt(6, 4).isValid());
}

// 黑方在白方之后可以移动
void TestMovement::blackCanMoveAfterWhite()
{
    GameController c;
    QVERIFY(c.tryMove(6, 4, 4, 4)); // 白 e4
    QCOMPARE(c.currentTurn(), PieceColor::Black);

    // 黑兵 e7 -> e5
    QVERIFY(c.tryMove(1, 4, 3, 4));

    QCOMPARE(c.currentTurn(), PieceColor::White);
    QVERIFY(c.state().pieceAt(3, 4).isValid());
    QVERIFY(!c.state().pieceAt(1, 4).isValid());
}

// 非法走法被拒绝：白兵不能斜向走到空格
void TestMovement::illegalMoveRejected()
{
    GameController c;
    // 白兵 e2 尝试斜向走到 d3（row 5, col 3）——非法
    QVERIFY(!c.tryMove(6, 4, 5, 3));

    // 回合不应切换，棋子不应移动
    QCOMPARE(c.currentTurn(), PieceColor::White);
    QVERIFY(c.state().pieceAt(6, 4).isValid());
    QVERIFY(!c.state().pieceAt(5, 3).isValid());
}

// 马走 L 形
void TestMovement::knightMoves()
{
    GameController c;
    // 白马 b1（row 7, col 1）
    const QVector<Move> moves = c.legalMoves(7, 1);
    bool hasTarget = false;
    for (const Move &m : moves) {
        // 可走到 a3(row5,col0) 或 c3(row5,col2)
        if ((m.toRow == 5 && m.toCol == 0) || (m.toRow == 5 && m.toCol == 2))
            hasTarget = true;
    }
    QVERIFY(hasTarget);
}

// 兵斜向吃子
void TestMovement::pawnCapture()
{
    GameController c;
    QVERIFY(c.tryMove(6, 4, 4, 4)); // 白 e4
    QVERIFY(c.tryMove(1, 3, 3, 3)); // 黑 d5

    // 白兵 e4 现在可斜吃 d5（row 3, col 3）
    const QVector<Move> moves = c.legalMoves(4, 4);
    bool canCapture = false;
    for (const Move &m : moves) {
        if (m.toRow == 3 && m.toCol == 3)
            canCapture = true;
    }
    QVERIFY(canCapture);

    // 执行吃子
    QVERIFY(c.tryMove(4, 4, 3, 3));
    QVERIFY(c.state().pieceAt(3, 3).isValid());
    QVERIFY(!c.state().pieceAt(4, 4).isValid());
}

// 王车易位（短易位 O-O）
void TestMovement::castlingKingside()
{
    GameState s;
    s.reset();
    // 清空王与车之间的棋子
    // 白方：e1(7,4) 王，h1(7,7) 车。清空 f1(7,5)、g1(7,6)
    // 黑方：e8(0,4) 王，h8(0,7) 车。清空 f8(0,5)、g8(0,6)
    s.setPiece(7, 5, ChessPiece());
    s.setPiece(7, 6, ChessPiece());
    s.setPiece(0, 5, ChessPiece());
    s.setPiece(0, 6, ChessPiece());

    // 白方短易位：王 e1 -> g1
    const QVector<Move> moves = ChessRules::getLegalMoves(s, 7, 4);
    Move castle;
    bool found = false;
    for (const Move &m : moves) {
        if (m.isCastleKingside) {
            castle = m;
            found = true;
            break;
        }
    }
    QVERIFY(found);

    // 执行易位
    GameState after = ChessRules::makeMove(s, castle);
    // 王在 g1(7,6)，车在 f1(7,5)
    QVERIFY(after.pieceAt(7, 6).type() == PieceType::King);
    QVERIFY(after.pieceAt(7, 5).type() == PieceType::Rook);
    QVERIFY(!after.pieceAt(7, 4).isValid());
    QVERIFY(!after.pieceAt(7, 7).isValid());
}

// 王车易位（长易位 O-O-O）
void TestMovement::castlingQueenside()
{
    GameState s;
    s.reset();
    // 清空 d1(7,3)、c1(7,2)、b1(7,1)
    s.setPiece(7, 3, ChessPiece());
    s.setPiece(7, 2, ChessPiece());
    s.setPiece(7, 1, ChessPiece());
    // 清空黑方 d8(0,3)、c8(0,2)、b8(0,1)
    s.setPiece(0, 3, ChessPiece());
    s.setPiece(0, 2, ChessPiece());
    s.setPiece(0, 1, ChessPiece());

    const QVector<Move> moves = ChessRules::getLegalMoves(s, 7, 4);
    Move castle;
    bool found = false;
    for (const Move &m : moves) {
        if (m.isCastleQueenside) {
            castle = m;
            found = true;
            break;
        }
    }
    QVERIFY(found);

    GameState after = ChessRules::makeMove(s, castle);
    // 王在 c1(7,2)，车在 d1(7,3)
    QVERIFY(after.pieceAt(7, 2).type() == PieceType::King);
    QVERIFY(after.pieceAt(7, 3).type() == PieceType::Rook);
    QVERIFY(!after.pieceAt(7, 4).isValid());
    QVERIFY(!after.pieceAt(7, 0).isValid());
}

// 王被将军时不能易位
void TestMovement::castlingBlockedByCheck()
{
    GameState s;
    s.reset();
    // 清空白方 f1(7,5)、g1(7,6)
    s.setPiece(7, 5, ChessPiece());
    s.setPiece(7, 6, ChessPiece());
    // 清空 e 列上的兵，使黑车能攻击白王
    s.setPiece(6, 4, ChessPiece()); // 白兵 e2
    s.setPiece(1, 4, ChessPiece()); // 黑兵 e7
    // 放置黑车攻击 e1(7,4) 所在列，使白王被将军
    s.setPiece(0, 4, ChessPiece(PieceType::Rook, PieceColor::Black));

    // 白王被将军，不能易位
    QVERIFY(ChessRules::isInCheck(s, PieceColor::White));
    const QVector<Move> moves = ChessRules::getLegalMoves(s, 7, 4);
    for (const Move &m : moves) {
        QVERIFY(!m.isCastleKingside);
        QVERIFY(!m.isCastleQueenside);
    }
}

// 吃过路兵
void TestMovement::enPassant()
{
    GameController c;
    // 白 e2 -> e4
    QVERIFY(c.tryMove(6, 4, 4, 4));
    // 黑 d7 -> d5（双步，触发吃过路兵，目标格 d6）
    QVERIFY(c.tryMove(1, 3, 3, 3));

    // 白兵 e4 现在可吃过路兵吃 d6（row 2, col 3）
    const QVector<Move> moves = c.legalMoves(4, 4);
    bool canEnPassant = false;
    for (const Move &m : moves) {
        if (m.isEnPassant && m.toRow == 2 && m.toCol == 3)
            canEnPassant = true;
    }
    QVERIFY(canEnPassant);

    // 执行吃过路兵
    QVERIFY(c.tryMove(4, 4, 2, 3));
    // 白兵在 d6(2,3)，黑兵 d5(3,3) 被移除
    QVERIFY(c.state().pieceAt(2, 3).isValid());
    QVERIFY(!c.state().pieceAt(3, 3).isValid());
}

// 升变流程：白兵到第 7 行（row 1），准备升变
void TestMovement::promotionFlow()
{
    GameState s;
    s.reset();
    // 清空白兵 e2(6,4)，放到 e7(1,4)
    s.setPiece(6, 4, ChessPiece());
    s.setPiece(1, 4, ChessPiece(PieceType::Pawn, PieceColor::White));
    // 清空黑方 e8(0,4) 王，放到 a8(0,0) 避免干扰
    s.setPiece(0, 4, ChessPiece());
    s.setPiece(0, 0, ChessPiece(PieceType::King, PieceColor::Black));

    const QVector<Move> moves = ChessRules::getLegalMoves(s, 1, 4);
    bool hasPromotion = false;
    for (const Move &m : moves) {
        if (m.isPromotion && m.toRow == 0)
            hasPromotion = true;
    }
    QVERIFY(hasPromotion);

    // 执行升变为后
    GameState after = ChessRules::makeMove(s, moves.first());
    QVERIFY(after.pieceAt(0, 4).type() == PieceType::Queen);
    QVERIFY(!after.pieceAt(1, 4).isValid());
}

// 悔棋恢复完整状态
void TestMovement::undoRestoresState()
{
    GameController c;
    QVERIFY(c.tryMove(6, 4, 4, 4)); // 白 e4
    QVERIFY(c.tryMove(1, 4, 3, 4)); // 黑 e5

    QCOMPARE(c.currentTurn(), PieceColor::White);
    QVERIFY(c.state().pieceAt(4, 4).isValid());

    // 悔棋一步（撤销黑方 e5），应恢复到黑方回合
    QVERIFY(c.undo());
    QCOMPARE(c.currentTurn(), PieceColor::Black);
    QVERIFY(!c.state().pieceAt(3, 4).isValid());
    QVERIFY(c.state().pieceAt(1, 4).isValid());

    // 再悔棋一步（撤销白方 e4），应恢复到白方回合
    QVERIFY(c.undo());
    QCOMPARE(c.currentTurn(), PieceColor::White);
    QVERIFY(!c.state().pieceAt(4, 4).isValid());
    QVERIFY(c.state().pieceAt(6, 4).isValid());

    // 无历史时悔棋失败
    QVERIFY(!c.undo());
}

// 新游戏重置状态
void TestMovement::newGameResets()
{
    GameController c;
    QVERIFY(c.tryMove(6, 4, 4, 4)); // 白 e4
    QVERIFY(c.tryMove(1, 4, 3, 4)); // 黑 e5

    c.newGame();
    QCOMPARE(c.currentTurn(), PieceColor::White);
    QCOMPARE(c.result(), GameController::Result::Ongoing);
    QVERIFY(c.state().pieceAt(6, 4).isValid());
    QVERIFY(c.state().pieceAt(1, 4).isValid());
    QCOMPARE(c.history().count(), 0);
}

// 将军检测
void TestMovement::checkDetection()
{
    GameState s;
    s.reset();
    // 清空 e 列上的兵，使黑车能攻击白王
    s.setPiece(6, 4, ChessPiece()); // 白兵 e2
    s.setPiece(1, 4, ChessPiece()); // 黑兵 e7
    // 白王 e1(7,4)，黑车 e8(0,4) 攻击整列 e
    s.setPiece(0, 4, ChessPiece(PieceType::Rook, PieceColor::Black));
    QVERIFY(ChessRules::isInCheck(s, PieceColor::White));
    QVERIFY(!ChessRules::isInCheck(s, PieceColor::Black));
}

// 将死检测：黑王 h8 被白后 g7 与白王 f6 夹击
void TestMovement::checkmateDetection()
{
    GameState s;
    s.reset();
    // 清空棋盘
    for (int r = 0; r < GameState::kBoardSize; ++r)
        for (int c = 0; c < GameState::kBoardSize; ++c)
            s.setPiece(r, c, ChessPiece());
    // 黑王 h8(0,7)，白王 f6(2,5)，白后 g7(1,6)
    s.setPiece(0, 7, ChessPiece(PieceType::King, PieceColor::Black));
    s.setPiece(2, 5, ChessPiece(PieceType::King, PieceColor::White));
    s.setPiece(1, 6, ChessPiece(PieceType::Queen, PieceColor::White));

    QVERIFY(ChessRules::isInCheck(s, PieceColor::Black));
    QVERIFY(ChessRules::isCheckmate(s, PieceColor::Black));
}

// 被牵制的棋子不能移动（会暴露王）
void TestMovement::pinnedPieceCannotMove()
{
    GameState s;
    s.reset();
    // 清空 e 列上的兵，使黑车能攻击白王
    s.setPiece(6, 4, ChessPiece()); // 白兵 e2（将被替换为象）
    s.setPiece(1, 4, ChessPiece()); // 黑兵 e7
    // 白王 e1(7,4)，白象 e2(6,4)，黑车 e8(0,4)
    s.setPiece(6, 4, ChessPiece(PieceType::Bishop, PieceColor::White));
    s.setPiece(0, 4, ChessPiece(PieceType::Rook, PieceColor::Black));

    const QVector<Move> moves = ChessRules::getLegalMoves(s, 6, 4);
    // 白象被牵制，不能移动（任何移动都会暴露王）
    QVERIFY(moves.isEmpty());
}

// 王不能走进被攻击的格子
void TestMovement::kingCannotMoveIntoCheck()
{
    GameState s;
    s.reset();
    // 清空 e 列上的兵，使黑车能攻击白王
    s.setPiece(6, 4, ChessPiece()); // 白兵 e2
    s.setPiece(1, 4, ChessPiece()); // 黑兵 e7
    // 白王 e1(7,4)，黑车 e8(0,4) 攻击 e 列
    s.setPiece(0, 4, ChessPiece(PieceType::Rook, PieceColor::Black));
    // 清空白方其他棋子，让王有移动空间
    for (int r = 5; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            s.setPiece(r, c, ChessPiece());
    s.setPiece(7, 4, ChessPiece(PieceType::King, PieceColor::White));

    const QVector<Move> moves = ChessRules::getLegalMoves(s, 7, 4);
    // 王不能走到 e 列（被黑车攻击）
    for (const Move &m : moves) {
        QVERIFY(m.toCol != 4);
    }
}

// 王不能被吃：任何棋子都不能吃掉对方的王（只能通过将死获胜）
void TestMovement::kingCannotBeCaptured()
{
    GameState s;
    s.reset();
    // 清空棋盘
    for (int r = 0; r < GameState::kBoardSize; ++r)
        for (int c = 0; c < GameState::kBoardSize; ++c)
            s.setPiece(r, c, ChessPiece());
    // 白王 e1(7,4)，黑王 e8(0,4)，白后 d1(7,3) 可沿 d 列攻击黑王
    s.setPiece(7, 4, ChessPiece(PieceType::King, PieceColor::White));
    s.setPiece(0, 4, ChessPiece(PieceType::King, PieceColor::Black));
    s.setPiece(7, 3, ChessPiece(PieceType::Queen, PieceColor::White));

    // 白后 d1 不能直接吃掉黑王 e8（目标格是对方王，非法）
    const QVector<Move> queenMoves = ChessRules::getLegalMoves(s, 7, 3);
    for (const Move &m : queenMoves) {
        QVERIFY(!(m.toRow == 0 && m.toCol == 4));
    }

    // 黑王仍在棋盘上
    int kr, kc;
    QVERIFY(s.findKing(PieceColor::Black, kr, kc));
    QCOMPARE(kr, 0);
    QCOMPARE(kc, 4);
}

// 完整棋局流程：多步走子、悔棋、新游戏
void TestMovement::fullGameFlow()
{
    GameController c;

    // 意大利开局前几步
    QVERIFY(c.tryMove(6, 4, 4, 4)); // 1. e4
    QVERIFY(c.tryMove(1, 4, 3, 4)); // 1... e5
    QVERIFY(c.tryMove(7, 6, 5, 5)); // 2. Nf3
    QVERIFY(c.tryMove(0, 6, 2, 5)); // 2... Nc6
    QVERIFY(c.tryMove(7, 5, 4, 2)); // 3. Bc4
    QVERIFY(c.tryMove(0, 5, 3, 2)); // 3... Bc5

    QCOMPARE(c.history().count(), 6);
    QCOMPARE(c.currentTurn(), PieceColor::White);
    QCOMPARE(c.result(), GameController::Result::Ongoing);

    // 棋谱应包含 SAN
    const QString notation = c.history().toNotation();
    QVERIFY(notation.contains(QStringLiteral("e4")));
    QVERIFY(notation.contains(QStringLiteral("Nf3")));

    // 悔棋两步
    QVERIFY(c.undo());
    QVERIFY(c.undo());
    QCOMPARE(c.history().count(), 4);
    QCOMPARE(c.currentTurn(), PieceColor::White);

    // 新游戏
    c.newGame();
    QCOMPARE(c.history().count(), 0);
    QCOMPARE(c.currentTurn(), PieceColor::White);
}

// AI 返回短代数记法（SAN）时，应能解析为合法走法
void TestMovement::resolveSanMove()
{
    GameController c;

    // 初始局面：白方 e4（兵 e2-e4）
    Move m = c.resolveMoveFromSan(QStringLiteral("e4"));
    QVERIFY(m.isValid());
    QCOMPARE(m.fromRow, 6);
    QCOMPARE(m.fromCol, 4);
    QCOMPARE(m.toRow, 4);
    QCOMPARE(m.toCol, 4);

    // 带将军符号
    m = c.resolveMoveFromSan(QStringLiteral("e4+"));
    QVERIFY(m.isValid());

    // 走一步后，黑方 e5
    QVERIFY(c.tryMove(6, 4, 4, 4)); // e4
    m = c.resolveMoveFromSan(QStringLiteral("e5"));
    QVERIFY(m.isValid());
    QCOMPARE(m.fromRow, 1);
    QCOMPARE(m.fromCol, 4);

    // 走两步后，白方 Nf3
    QVERIFY(c.tryMove(1, 4, 3, 4)); // e5
    m = c.resolveMoveFromSan(QStringLiteral("Nf3"));
    QVERIFY(m.isValid());
    QCOMPARE(m.fromRow, 7);
    QCOMPARE(m.fromCol, 6);
    QCOMPARE(m.toRow, 5);
    QCOMPARE(m.toCol, 5);

    // 此时轮到黑方。黑方 e4 非法（黑兵只能前进一格/两格到 e6/e5）
    m = c.resolveMoveFromSan(QStringLiteral("e4"));
    QVERIFY(!m.isValid());
}

QTEST_MAIN(TestMovement)
#include "test_movement.moc"
