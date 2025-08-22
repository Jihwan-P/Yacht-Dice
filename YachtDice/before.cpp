#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <set>
#include <numeric>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>
#include <random>
#include <thread>
#include <chrono>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace std;

// =================== Constants & Categories ===================

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

const array<string, static_cast<size_t>(Category::CATEGORY_COUNT)> CAT_NAME = {
    "에이스(1)", "투즈(2)", "쓰리즈(3)", "포즈(4)", "파이브즈(5)", "식스즈(6)",
    "쓰리카인드", "포카인드", "풀하우스",
    "스몰 스트레이트", "라지 스트레이트", "야추", "찬스"
};

// AI Difficulty Enum
enum class AIDifficulty { EASY, NORMAL, HARD };

using Dice = array<int, 5>;

// =================== Console & Rendering Low-Level ===================
#ifdef _WIN32
HANDLE HOUT;
void hideCursor(bool hide) {
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 20; ci.bVisible = hide ? FALSE : TRUE;
    SetConsoleCursorInfo(HOUT, &ci);
}
void gotoXY(int x, int y) { COORD c; c.X = (SHORT)x; c.Y = (SHORT)y; SetConsoleCursorPosition(HOUT, c); }
void writeAt(int x, int y, const string& s) {
    gotoXY(x, y);
    DWORD w;
    WriteConsoleA(HOUT, s.c_str(), (DWORD)s.size(), &w, nullptr);
}
void ensureConsoleSize(int cols, int rows) {
    SMALL_RECT tiny = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(HOUT, TRUE, &tiny);
    COORD buf = { (SHORT)cols, (SHORT)rows };
    SetConsoleScreenBufferSize(HOUT, buf);
    SMALL_RECT win = { 0, 0, (SHORT)(cols - 1), (SHORT)(rows - 1) };
    SetConsoleWindowInfo(HOUT, TRUE, &win);
}
#else
void hideCursor(bool hide) { if (hide) cout << "\x1b[?25l"; else cout << "\x1b[?25h"; }
void gotoXY(int x, int y) { cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H"; }
void writeAt(int x, int y, const string& s) { gotoXY(x, y); cout << s; cout.flush(); }
void ensureConsoleSize(int, int) {}
#endif

void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// =================== Game Logic Helpers ===================
int sumDice(const Dice& d) { return accumulate(d.begin(), d.end(), 0); }
array<int, 7> countFace(const Dice& d) {
    array<int, 7> c{}; c.fill(0);
    for (int v : d) c[v]++;
    return c;
}

bool isYahtzee(const Dice& d) {
    auto c = countFace(d);
    for (int i = 1; i <= 6; i++) {
        if (c[i] == 5) return true;
    }
    return false;
}

int scoreUpper(const Dice& d, int face) { auto c = countFace(d); return c[face] * face; }
int scoreThreeKind(const Dice& d) { auto c = countFace(d); for (int i = 1; i <= 6; i++) if (c[i] >= 3) return sumDice(d); return 0; }
int scoreFourKind(const Dice& d) { auto c = countFace(d); for (int i = 1; i <= 6; i++) if (c[i] >= 4) return sumDice(d); return 0; }
int scoreFullHouse(const Dice& d) {
    auto c = countFace(d); bool h3 = false, h2 = false;
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

// =================== Scorecard Struct ===================
struct Scorecard {
    string name;
    array<int, static_cast<size_t>(Category::CATEGORY_COUNT)> scores{};
    array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)> used{};
    int yahtzeeBonusCount = 0;

    Scorecard(string n = "플레이어") : name(move(n)) {
        scores.fill(0); used.fill(false);
    }
    int upperSum() const {
        int s = 0;
        for (int i = 0; i <= static_cast<int>(Category::SIXES); i++) if (used[i]) s += scores[i];
        return s;
    }
    int lowerSum() const {
        int s = 0;
        for (int i = static_cast<int>(Category::THREE_KIND); i <= static_cast<int>(Category::CHANCE); i++) if (used[i]) s += scores[i];
        return s;
    }
    int upperBonus() const { return (upperSum() >= UPPER_BONUS_THRESHOLD) ? UPPER_BONUS_SCORE : 0; }
    int total() const { return upperSum() + upperBonus() + lowerSum() + (yahtzeeBonusCount * YAHTZEE_BONUS_SCORE); }
};

// =================== Dice / Input ===================
mt19937 g_rng(random_device{}());
uniform_int_distribution<int> g_diceDist(1, 6);
inline int roll6() { return g_diceDist(g_rng); }

void rollDice(Dice& d, const array<bool, 5>& held) { for (int i = 0; i < 5; i++) if (!held[i]) d[i] = roll6(); }
vector<int> parseIndices(string line) {
    for (char& ch : line) if (ch == ',') ch = ' ';
    vector<int> idx; istringstream iss(line); int x;
    while (iss >> x) if (1 <= x && x <= 5) idx.push_back(x - 1);
    sort(idx.begin(), idx.end()); idx.erase(unique(idx.begin(), idx.end()), idx.end());
    return idx;
}

// =================== AI STRATEGY FUNCTIONS ===================

// Forward declarations for AI functions
array<int, static_cast<size_t>(Category::CATEGORY_COUNT)> calculateScores(const Dice& d);
pair<Category, int> findBestExpectedCategory_Normal(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft);
array<bool, 5> chooseBestHoldStrategy_Hard(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft, int round);
Category chooseBestScoringCategory(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int round, AIDifficulty difficulty);
Category chooseBestScoringCategory_Easy(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used);

// Easy AI: Chooses the category that gives the highest score with the current dice.
Category chooseBestScoringCategory_Easy(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used) {
    int maxScore = -1;
    Category bestCat = Category::CATEGORY_COUNT;
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); ++i) {
        if (!used[i]) {
            int currentScore = scoreCategory(static_cast<Category>(i), d);
            if (currentScore > maxScore) {
                maxScore = currentScore;
                bestCat = static_cast<Category>(i);
            }
        }
    }
    return bestCat;
}

