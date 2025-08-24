#include "gameAI.h"
#include "gameLogic.h"

Category chooseBestScoringCategory_Easy(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used) {
    int maxScore = -1;
    Category bestCat = Category::CATEGORY_COUNT;
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); ++i) {
        if (!used[i]) {
            int currentScore = scoreCategory(static_cast<Category>(i), d);
            if (currentScore > maxScore) { maxScore = currentScore; bestCat = static_cast<Category>(i); }
        }
    }
    return bestCat;
}

pair<Category, int> findBestExpectedCategory_Normal(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft) {
    int maxExpectedScore = -1;
    Category bestCategory = Category::CATEGORY_COUNT;
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); ++i) {
        if (used[i]) continue;
        long long sumScores = 0;
        const int SIMULATIONS = 2000;
        for (int k = 0; k < SIMULATIONS; ++k) {
            Dice tempDice = d;
            array<bool, 5> tempHeld{}; tempHeld.fill(false);
            switch (static_cast<Category>(i)) {
            case Category::ONES: case Category::TWOS: case Category::THREES: case Category::FOURS: case Category::FIVES: case Category::SIXES:
                for (int j = 0; j < 5; ++j) if (tempDice[j] == (i + 1)) tempHeld[j] = true;
                break;
            case Category::THREE_KIND: case Category::FOUR_KIND: case Category::YAHTZEE: {
                auto counts = countFace(tempDice);
                int targetFace = 0, maxCount = 0;
                for (int j = 1; j <= 6; ++j) if (counts[j] > maxCount) { maxCount = counts[j]; targetFace = j; }
                for (int j = 0; j < 5; ++j) if (tempDice[j] == targetFace) tempHeld[j] = true;
                break;
            }
            default: break;
            }
            for (int r = 0; r < rollsLeft; ++r)
                for (int j = 0; j < 5; ++j) if (!tempHeld[j]) tempDice[j] = roll6();
            sumScores += scoreCategory(static_cast<Category>(i), tempDice);
        }
        int expectedScore = static_cast<int>(sumScores / SIMULATIONS);
        if (expectedScore > maxExpectedScore) { maxExpectedScore = expectedScore; bestCategory = static_cast<Category>(i); }
    }
    return make_pair(bestCategory, maxExpectedScore);
}

array<bool, 5> chooseBestHoldStrategy_Hard(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft, int /*round*/) {
    if (rollsLeft == 0) return { true, true, true, true, true };
    if (!used[static_cast<int>(Category::YAHTZEE)] && scoreYahtzee(d) > 0) return { true, true, true, true, true };
    if (!used[static_cast<int>(Category::LARGE_STRAIGHT)] && scoreLargeStraight(d) > 0) return { true, true, true, true, true };
    if (!used[static_cast<int>(Category::FULL_HOUSE)] && scoreFullHouse(d) > 0) return { true, true, true, true, true };

    auto bestExpected = findBestExpectedCategory_Normal(d, used, rollsLeft);
    Category bestCat = bestExpected.first;
    if (bestCat == Category::CATEGORY_COUNT) return { false, false, false, false, false };

    array<bool, 5> bestHeld{}; bestHeld.fill(false);
    switch (bestCat) {
    case Category::ONES: case Category::TWOS: case Category::THREES: case Category::FOURS: case Category::FIVES: case Category::SIXES:
        for (int i = 0; i < 5; ++i) if (d[i] == (static_cast<int>(bestCat) + 1)) bestHeld[i] = true;
        break;
    case Category::THREE_KIND: case Category::FOUR_KIND: case Category::YAHTZEE: {
        auto counts = countFace(d);
        int targetFace = 0, maxCount = 0;
        for (int i = 1; i <= 6; ++i) if (counts[i] > maxCount) { maxCount = counts[i]; targetFace = i; }
        for (int i = 0; i < 5; ++i) if (d[i] == targetFace) bestHeld[i] = true;
        break;
    }
    case Category::FULL_HOUSE: {
        auto counts = countFace(d);
        int threeKindFace = 0, twoKindFace = 0;

        for (int face = 1; face <= 6; ++face) if (counts[face] >= 3) { threeKindFace = face; break; }
        for (int face = 1; face <= 6; ++face) {
            if (face == threeKindFace) continue;
            if (counts[face] >= 2) { twoKindFace = face; break; }
        }
        if (threeKindFace == 0) {
            int bestFace = 1, bestCnt = counts[1];
            for (int face = 2; face <= 6; ++face) if (counts[face] > bestCnt) { bestCnt = counts[face]; bestFace = face; }
            threeKindFace = bestFace;
        }
        for (int i = 0; i < 5; ++i)
            if (d[i] == threeKindFace || (twoKindFace && d[i] == twoKindFace)) bestHeld[i] = true;
        break;
    }
    case Category::SMALL_STRAIGHT: case Category::LARGE_STRAIGHT: {
        set<int> uniqueDice;
        array<bool, 5> heldForStraight{};
        for (int val : d) {
            if (uniqueDice.insert(val).second) {
                for (int i = 0; i < 5; ++i) if (d[i] == val && !heldForStraight[i]) { heldForStraight[i] = true; break; }
            }
        }
        return heldForStraight;
    }
    case Category::CHANCE:
        for (int i = 0; i < 5; ++i) if (d[i] >= 4) bestHeld[i] = true;
        break;
    default: break;
    }
    return bestHeld;
}

Category chooseBestScoringCategory(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int /*round*/, AIDifficulty difficulty) {
    if (difficulty == AIDifficulty::EASY) return chooseBestScoringCategory_Easy(d, used);
    int maxScore = -1; Category bestCat = Category::CATEGORY_COUNT;
    if (!used[static_cast<int>(Category::YAHTZEE)] && scoreYahtzee(d) > 0) return Category::YAHTZEE;
    if (!used[static_cast<int>(Category::LARGE_STRAIGHT)] && scoreLargeStraight(d) > 0) return Category::LARGE_STRAIGHT;
    if (!used[static_cast<int>(Category::SMALL_STRAIGHT)] && scoreSmallStraight(d) > 0) return Category::SMALL_STRAIGHT;
    if (!used[static_cast<int>(Category::FULL_HOUSE)] && scoreFullHouse(d) > 0) return Category::FULL_HOUSE;
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); ++i) if (!used[i]) {
        int currentScore = scoreCategory(static_cast<Category>(i), d);
        if (currentScore > maxScore) { maxScore = currentScore; bestCat = static_cast<Category>(i); }
    }
    if (maxScore <= 5) {
        if (!used[static_cast<int>(Category::ONES)]) return Category::ONES;
        if (!used[static_cast<int>(Category::CHANCE)] && sumDice(d) < 10) return Category::CHANCE;
    }
    return bestCat;
}   
