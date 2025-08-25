#include "gameDB.h"

GameDB::GameDB(const string& server, const string& user, const string& password, const string& database)
    : server(server), user(user), password(password), database(database), conn(nullptr) {
}

GameDB::~GameDB() {
    disconnect();
}

bool GameDB::connect() {
    conn = mysql_init(NULL);
    if (!conn) {
        cerr << "MySQL �ʱ�ȭ ����." << endl;
        return false;
    }

    if (!mysql_real_connect(conn, server.c_str(), user.c_str(), password.c_str(), database.c_str(), 0, NULL, 0)) {
        cerr << "DB ���� ����: " << mysql_error(conn) << endl;
        mysql_close(conn);
        conn = nullptr;
        return false;
    }

    // �ѱ� ���� ����
    mysql_set_character_set(conn, "utf8mb4");

    cout << "DB ���� ����." << endl;
    return true;
}

void GameDB::disconnect() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

void GameDB::recordScore(const string& name, int score) {
    if (!conn) {
        cerr << "DB�� ������� �ʾ� ������ ����� �� �����ϴ�." << endl;
        return;
    }

    // SQL Injection ������ ���� PreparedStatement ��� ����
    // ���� �Ҵ�� ������ ũ�⸦ ���
    size_t buffer_size = 2 * name.length() + 1;
    std::vector<char> to_buffer(buffer_size);

    // �̽��������� ���ڿ��� ���� ���̸� ��ȯ
    unsigned long escaped_length = mysql_real_escape_string(conn, to_buffer.data(), name.c_str(), name.length());

    std::string escaped_name(to_buffer.data(), escaped_length);

    ostringstream query;
    query << "INSERT INTO score (name, score) VALUES ('" << escaped_name << "', " << score << ")";

    if (mysql_query(conn, query.str().c_str())) {
        cerr << "���� ��� ����: " << mysql_error(conn) << endl;
    }
}

vector<ScoreEntry> GameDB::getTopScores(int count) {
    vector<ScoreEntry> topScores;
    if (!conn) {
        cerr << "DB�� ������� �ʾ� ���� ����� ������ �� �����ϴ�." << endl;
        return topScores;
    }

    ostringstream query;
    query << "SELECT name, score FROM score ORDER BY score DESC, id ASC LIMIT " << count;

    if (mysql_query(conn, query.str().c_str())) {
        cerr << "���� ��ȸ ����: " << mysql_error(conn) << endl;
        return topScores;
    }

    MYSQL_RES* res = mysql_store_result(conn);
    if (res == NULL) {
        cerr << "��� ���� ����: " << mysql_error(conn) << endl;
        return topScores;
    }

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        ScoreEntry entry;
        entry.name = row[0] ? row[0] : "";
        entry.score = row[1] ? stoi(row[1]) : 0;
        topScores.push_back(entry);
    }

    mysql_free_result(res);
    return topScores;
}