// Normal AI: Uses Monte Carlo simulation to find the category with the highest expected score.
pair<Category, int> findBestExpectedCategory_Normal(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft) {
    int maxExpectedScore = -1;
    Category bestCategory = Category::CATEGORY_COUNT;
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); ++i) {
        if (used[i]) continue;
        long long sumScores = 0;
        const int SIMULATIONS = 2000; // Reduced for performance in a turn-based setting
        for (int k = 0; k < SIMULATIONS; ++k) {
            Dice tempDice = d;
            array<bool, 5> tempHeld{};
            tempHeld.fill(false);
            // Simple hold strategy for simulation based on target category
            switch (static_cast<Category>(i)) {
            case Category::ONES: case Category::TWOS: case Category::THREES: case Category::FOURS: case Category::FIVES: case Category::SIXES:
                for (int j = 0; j < 5; ++j) if (tempDice[j] == (i + 1)) tempHeld[j] = true;
                break;
            case Category::THREE_KIND: case Category::FOUR_KIND: case Category::YAHTZEE: {
                auto counts = countFace(tempDice);
                int targetFace = 0; int maxCount = 0;
                for (int j = 1; j <= 6; ++j) if (counts[j] > maxCount) { maxCount = counts[j]; targetFace = j; }
                for (int j = 0; j < 5; ++j) if (tempDice[j] == targetFace) tempHeld[j] = true;
                break;
            }
                                     // Other cases can be added for more sophisticated simulation
            default:
                // For simplicity, don't hold anything for complex categories in simulation
                break;
            }

            for (int r = 0; r < rollsLeft; ++r) {
                rollDice(tempDice, tempHeld);
            }
            sumScores += scoreCategory(static_cast<Category>(i), tempDice);
        }
        int expectedScore = static_cast<int>(sumScores / SIMULATIONS);
        if (expectedScore > maxExpectedScore) {
            maxExpectedScore = expectedScore;
            bestCategory = static_cast<Category>(i);
        }
    }
    return make_pair(bestCategory, maxExpectedScore);
}

