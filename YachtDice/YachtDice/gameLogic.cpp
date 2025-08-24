#include "gameLogic.h"

// =================== 전역 변수 정의 ===================
const array<string, static_cast<size_t>(Category::CATEGORY_COUNT)> CAT_NAME = {
    u8"에이스(1)", u8"투즈(2)", u8"쓰리즈(3)", u8"포즈(4)", u8"파이브즈(5)", u8"식스즈(6)",
    u8"쓰리카인드", u8"포카인드", u8"풀하우스",
    u8"스몰 스트레이트", u8"라지 스트레이트", u8"야추", u8"찬스"
};

mt19937 g_rng(random_device{}());
uniform_int_distribution<int> g_diceDist(1, 6);

// =================== 구조체 메서드 구현 ===================
Scorecard::Scorecard(string n) : name(move(n)) {
    scores.fill(0); used.fill(false);
}
int Scorecard::upperSum() const {
    int s = 0;
    for (int i = 0; i <= static_cast<int>(Category::SIXES); i++) if (used[i]) s += scores[i];
    return s;
}
int Scorecard::lowerSum() const {
    int s = 0;
    for (int i = static_cast<int>(Category::THREE_KIND); i <= static_cast<int>(Category::CHANCE); i++) if (used[i]) s += scores[i];
    return s;
}
int Scorecard::upperBonus() const {
    return (upperSum() >= UPPER_BONUS_THRESHOLD) ? UPPER_BONUS_SCORE : 0;
}
int Scorecard::total() const { return upperSum() + upperBonus() + lowerSum() + (yahtzeeBonusCount * YAHTZEE_BONUS_SCORE); }

// =================== 게임 로직 및 헬퍼 함수 ===================
int sumDice(const Dice& d) { return accumulate(d.begin(), d.end(), 0); }
array<int, 7> countFace(const Dice& d) {
    array<int, 7> c{};
    c.fill(0);
    for (int v : d) c[v]++;
    return c;
}
bool isYahtzee(const Dice& d) {
    auto c = countFace(d);
    for (int i = 1; i <= 6; i++) if (c[i] == 5) return true;
    return false;
}
int scoreUpper(const Dice& d, int face) { auto c = countFace(d); return c[face] * face; }
int scoreThreeKind(const Dice& d) { auto c = countFace(d); for (int i = 1; i <= 6; i++) if (c[i] >= 3) return sumDice(d); return 0; }
int scoreFourKind(const Dice& d) { auto c = countFace(d); for (int i = 1; i <= 6; i++) if (c[i] >= 4) return sumDice(d); return 0; }
int scoreFullHouse(const Dice& d) {
    auto c = countFace(d);
    bool h3 = false, h2 = false;
    for (int i = 1; i <= 6; i++) { if (c[i] == 3) h3 = true; if (c[i] == 2) h2 = true; }
    return (h3 && h2) ? FULL_HOUSE_SCORE : 0;
}
int longestRun(const set<int>& s) {
    if (s.empty()) return 0;
    int best = 1, cur = 1, prev = *s.begin();
    for (auto it = next(s.begin()); it != s.end(); ++it) {
        if (*it == prev + 1) cur++; else cur = 1;
        best = (std::max)(best, cur);
        prev = *it;
    }
    return best;
}
int scoreSmallStraight(const Dice& d) { set<int> s(d.begin(), d.end()); return (longestRun(s) >= 4) ? SMALL_STRAIGHT_SCORE : 0; }
int scoreLargeStraight(const Dice& d) { set<int> s(d.begin(), d.end()); return (longestRun(s) >= 5) ? LARGE_STRAIGHT_SCORE : 0; }
int scoreYahtzee(const Dice& d) { return isYahtzee(d) ? YAHTZEE_SCORE : 0; }
int scoreChance(const Dice& d) { return sumDice(d); }
int scoreCategory(Category cat, const Dice& d) {
    switch (cat) {
    case Category::ONES: return scoreUpper(d, 1);
    case Category::TWOS: return scoreUpper(d, 2);
    case Category::THREES: return scoreUpper(d, 3);
    case Category::FOURS: return scoreUpper(d, 4);
    case Category::FIVES: return scoreUpper(d, 5);
    case Category::SIXES: return scoreUpper(d, 6);
    case Category::THREE_KIND: return scoreThreeKind(d);
    case Category::FOUR_KIND: return scoreFourKind(d);
    case Category::FULL_HOUSE: return scoreFullHouse(d);
    case Category::SMALL_STRAIGHT: return scoreSmallStraight(d);
    case Category::LARGE_STRAIGHT: return scoreLargeStraight(d);
    case Category::YAHTZEE: return scoreYahtzee(d);
    case Category::CHANCE: return scoreChance(d);
    default: return 0;
    }
}
inline int roll6() { return g_diceDist(g_rng); }
vector<int> parseIndices(string line) {
    for (char& ch : line) if (ch == ',') ch = ' ';
    vector<int> idx; istringstream iss(line); int x;
    while (iss >> x) if (1 <= x && x <= 5) idx.push_back(x - 1);
    sort(idx.begin(), idx.end()); idx.erase(unique(idx.begin(), idx.end()), idx.end());
    return idx;
}
vector<int> generateDiceSequence(int totalFrames, int maxRepeat) {
    vector<int> sequence;
    int framesLeft = totalFrames;
    while (framesLeft > 0) {
        int repeat = (g_rng() % maxRepeat) + 1;
        if (repeat > framesLeft) repeat = framesLeft;
        int face = roll6();
        for (int i = 0; i < repeat; ++i) sequence.push_back(face);
        framesLeft -= repeat;
    }
    return sequence;
}
string checkForSpecialCombinations(const Dice& d) {
    if (scoreYahtzee(d) > 0) return u8"야추!";
    if (scoreLargeStraight(d) > 0) return u8"라지 스트레이트!";
    if (scoreSmallStraight(d) > 0) return u8"스몰 스트레이트!";
    if (scoreFullHouse(d) > 0) return u8"풀하우스!";
    if (scoreFourKind(d) > 0) return u8"포카인드!";
    if (scoreThreeKind(d) > 0) return u8"쓰리카인드!";
    return "";
}
