#include "gamePlay.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "gameUI.h"
#include <sstream>

void run_yahtzee_game(GameDB& db) {
    ios::sync_with_stdio(false); cin.tie(nullptr);
#ifdef _WIN32
    system("chcp 65001 > nul");
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    setvbuf(stdout, nullptr, _IOFBF, 1000);
    HOUT = GetStdHandle(STD_OUTPUT_HANDLE);
    ensureConsoleSize(120, 60);
#endif
    hideCursor(true);

    while (true) {
        clearScreen();
        writeAt(5, 2, u8"===== ���� (�ܼ�, �ȼ� ��Ʈ �ֻ���) =====");
        writeAt(5, 4, u8"��Ģ: 5���� �ֻ����� �ִ� 3������ ���� ��, 13�� ī�װ��� �� 1ȸ�� ���� ���.");

        int gameMode = 0;
        writeAt(5, 7, u8"--- ���� ��� ���� ---");
        writeAt(5, 8, u8"1. ���� ��Ƽ�÷��� (Player vs Player)");
        writeAt(5, 9, u8"2. AI ���� (Player vs AI)");
        writeAt(5, 10, u8"3. AI ���� (AI vs AI)");
        writeAt(5, 11, u8"4. ���� Ȯ��");
        writeAt(5, 12, u8"5. ����");
        writeAt(5, 14, u8"���ϴ� ����� ��ȣ�� �Է��ϼ���: ");

        while (!(cin >> gameMode) || gameMode < 1 || gameMode > 5) { // 1~5���� �Է� ����
            cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            writeAt(5, 15, string(40, ' '));
            writeAt(5, 15, u8"1~5 ������ �ùٸ� ���ڸ� �Է��ϼ���: ");
        }
        cin.ignore((numeric_limits<streamsize>::max)(), '\n');

        if (gameMode == 5) break; // ����
        if (gameMode == 4) {
            displayTopScores(db);
            continue; // �ٽ� �޴��� ���ư�
        }

        // ���� ��� ���� ����
        int numHumanPlayers = 0, numAIPlayers = 0;
        clearScreen();
        switch (gameMode) {
        case 1:
            writeAt(5, 2, u8"--- ���� ��Ƽ�÷��� ---");
            writeAt(5, 4, u8"�÷��̾� ���� �Է��ϼ���(1-5): ");
            while (!(cin >> numHumanPlayers) || numHumanPlayers < 1 || numHumanPlayers > 5) {
                cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                writeAt(5, 5, string(40, ' '));
                writeAt(5, 5, u8"1~5 ������ �ùٸ� ���ڸ� �Է�: ");
            }
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            numAIPlayers = 0; break;
        case 2:
            writeAt(5, 2, u8"--- AI ���� ---");
            numHumanPlayers = 1;
            writeAt(5, 4, u8"����� AI �÷��̾� ���� �Է��ϼ���(1-4): ");
            while (!(cin >> numAIPlayers) || numAIPlayers < 1 || numAIPlayers > 4) {
                cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                writeAt(5, 5, string(40, ' '));
                writeAt(5, 5, u8"1~4 ������ �ùٸ� ���ڸ� �Է�: ");
            }
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            break;
        case 3:
            writeAt(5, 2, u8"--- AI ���� ��� ---");
            numHumanPlayers = 0;
            writeAt(5, 4, u8"������ AI �÷��̾� ���� �Է��ϼ���(2-5): ");
            while (!(cin >> numAIPlayers) || numAIPlayers < 2 || numAIPlayers > 5) {
                cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                writeAt(5, 5, string(40, ' '));
                writeAt(5, 5, u8"2~5 ������ �ùٸ� ���ڸ� �Է�: ");
            }
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            break;
        }

        vector<Scorecard> players;
        vector<bool> is_computer;
        vector<AIDifficulty> ai_difficulties;
        int current_y_prompt = 8;

        for (int i = 0; i < numHumanPlayers; i++) {
            string prompt = u8"��� �÷��̾� " + to_string(i + 1) + u8" �̸�: ";
            writeAt(5, current_y_prompt++, prompt);
            string name; getline(cin, name);
            if (name.empty()) name = u8"�÷��̾�" + to_string(i + 1);
            players.emplace_back(name);
            is_computer.push_back(false);
        }
        for (int i = 0; i < numAIPlayers; i++) {
            string prompt = u8"��ǻ�� " + to_string(i + 1) + u8" ���̵� (1:����, 2:����, 3:�����): ";
            writeAt(5, current_y_prompt, prompt);
            int diff_choice;
            while (!(cin >> diff_choice) || diff_choice < 1 || diff_choice > 3) {
                cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                writeAt(5, current_y_prompt + 1, string(40, ' '));
                writeAt(5, current_y_prompt + 1, u8"1, 2, 3 �� �ϳ��� �Է��ϼ���: ");
            }
            cin.ignore((numeric_limits<streamsize>::max)(), '\n');
            string name = u8"��ǻ��" + to_string(i + 1);
            players.emplace_back(name);
            is_computer.push_back(true);
            ai_difficulties.push_back(static_cast<AIDifficulty>(diff_choice - 1));
            current_y_prompt += 2;
        }

        Dice dice{ 1, 1, 1, 1, 1 };
        array<bool, 5> held{};

        for (int round = 1; round <= 13; ++round) {
            for (size_t p = 0; p < players.size(); ++p) {
                string turn_prompt = players[p].name + u8" �� �����Դϴ�. Enter Ű�� ���� ��������...";
                if (is_computer[p]) turn_prompt = players[p].name + u8" ���� �����Դϴ�. ��� �� �����մϴ�...";
                redrawAll(round, (int)p, 3, dice, held, players, turn_prompt);

                if (is_computer[p]) this_thread::sleep_for(chrono::seconds(2));
                else { string dummy; getline(cin, dummy); }

                held.fill(false);
                int rolls = 0;

                redrawAll(round, (int)p, 3 - rolls, dice, held, players, "", "", u8"�ֻ����� �����ϴ�...");
                animateRoll(dice, held);
                rolls++;

                string statusMsg = "";
                string errorMsg = "";
                string combination = checkForSpecialCombinations(dice);
                if (!combination.empty()) displayImpactEffect(combination);

                bool turn_over = false;
                while (!turn_over && rolls < 3) {
                    if (is_computer[p]) {
                        this_thread::sleep_for(chrono::seconds(1));
                        AIDifficulty difficulty = ai_difficulties[p - numHumanPlayers];

                        if (difficulty == AIDifficulty::EASY) {
                            if (rolls < 3) {
                                redrawAll(round, (int)p, 3 - rolls, dice, held, players, "", "", u8"�ֻ����� �����ϴ�...");
                                animateRoll(dice, held);
                                rolls++;
                                combination = checkForSpecialCombinations(dice);
                                if (!combination.empty()) displayImpactEffect(combination);
                            }
                            else {
                                turn_over = true;
                            }
                        }
                        else {
                            held = chooseBestHoldStrategy_Hard(dice, players[p].used, 3 - rolls, round);
                            bool all_held = true;
                            for (bool h : held) if (!h) { all_held = false; break; }
                            if (all_held) {
                                turn_over = true;
                            }
                            else {
                                redrawAll(round, (int)p, 3 - rolls, dice, held, players, "", "", u8"�ֻ����� �����ϴ�...");
                                animateRoll(dice, held);
                                rolls++;
                                combination = checkForSpecialCombinations(dice);
                                if (!combination.empty()) displayImpactEffect(combination);
                            }
                        }
                    }
                    else {
                        string human_prompt = "[T]oggle, [R]eroll, [S]core: ";
                        redrawAll(round, (int)p, 3 - rolls, dice, held, players, human_prompt, errorMsg, statusMsg);
                        errorMsg = ""; statusMsg = "";
                        string line, cmd_str; getline(cin, line);
                        istringstream iss(line); iss >> cmd_str;
                        char command = (cmd_str.empty()) ? ' ' : (char)toupper(cmd_str[0]);

                        if (command == 'T') {
                            size_t first_digit_pos = line.find_first_of("0123456789");
                            if (first_digit_pos == string::npos) errorMsg = u8"�߸��� ��ɾ��Դϴ�. (��: t 1 2)";
                            else {
                                vector<int> indices = parseIndices(line.substr(first_digit_pos));
                                for (int idx : indices) held[idx] = !held[idx];
                            }
                        }
                        else if (command == 'R') {
                            redrawAll(round, (int)p, 3 - rolls, dice, held, players, human_prompt, "", u8"�ֻ����� �����ϴ�...");
                            animateRoll(dice, held);
                            rolls++;
                            combination = checkForSpecialCombinations(dice);
                            if (!combination.empty()) displayImpactEffect(combination);
                        }
                        else if (command == 'S') {
                            turn_over = true;
                        }
                        else {
                            errorMsg = u8"�߸��� ��ɾ��Դϴ�.";
                        }
                    }
                }

                Category chosenCat;
                if (is_computer[p]) {
                    redrawAll(round, (int)p, 3 - rolls, dice, held, players, "", "", players[p].name + u8" ���� ������ �����մϴ�...");
                    this_thread::sleep_for(chrono::seconds(2));
                    chosenCat = chooseBestScoringCategory(dice, players[p].used, round, ai_difficulties[p - numHumanPlayers]);
                }
                else {
                    bool score_chosen = false; errorMsg = "";
                    while (!score_chosen) {
                        redrawAll(round, (int)p, 3 - rolls, dice, held, players, u8"����� ī�װ� ��ȣ�� �Է��ϼ���: ", errorMsg);
                        errorMsg = "";
                        int cat_idx;
                        if (cin >> cat_idx && cat_idx >= 1 && cat_idx <= 13) {
                            chosenCat = static_cast<Category>(cat_idx - 1);
                            if (!players[p].used[cat_idx - 1]) score_chosen = true;
                            else errorMsg = u8"�̹� ���� ī�װ��Դϴ�.";
                        }
                        else {
                            errorMsg = u8"1~13 ������ ���ڸ� �Է��ϼ���.";
                            cin.clear(); cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                        }
                    }
                    cin.ignore((numeric_limits<streamsize>::max)(), '\n');
                }

                bool hadYahtzeeScored =
                    players[p].used[static_cast<int>(Category::YAHTZEE)] &&
                    players[p].scores[static_cast<int>(Category::YAHTZEE)] > 0;

                int score = scorePreviewLine(chosenCat, dice, players[p]);
                players[p].scores[static_cast<int>(chosenCat)] = score;
                players[p].used[static_cast<int>(chosenCat)] = true;

                if (isYahtzee(dice) && hadYahtzeeScored) {
                    players[p].yahtzeeBonusCount++;
                }

                string statusMsg2 = CAT_NAME[static_cast<int>(chosenCat)] + u8"�� " + to_string(score) + u8"���� ����߽��ϴ�.";
                redrawAll(round, (int)p, 0, dice, held, players, u8"Enter�� ���� ���� �÷��̾��...", "", statusMsg2);

                if (is_computer[p]) this_thread::sleep_for(chrono::seconds(2));
                else { string dummy; getline(cin, dummy); }
            }
        }

        clearScreen();
        writeAt(5, 2, u8"========== ���� ��� ==========");
        sort(players.begin(), players.end(), [](const Scorecard& a, const Scorecard& b) { return a.total() > b.total(); });
        int y = 4;
        for (const auto& pl : players) {
            db.recordScore(pl.name, pl.total()); // ���� ���� ���
            writeAt(5, y++, fitName(pl.name, 15) + " : " + to_string(pl.total()) + u8"��");
        }
        writeAt(5, y + 2, u8"'" + players[0].name + u8"' ���� �¸��Դϴ�!");
        writeAt(5, y + 4, u8"�ƹ� Ű�� ���� �޴��� ���ư�����...");
        cin.get();
    }

    hideCursor(false);
}