// Hard AI: Uses different strategies based on the game phase and sophisticated judgment.
array<bool, 5> chooseBestHoldStrategy_Hard(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft, int round) {
    if (rollsLeft == 0) return { true, true, true, true, true };

    // 1. Instantly hold high-scoring completed categories
    if (!used[static_cast<int>(Category::YAHTZEE)] && scoreYahtzee(d) > 0) return { true, true, true, true, true };
    if (!used[static_cast<int>(Category::LARGE_STRAIGHT)] && scoreLargeStraight(d) > 0) return { true, true, true, true, true };
    if (!used[static_cast<int>(Category::FULL_HOUSE)] && scoreFullHouse(d) > 0) return { true, true, true, true, true };

    // 2. Use Monte Carlo simulation to find the best category to aim for
    pair<Category, int> bestExpected = findBestExpectedCategory_Normal(d, used, rollsLeft);
    Category bestCat = bestExpected.first;

    if (bestCat == Category::CATEGORY_COUNT) return { false, false, false, false, false };

    // 3. Determine the best hold based on the target category
    array<bool, 5> bestHeld{};
    bestHeld.fill(false);
    switch (bestCat) {
    case Category::ONES: case Category::TWOS: case Category::THREES: case Category::FOURS: case Category::FIVES: case Category::SIXES:
        for (int i = 0; i < 5; ++i) if (d[i] == (static_cast<int>(bestCat) + 1)) bestHeld[i] = true;
        break;
    case Category::THREE_KIND: case Category::FOUR_KIND: case Category::YAHTZEE: {
        auto counts = countFace(d);
        int targetFace = 0; int maxCount = 0;
        for (int i = 1; i <= 6; ++i) if (counts[i] > maxCount) { maxCount = counts[i]; targetFace = i; }
        for (int i = 0; i < 5; ++i) if (d[i] == targetFace) bestHeld[i] = true;
        break;
    }
    case Category::FULL_HOUSE: {
        auto counts = countFace(d);
        int threeKindFace = 0, twoKindFace = 0;
        for (int i = 1; i <= 6; ++i) {
            if (counts[i] >= 3) threeKindFace = i;
            else if (counts[i] >= 2) twoKindFace = i;
        }
        for (int i = 0; i < 5; ++i) if (d[i] == threeKindFace || d[i] == twoKindFace) bestHeld[i] = true;
        break;
    }
    case Category::SMALL_STRAIGHT: case Category::LARGE_STRAIGHT: {
        set<int> uniqueDice;
        array<bool, 5> heldForStraight{};
        for (int val : d) {
            if (uniqueDice.find(val) == uniqueDice.end()) {
                uniqueDice.insert(val);
                for (int i = 0; i < 5; ++i) {
                    if (d[i] == val && !heldForStraight[i]) {
                        heldForStraight[i] = true;
                        break;
                    }
                }
            }
        }
        return heldForStraight;
    }
    case Category::CHANCE:
        for (int i = 0; i < 5; ++i) if (d[i] >= 4) bestHeld[i] = true; // Keep high dice for chance
        break;
    default: break;
    }
    return bestHeld;
}

// Dispatches to the correct AI scoring strategy based on difficulty.
Category chooseBestScoringCategory(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int round, AIDifficulty difficulty) {
    if (difficulty == AIDifficulty::EASY) {
        return chooseBestScoringCategory_Easy(d, used);
    }

    // For Normal and Hard, prioritize high-value categories if available
    int maxScore = -1;
    Category bestCat = Category::CATEGORY_COUNT;

    // Try to fill Yahtzee, Straights, Full House first
    if (!used[static_cast<int>(Category::YAHTZEE)] && scoreYahtzee(d) > 0) return Category::YAHTZEE;
    if (!used[static_cast<int>(Category::LARGE_STRAIGHT)] && scoreLargeStraight(d) > 0) return Category::LARGE_STRAIGHT;
    if (!used[static_cast<int>(Category::SMALL_STRAIGHT)] && scoreSmallStraight(d) > 0) return Category::SMALL_STRAIGHT;
    if (!used[static_cast<int>(Category::FULL_HOUSE)] && scoreFullHouse(d) > 0) return Category::FULL_HOUSE;

    // Otherwise, find the best available score
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); ++i) {
        if (!used[i]) {
            int currentScore = scoreCategory(static_cast<Category>(i), d);
            if (currentScore > maxScore) {
                maxScore = currentScore;
                bestCat = static_cast<Category>(i);
            }
        }
    }

    // If no good score, sacrifice a low-value category
    if (maxScore <= 5) {
        if (!used[static_cast<int>(Category::ONES)]) return Category::ONES;
        if (!used[static_cast<int>(Category::CHANCE)] && sumDice(d) < 10) return Category::CHANCE;
    }

    return bestCat;
}


