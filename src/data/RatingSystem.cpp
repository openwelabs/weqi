#include "RatingSystem.h"

#include <QtMath>

double RatingSystem::expectedScore(int playerRating, int opponentRating)
{
    return 1.0 / (1.0 + qPow(10.0, (opponentRating - playerRating) / 400.0));
}

int RatingSystem::ratingDelta(int playerRating, int opponentRating, double result, int kFactor)
{
    const double expected = expectedScore(playerRating, opponentRating);
    return qRound(kFactor * (result - expected));
}
