#include "gameUI.h"

// =================== 전역 변수 정의 ===================
const int LEFT_W = 36;
const int LEFT_X = 0;
const int LEFT_Y = 0;
const int RIGHT_X = LEFT_W + 1;
const int RIGHT_Y = 0;

// =================== 콘솔 및 저수준 렌더링 함수 ===================
#ifdef _WIN32
HANDLE HOUT;
wstring s2ws(const std::string& s) {
    if (s.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &s[0], (int)s.size(), &wstrTo[0], size_needed);
    return wstrTo;
}
void hideCursor(bool hide) {
    CONSOLE_CURSOR_INFO ci;
    ci.dwSize = 20; ci.bVisible = hide ? FALSE : TRUE;
    SetConsoleCursorInfo(HOUT, &ci);
}
void gotoXY(int x, int y) {
    COORD c; c.X = (SHORT)x; c.Y = (SHORT)y; SetConsoleCursorPosition(HOUT, c);
}
void writeAt(int x, int y, const string& s) {
    gotoXY(x, y);
    wstring ws = s2ws(s);
    DWORD w;
    WriteConsoleW(HOUT, ws.c_str(), (DWORD)ws.size(), &w, nullptr);
}
void ensureConsoleSize(int cols, int rows) {
    SMALL_RECT tiny = { 0, 0, 1, 1 };
    SetConsoleWindowInfo(HOUT, TRUE, &tiny);
    COORD buf = { (SHORT)cols, (SHORT)rows };
    SetConsoleScreenBufferSize(HOUT, buf);
    SMALL_RECT win = { 0, 0, (SHORT)(cols - 1), (SHORT)(rows - 1) };
    SetConsoleWindowInfo(HOUT, TRUE, &win);
}
void clearScreen() { system("cls"); }
int getConsoleCols() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(HOUT, &csbi);
    return csbi.srWindow.Right - csbi.srWindow.Left + 1;
}
int getConsoleRows() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(HOUT, &csbi);
    return csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
}
#else
void hideCursor(bool hide) { if (hide) cout << "\x1b[?25l"; else cout << "\x1b[?25h"; }
void gotoXY(int x, int y) { cout << "\x1b[" << (y + 1) << ";" << (x + 1) << "H"; }
void writeAt(int x, int y, const string& s) { gotoXY(x, y); cout << s; cout.flush(); }
void ensureConsoleSize(int, int) {}
void clearScreen() { system("clear"); }
int getConsoleCols() { return 120; }
int getConsoleRows() { return 60; }
#endif

