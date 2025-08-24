#ifndef GAMELOGIC_H
#define GAMELOGIC_H

#include <string>
#include <vector>
#include <array>
#include <set>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <random>

using namespace std;

// =================== 상수, 열거형, 전역 변수 ===================

constexpr int FULL_HOUSE_SCORE = 25;
constexpr int SMALL_STRAIGHT_SCORE = 30;
constexpr int LARGE_STRAIGHT_SCORE = 40;
constexpr int YAHTZEE_SCORE = 50;
constexpr int YAHTZEE_BONUS_SCORE = 100;
constexpr int UPPER_BONUS_THRESHOLD = 63;
constexpr int UPPER_BONUS_SCORE = 35;

enum class Category {
    ONES, TWOS, THREES, FOURS, FIVES, SIXES,
    THREE_KIND, FOUR_KIND, FULL_HOUSE,
    SMALL_STRAIGHT, LARGE_STRAIGHT, YAHTZEE, CHANCE,
    CATEGORY_COUNT
};

extern const array<string, static_cast<size_t>(Category::CATEGORY_COUNT)> CAT_NAME;

using Dice = array<int, 5>;

extern mt19937 g_rng;
extern uniform_int_distribution<int> g_diceDist;

// =================== 구조체 정의 ===================

struct Scorecard {
    string name;
    array<int, static_cast<size_t>(Category::CATEGORY_COUNT)> scores{};
    array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)> used{};
    int yahtzeeBonusCount = 0;

    Scorecard(string n = u8"플레이어");
    int upperSum() const;
    int lowerSum() const;
    int upperBonus() const;
    int total() const;
};

// =================== 함수 선언 ===================

// 게임 로직 및 헬퍼 함수
int sumDice(const Dice& d);
array<int, 7> countFace(const Dice& d);
bool isYahtzee(const Dice& d);
int scoreUpper(const Dice& d, int face);
int scoreThreeKind(const Dice& d);
int scoreFourKind(const Dice& d);
int scoreFullHouse(const Dice& d);
int scoreSmallStraight(const Dice& d);
int scoreLargeStraight(const Dice& d);
int scoreYahtzee(const Dice& d);
int scoreChance(const Dice& d);
int scoreCategory(Category cat, const Dice& d);
inline int roll6();
vector<int> parseIndices(string line);
vector<int> generateDiceSequence(int totalFrames = 20, int maxRepeat = 4);
string checkForSpecialCombinations(const Dice& d);

#endif // GAMELOGIC_H