// =================== Game Rendering (High-Level) ===================

const int LEFT_W = 36;
const int LEFT_X = 0;
const int LEFT_Y = 0;
const int RIGHT_X = LEFT_W + 1;
const int RIGHT_Y = 0;

static string fitName(const string& s, int maxCells = 10) {
    string out; int cells = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        int add = (ch < 0x80) ? 1 : 2;
        if (cells + add > maxCells) break;
        out.push_back(s[i]); cells += add;
    }
    out.append(maxCells - cells, ' ');
    return out;
}

vector<string> renderDie(int v, bool held) {
    vector<string> g = {
        "+-------+", "|       |", "|       |", "|       |", "+-------+"
    };
    auto setp = [&](int r, int c) { g[r][c] = 'o'; };
    if (v == 1 || v == 3 || v == 5) setp(2, 4);
    if (v >= 2) { setp(1, 2); setp(3, 6); }
    if (v >= 4) { setp(1, 6); setp(3, 2); }
    if (v == 6) { setp(2, 2); setp(2, 6); }
    if (held) { g[0][0] = g[0][8] = 'H'; for (int i = 1; i <= 7; i++) g[0][i] = '='; }
    return g;
}

int scorePreviewLine(Category i, const Dice& d, const Scorecard& sc) {
    if (isYahtzee(d) && sc.used[static_cast<int>(Category::YAHTZEE)] && sc.scores[static_cast<int>(Category::YAHTZEE)] > 0) {
        // Joker Rules
        switch (i) {
        case Category::FULL_HOUSE: return FULL_HOUSE_SCORE;
        case Category::SMALL_STRAIGHT: return SMALL_STRAIGHT_SCORE;
        case Category::LARGE_STRAIGHT: return LARGE_STRAIGHT_SCORE;
        case Category::ONES: case Category::TWOS: case Category::THREES: case Category::FOURS: case Category::FIVES: case Category::SIXES:
            // Must score in the corresponding upper section if available
            if (!sc.used[d[0] - 1]) return scoreUpper(d, d[0]);
            // Fallthrough if upper section is used
        default: return scoreCategory(i, d);
        }
    }
    return scoreCategory(i, d);
}

