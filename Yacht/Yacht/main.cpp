#include "gamePlay.h"
#include "gameDB.h"

int main() {
    GameDB db("192.168.0.49", "root", "1111", "scoredb"); // DB ��ü ���� �� ���� �Է�
    if (!db.connect()) {
        std::cerr << "DB ���� ����. ���α׷��� �����մϴ�." << std::endl;
        return 1;
    }

    run_yahtzee_game(db);
    return 0;
}