// 유틸: 세로 구분선/가로줄/오른쪽 지우기
void drawVerticalSep(int col, int y_top, int y_bottom, const string& glyph) {
    for (int y = y_top; y <= y_bottom; ++y) writeAt(col, y, glyph);
}
void drawHRule(int x, int y) {
    int cols = getConsoleCols();
    int width = max(1, cols - x - 1);
    writeAt(x, y, string(width, '-'));
}
void clearRightPaneFrom(int start_y) {
    int cols = getConsoleCols();
    int rows = getConsoleRows();
    int width = max(1, cols - RIGHT_X - 1);
    for (int y = start_y; y < rows; ++y) writeAt(RIGHT_X, y, string(width, ' '));
}
int getVisualWidth(const string& s) {
    int width = 0;
    for (size_t i = 0; i < s.length(); ) {
        unsigned char ch = s[i];
        if (ch < 0x80) { width++; i++; }
        else {
            width += 2;
            if ((ch & 0xE0) == 0xC0) i += 2;
            else if ((ch & 0xF0) == 0xE0) i += 3;
            else i++;
        }
    }
    return width;
}
string fitName(const string& s, int maxCells) {
    string out; int cells = 0;
    for (size_t i = 0; i < s.size();) {
        unsigned char ch = static_cast<unsigned char>(s[i]);
        int add = (ch < 0x80) ? 1 : 2;
        if (cells + add > maxCells) break;
        if (add == 1) { out.push_back(s[i]); i++; }
        else { out.append(s.substr(i, 3)); i += 3; }
        cells += add;
    }
    out.append(maxCells - cells, ' ');
    return out;
}
vector<string> renderDie(int v, bool held) {
    vector<string> g = {
        "+-------+",
        "|       |",
        "|       |",
        "|       |",
        "+-------+"
    };
    if (v < 1 || v > 6) return g;
    auto setp = [&](int r, int c) { g[r][c] = 'o'; };
    if (v == 1 || v == 3 || v == 5) setp(2, 4);
    if (v >= 2) { setp(1, 2); setp(3, 6); }
    if (v >= 4) { setp(1, 6); setp(3, 2); }
    if (v == 6) { setp(2, 2); setp(2, 6); }
    if (held) g[0] = "H=======H";
    return g;
}
int scorePreviewLine(Category i, const Dice& d, const Scorecard& sc) {
    if (isYahtzee(d) && sc.used[static_cast<int>(Category::YAHTZEE)] && sc.scores[static_cast<int>(Category::YAHTZEE)] > 0) {
        switch (i) {
        case Category::FULL_HOUSE: return FULL_HOUSE_SCORE;
        case Category::SMALL_STRAIGHT: return SMALL_STRAIGHT_SCORE;
        case Category::LARGE_STRAIGHT: return LARGE_STRAIGHT_SCORE;
        case Category::ONES: case Category::TWOS: case Category::THREES: case Category::FOURS: case Category::FIVES: case Category::SIXES:
            if (!sc.used[d[0] - 1]) return scoreUpper(d, d[0]);
        default: return scoreCategory(i, d);
        }
    }
    return scoreCategory(i, d);
}
void redrawDiceOnly(const Dice& dice, const array<bool, 5>& held, int start_y) {
    vector<vector<string>> arts; arts.reserve(5);
    for (int i = 0; i < 5; ++i) arts.push_back(renderDie(dice[i], held[i]));
    for (int r = 0; r < 5; r++) {
        ostringstream line;
        for (int i = 0; i < 5; i++) line << arts[i][r] << ' ';
        writeAt(RIGHT_X, start_y + 1 + r, line.str());
    }
}
void displayImpactEffect(const string& combinationName) {
    int effectX = RIGHT_X + 45;
    int effectY = RIGHT_Y + 15;
    string border = "+-------------------------+";
    string empty = "|                         |";
    string text = "!! " + combinationName + " !!";
    int textX = effectX + (border.length() - getVisualWidth(text)) / 2;
    writeAt(effectX, effectY, border);
    writeAt(effectX, effectY + 1, empty);
    writeAt(textX, effectY + 2, text);
    writeAt(effectX, effectY + 3, empty);
    writeAt(effectX, effectY + 4, border);
    this_thread::sleep_for(chrono::milliseconds(1500));
}
void redrawAll(int round, int p_idx, int rollsLeft, const Dice& dice, const array<bool, 5>& held, const vector<Scorecard>& players, const string& promptMsg, const string& errorMsg, const string& statusMsg) {
    clearScreen();

    // 왼쪽 패널 제목
    writeAt(LEFT_X, LEFT_Y, u8"      == 야추 점수판 ==");
    // 동적 구분선
    drawVerticalSep(LEFT_W, 1, getConsoleRows() - 1);

    const Scorecard& sc = players[p_idx];
    int y = LEFT_Y + 2;
    const int ALIGN_COL = 26;

    for (int i = 0; i <= static_cast<int>(Category::SIXES); ++i) {
        ostringstream line_prefix;
        line_prefix << " " << setw(2) << (i + 1) << ") " << CAT_NAME[i];
        string prefix_str = line_prefix.str();
        int padding = ALIGN_COL - getVisualWidth(prefix_str);
        if (padding < 1) padding = 1;
        ostringstream final_line;
        final_line << prefix_str << string(padding, ' ') << ": " << (sc.used[i] ? to_string(sc.scores[i]) : u8"(미사용)");
        writeAt(LEFT_X, y++, final_line.str());
    }
    y++;
    string subTotalStr = u8" 서브 토탈";
    writeAt(LEFT_X, y++, subTotalStr + string(ALIGN_COL - getVisualWidth(subTotalStr), ' ') + ": " + to_string(sc.upperSum()) + " / " + to_string(UPPER_BONUS_THRESHOLD));
    string bonusStr = u8" +35 보너스";
    writeAt(LEFT_X, y++, bonusStr + string(ALIGN_COL - getVisualWidth(bonusStr), ' ') + ": " + to_string(sc.upperBonus()));
    y++;

    for (int i = static_cast<int>(Category::THREE_KIND); i <= static_cast<int>(Category::CHANCE); ++i) {
        ostringstream line_prefix;
        line_prefix << " " << setw(2) << (i + 1) << ") " << CAT_NAME[i];
        string prefix_str = line_prefix.str();
        int padding = ALIGN_COL - getVisualWidth(prefix_str);
        if (padding < 1) padding = 1;
        ostringstream final_line;
        final_line << prefix_str << string(padding, ' ') << ": " << (sc.used[i] ? to_string(sc.scores[i]) : u8"(미사용)");
        writeAt(LEFT_X, y++, final_line.str());
    }
    y++;
    string yahtzeeBonusStr = u8" 야추 보너스 x " + to_string(sc.yahtzeeBonusCount);
    writeAt(LEFT_X, y++, yahtzeeBonusStr + string(ALIGN_COL - getVisualWidth(yahtzeeBonusStr), ' ') + ": " + to_string(sc.yahtzeeBonusCount * YAHTZEE_BONUS_SCORE));
    y++;
    writeAt(LEFT_X, y++, u8" 총점: " + to_string(sc.total()));
    y += 2;
    writeAt(LEFT_X, y++, u8"--- 플레이어 점수 총합 ---");
    for (size_t i = 0; i < players.size(); ++i) {
        const auto& pl = players[i];
        ostringstream line; line << (pl.name == sc.name ? " > " : "   ") << fitName(pl.name, 10) << " : " << pl.total();
        writeAt(LEFT_X, y++, line.str());
    }

    // 오른쪽 패널
    ostringstream hdr; hdr << u8"--- 라운드 " << round << u8" / 13 --- (" << sc.name << u8" 님)";
    writeAt(RIGHT_X, RIGHT_Y, hdr.str());
    ostringstream rollsStr; rollsStr << u8"남은 굴리기: " << rollsLeft; writeAt(RIGHT_X, RIGHT_Y + 1, rollsStr.str());

    int guide_y = RIGHT_Y + 3;
    drawHRule(RIGHT_X, guide_y++);
    writeAt(RIGHT_X, guide_y++, u8"규칙: 주사위는 최대 3번까지 굴릴 수 있습니다.");
    writeAt(RIGHT_X, guide_y++, u8"입력: [T] 홀드, [R] 다시 굴리기, [S] 점수 선택");
    writeAt(RIGHT_X, guide_y++, u8"예시: T 1 3 (1번, 3번 주사위를 홀드/해제합니다)");
    drawHRule(RIGHT_X, guide_y++);

    int dice_start_y = guide_y;
    redrawDiceOnly(dice, held, dice_start_y);
    writeAt(RIGHT_X, dice_start_y + 6, "   (1)      (2)      (3)      (4)      (5)");

    int potential_y = dice_start_y + 9;
    writeAt(RIGHT_X, potential_y, u8"--- 현재 주사위로 가능한 점수 ---");
    if (isYahtzee(dice) && sc.used[static_cast<int>(Category::YAHTZEE)] && sc.scores[static_cast<int>(Category::YAHTZEE)] > 0)
        writeAt(RIGHT_X, potential_y, u8"--- 현재 주사위로 가능한 점수 (조커!) ---");

    int row = potential_y + 1;
    for (int i = 0; i < static_cast<int>(Category::CATEGORY_COUNT); i++) if (!sc.used[i]) {
        ostringstream line_prefix; line_prefix << " " << setw(2) << (i + 1) << ") " << CAT_NAME[i];
        string prefix_str = line_prefix.str();
        int visual_width = getVisualWidth(prefix_str);
        int padding_needed = 28 - visual_width; if (padding_needed < 1) padding_needed = 1;
        ostringstream final_line; final_line << prefix_str << string(padding_needed, ' ')
            << "= " << scorePreviewLine(static_cast<Category>(i), dice, sc);
        writeAt(RIGHT_X, row++, final_line.str());
    }

    // 오른쪽 잔상 지우기(동적)
    clearRightPaneFrom(row);

    int message_y = row;
    int prompt_y = row + 1;

    if (!errorMsg.empty()) writeAt(RIGHT_X, message_y, errorMsg);
    else if (!statusMsg.empty()) writeAt(RIGHT_X, message_y, statusMsg);

    writeAt(RIGHT_X, prompt_y, promptMsg);
    gotoXY(RIGHT_X + getVisualWidth(promptMsg), prompt_y);
}
void animateRoll(Dice& dice, const array<bool, 5>& held) {
    const int totalFrames = 20;
    vector<vector<int>> sequences(5);
    Dice tempDice = dice;
    for (int i = 0; i < 5; ++i) if (!held[i]) sequences[i] = generateDiceSequence(totalFrames, 4);

    for (int frame = 0; frame < totalFrames; ++frame) {
        for (int i = 0; i < 5; ++i) if (!held[i]) tempDice[i] = sequences[i][frame];
        int dice_start_y = (RIGHT_Y + 3) + 5;
        redrawDiceOnly(tempDice, held, dice_start_y);
        this_thread::sleep_for(chrono::milliseconds(60));
    }
    for (int i = 0; i < 5; ++i) if (!held[i]) dice[i] = sequences[i].back();
}