void redrawAll(int round, int p, int rollsLeft, const Dice& dice, const array<bool, 5>& held, const vector<Scorecard>& players, const string& promptMsg, const string& errorMsg = "", const string& statusMsg = "") {
    clearScreen();

    // 1. Draw left scorecard panel
    writeAt(LEFT_X, LEFT_Y, "      == 야추 점수판 ==");
    for (int y = 1; y < 45; ++y) writeAt(LEFT_W, y, "|");

    const Scorecard& sc = players[p];
    int y = LEFT_Y + 2;

    for (int i = 0; i <= static_cast<int>(Category::SIXES); ++i) {
        ostringstream line;
        line << " " << setw(2) << (i + 1) << ") " << left << setw(18) << CAT_NAME[i] << " : ";
        if (sc.used[i]) line << sc.scores[i]; else line << "(미사용)";
        writeAt(LEFT_X, y++, line.str());
    }

    y++;
    ostringstream subTotal;
    subTotal << " " << left << setw(23) << "서브 토탈" << " : " << sc.upperSum() << " / " << UPPER_BONUS_THRESHOLD;
    writeAt(LEFT_X, y++, subTotal.str());
    ostringstream bonus;
    bonus << " " << left << setw(23) << "+35 보너스" << " : " << sc.upperBonus();
    writeAt(LEFT_X, y++, bonus.str());
    y++;

    for (int i = static_cast<int>(Category::THREE_KIND); i <= static_cast<int>(Category::CHANCE); ++i) {
        ostringstream line;
        line << " " << setw(2) << (i + 1) << ") " << left << setw(18) << CAT_NAME[i] << " : ";
        if (sc.used[i]) line << sc.scores[i]; else line << "(미사용)";
        writeAt(LEFT_X, y++, line.str());
    }
    y++;
    ostringstream yahtzeeBonus;
    yahtzeeBonus << "야추 보너스 x " << sc.yahtzeeBonusCount << " : " << sc.yahtzeeBonusCount * YAHTZEE_BONUS_SCORE;
    writeAt(LEFT_X, y++, yahtzeeBonus.str());
    y++;
    ostringstream sum3; sum3 << "총점: " << sc.total(); writeAt(LEFT_X, y++, sum3.str());
    y += 2;
    writeAt(LEFT_X, y++, "--- 플레이어 점수 총합 ---");
    for (size_t i = 0; i < players.size(); ++i) {
        const auto& pl = players[i];
        ostringstream line;
        line << (pl.name == sc.name ? " > " : "   ") << fitName(pl.name, 10) << " : " << pl.total();
        writeAt(LEFT_X, y++, line.str());
    }

    // 2. Draw right game panel
    ostringstream hdr; hdr << "--- 라운드 " << round << " / 13 --- (" << sc.name << " 님)"; writeAt(RIGHT_X, RIGHT_Y, hdr.str());
    ostringstream rollsStr; rollsStr << "남은 굴리기: " << rollsLeft; writeAt(RIGHT_X, RIGHT_Y + 1, rollsStr.str());

    vector<vector<string>> arts;
    arts.reserve(5);
    for (int i = 0; i < 5; ++i) {
        arts.push_back(renderDie(dice[i], held[i]));
    }

    int dice_y = RIGHT_Y + 3;
    for (int r = 0; r < 5; r++) {
        ostringstream line;
        for (int i = 0; i < 5; i++) line << arts[i][r] << ' ';
        writeAt(RIGHT_X, dice_y + 1 + r, line.str());
    }
    writeAt(RIGHT_X, dice_y + 7, "    (1)       (2)       (3)       (4)       (5)");

    int potential_y = dice_y + 10;
    int row = potential_y + 1;
    writeAt(RIGHT_X, potential_y, "--- 현재 주사위로 가능한 점수 ---");
    if (isYahtzee(dice) && sc.used[static_cast<int>(Category::YAHTZEE)] && sc.scores[static_cast<int>(Category::YAHTZEE)] > 0) {
        writeAt(RIGHT_X, potential_y, "--- 현재 주사위로 가능한 점수 (야추 조커 규칙 적용!) ---");
    }
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); i++) {
        if (!sc.used[i]) {
            ostringstream line;
            line << " " << setw(2) << (i + 1) << ") " << left << setw(18) << CAT_NAME[i]
                << " = " << scorePreviewLine(static_cast<Category>(i), dice, sc);
            writeAt(RIGHT_X, row++, line.str());
        }
    }

    // 3. Draw instructions
    int guide_y = 30;
    writeAt(RIGHT_X, guide_y++, "--------------------------------------------------");
    writeAt(RIGHT_X, guide_y++, "규칙: 주사위는 최대 3번까지 굴릴 수 있습니다.");
    writeAt(RIGHT_X, guide_y++, "입력: [T] 홀드, [R] 다시 굴리기, [S] 점수 선택");
    writeAt(RIGHT_X, guide_y++, "예시: T 1 3 (1번, 3번 주사위를 홀드/해제합니다)");
    writeAt(RIGHT_X, guide_y++, "--------------------------------------------------");

    // 4. Draw status message (bonus, score recorded, etc.)
    if (!statusMsg.empty()) {
        writeAt(RIGHT_X, guide_y++, statusMsg);
    }

    // 5. Display prompt and error messages
    int prompt_y = 38;
    writeAt(RIGHT_X, prompt_y, promptMsg);
    if (!errorMsg.empty()) {
        writeAt(RIGHT_X, prompt_y + 1, errorMsg);
    }
    gotoXY(RIGHT_X + (int)promptMsg.size(), prompt_y);
}


