#include "gamePlay.h"
#include "gameDB.h"

int main() {
    GameDB db("192.168.0.49", "root", "1111", "scoredb"); // DB 객체 생성 시 정보 입력
    if (!db.connect()) {
        std::cerr << "DB 연결 실패. 프로그램을 종료합니다." << std::endl;
        return 1;
    }

    run_yahtzee_game(db);
    return 0;
}