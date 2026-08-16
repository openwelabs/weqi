#include "GameRecord.h"

#include <QUuid>

QJsonObject GameRecord::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["date"] = date.toString(Qt::ISODate);
    obj["mode"] = static_cast<int>(mode);
    obj["opponent"] = opponent;
    obj["whiteName"] = whiteName;
    obj["blackName"] = blackName;
    obj["outcome"] = static_cast<int>(outcome);
    obj["resultText"] = resultText;
    obj["reason"] = reason;
    obj["ratingChange"] = ratingChange;
    obj["moveCount"] = moveCount;
    obj["pgn"] = pgn;
    return obj;
}

GameRecord GameRecord::fromJson(const QJsonObject &obj)
{
    GameRecord rec;
    rec.id = obj["id"].toString();
    if (rec.id.isEmpty())
        rec.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    rec.date = QDateTime::fromString(obj["date"].toString(), Qt::ISODate);
    if (!rec.date.isValid())
        rec.date = QDateTime::currentDateTime();
    rec.mode = static_cast<GameMode>(obj["mode"].toInt(static_cast<int>(GameMode::HumanVsHuman)));
    rec.opponent = obj["opponent"].toString();
    rec.whiteName = obj["whiteName"].toString();
    rec.blackName = obj["blackName"].toString();
    rec.outcome = static_cast<GameOutcome>(obj["outcome"].toInt(static_cast<int>(GameOutcome::Ongoing)));
    rec.resultText = obj["resultText"].toString();
    rec.reason = obj["reason"].toString();
    rec.ratingChange = obj["ratingChange"].toInt(0);
    rec.moveCount = obj["moveCount"].toInt(0);
    rec.pgn = obj["pgn"].toString();
    return rec;
}