// =================== Main ===================
int main() {
    ios::sync_with_stdio(false); cin.tie(nullptr);

#ifdef _WIN32
    HOUT = GetStdHandle(STD_OUTPUT_HANDLE);
    ensureConsoleSize(120, 45);
#endif
    hideCursor(true);

    clearScreen();
    writeAt(5, 2, "===== 야추 (콘솔, 픽셀 아트 주사위) =====");
    writeAt(5, 4, "규칙: 5개의 주사위를 최대 3번까지 굴린 뒤, 13개 카테고리에 각 1회씩 점수 기록.");

    int numHumanPlayers = 0, numAIPlayers = 0;
    writeAt(5, 7, "사람 플레이어 수를 입력하세요(0-5): ");
    while (!(cin >> numHumanPlayers) || numHumanPlayers < 0 || numHumanPlayers > 5) {
        cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        writeAt(5, 8, "0~5 사이의 올바른 숫자를 입력: ");
    }
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');

    writeAt(5, 9, "컴퓨터 플레이어 수를 입력하세요(0-5): ");
    while (!(cin >> numAIPlayers) || numAIPlayers < 0 || numAIPlayers > 5) {
        cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        writeAt(5, 10, "0~5 사이의 올바른 숫자를 입력: ");
    }
    cin.ignore((numeric_limits<streamsize>::max)(), '\n');

    if (numHumanPlayers + numAIPlayers == 0) return 0;

    vector<Scorecard> players;
    vector<bool> is_computer;
    vector<AIDifficulty> computer_difficulty;

    for (int i = 0; i < numHumanPlayers; i++) {
        string prompt = "사람 플레이어 " + to_string(i + 1) + " 이름: ";
        writeAt(5, 12 + i, prompt);
        string name; getline(cin, name);
        if (name.empty()) name = "플레이어" + to_string(i + 1);
        players.emplace_back(name);
        is_computer.push_back(false);
    }

    for (int i = 0; i < numAIPlayers; i++) {
        string prompt = "컴퓨터 " + to_string(i + 1) + " 난이도 (1:쉬움, 2:보통, 3:어려움): ";
        writeAt(5, 12 + numHumanPlayers + i, prompt);
        int diff_choice;
        while (!(cin >> diff_choice) || diff_choice < 1 || diff_choice > 3) {
            cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            writeAt(5, 13 + numHumanPlayers + i, "1, 2, 3 중 하나를 입력하세요: ");
        }
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');
        string name = "컴퓨터" + to_string(i + 1);
        players.emplace_back(name);
        is_computer.push_back(true);
        computer_difficulty.push_back(static_cast<AIDifficulty>(diff_choice - 1));
    }

    Dice dice{ 1, 2, 3, 4, 5 };
    array<bool, 5> held{};

    for (int round = 1; round <= 13; ++round) {
        for (size_t p = 0; p < players.size(); ++p) {

            string turn_prompt = players[p].name + " 님 차례입니다. Enter 키를 눌러 굴리세요...";
            if (is_computer[p]) turn_prompt = players[p].name + " 님의 차례입니다. 잠시 후 시작합니다...";
            redrawAll(round, p, 3, dice, held, players, turn_prompt);

            if (is_computer[p]) {
                this_thread::sleep_for(chrono::seconds(2));
            }
            else {
                string dummy; getline(cin, dummy);
            }

            held.fill(false);
            int rolls = 0;
            rollDice(dice, held);
            rolls++;

            if (is_computer[p]) {
                // AI Player Turn Logic
                AIDifficulty diff = computer_difficulty[p - numHumanPlayers];
                while (rolls < 3) {
                    redrawAll(round, p, 3 - rolls, dice, held, players, "컴퓨터가 생각 중입니다...", "");
                    this_thread::sleep_for(chrono::seconds(2));

                    if (diff == AIDifficulty::EASY) {
                        held.fill(false); // Easy AI doesn't hold
                    }
                    else { // Normal and Hard use the same hold strategy
                        held = chooseBestHoldStrategy_Hard(dice, players[p].used, 3 - rolls, round);
                    }

                    if (all_of(held.begin(), held.end(), [](bool v) { return v; })) {
                        redrawAll(round, p, 3 - rolls, dice, held, players, "컴퓨터가 모든 주사위를 홀드합니다.", "");
                        this_thread::sleep_for(chrono::seconds(2));
                        break; // Stop rolling if all dice are held
                    }

                    redrawAll(round, p, 3 - rolls, dice, held, players, "컴퓨터가 주사위를 다시 굴립니다...", "");
                    this_thread::sleep_for(chrono::seconds(2));

                    rollDice(dice, held);
                    rolls++;
                }

            }
            else {
                // Human Player Turn Logic
                string errorMsg = "";
                while (true) {
                    string prompt = (rolls >= 3) ? "점수를 기록해야 합니다. [S] 입력: " : "명령 입력 ([T] 홀드, [R] 굴리기, [S] 점수기록): ";
                    redrawAll(round, p, 3 - rolls, dice, held, players, prompt, errorMsg);
                    errorMsg = "";

                    string line; getline(cin, line);
                    if (line.empty()) continue;
                    istringstream iss(line); char cmd; iss >> cmd;

                    if (cmd == 'S' || cmd == 's') break;

                    if (cmd == 'T' || cmd == 't') {
                        string rest; getline(iss, rest);
                        vector<int> idx = parseIndices(rest);
                        if (idx.empty()) errorMsg = "토글할 인덱스를 공백/쉼표로 입력 (예: T 1 3 5)";
                        else for (int i : idx) held[i] = !held[i];
                    }
                    else if (cmd == 'R' || cmd == 'r') {
                        if (rolls >= 3) errorMsg = "더 이상 굴릴 수 없습니다.";
                        else { rollDice(dice, held); rolls++; }
                    }
                    else {
                        errorMsg = "알 수 없는 명령. T/R/S 사용.";
                    }
                }
            }

            // Scoring phase for both human and AI
            if (isYahtzee(dice) && players[p].used[static_cast<int>(Category::YAHTZEE)] && players[p].scores[static_cast<int>(Category::YAHTZEE)] > 0) {
                players[p].yahtzeeBonusCount++;
            }

            int pick = -1;
            Category chosenCat;

            if (is_computer[p]) {
                AIDifficulty diff = computer_difficulty[p - numHumanPlayers];
                redrawAll(round, p, 0, dice, held, players, "컴퓨터가 기록할 점수를 선택 중입니다...", "");
                this_thread::sleep_for(chrono::seconds(2));
                chosenCat = chooseBestScoringCategory(dice, players[p].used, round, diff);
                pick = static_cast<int>(chosenCat) + 1;
            }
            else {
                while (true) {
                    string prompt = "기록할 카테고리 번호를 선택하세요: ";
                    string status = "";
                    if (isYahtzee(dice) && players[p].used[static_cast<int>(Category::YAHTZEE)] && players[p].scores[static_cast<int>(Category::YAHTZEE)] > 0) {
                        status = "### 야추 보너스! +100점! ###";
                    }

                    redrawAll(round, p, 0, dice, held, players, prompt, "", status);

                    if ((cin >> pick) && 1 <= pick && pick <= static_cast<int>(Category::CATEGORY_COUNT) && !players[p].used[pick - 1]) break;

                    cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                    redrawAll(round, p, 0, dice, held, players, prompt, "유효하지 않거나 이미 사용됨. 다시 입력.", status);
                    { string dummy; getline(cin, dummy); }
                }
                cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                chosenCat = static_cast<Category>(pick - 1);
            }

            int sc = scorePreviewLine(chosenCat, dice, players[p]);
            players[p].used[pick - 1] = true;
            players[p].scores[pick - 1] = sc;

            string finalStatus = "'" + CAT_NAME[pick - 1] + "'에 " + to_string(sc) + "점을 기록했습니다.";
            redrawAll(round, p, 0, dice, held, players, "Enter 키를 누르면 다음으로...", "", finalStatus);

            if (is_computer[p]) {
                this_thread::sleep_for(chrono::seconds(3));
            }
            else {
                string dummy; getline(cin, dummy);
            }
        }
    }

    // Final results
    clearScreen();
    sort(players.begin(), players.end(), [](const Scorecard& a, const Scorecard& b) { return a.total() > b.total(); });

    writeAt(5, 2, "================ 최종 결과 ================");
    int y_pos = 4;
    for (size_t rank = 0; rank < players.size(); ++rank) {
        const auto& pl = players[rank];
        writeAt(5, y_pos++, to_string(rank + 1) + "위: " + pl.name + " - 총점 " + to_string(pl.total()) + "점");
        y_pos++;
        writeAt(7, y_pos++, "상단 보너스: " + to_string(pl.upperBonus()));
        writeAt(7, y_pos++, "야추 보너스: " + to_string(pl.yahtzeeBonusCount * YAHTZEE_BONUS_SCORE));
        y_pos += 2;
    }
    writeAt(5, y_pos + 2, "플레이해주셔서 감사합니다!");

    hideCursor(false);
    gotoXY(0, y_pos + 4);
    return 0;
}
