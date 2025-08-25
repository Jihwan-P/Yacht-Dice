#pragma once
#ifndef GAMEDB_H
#define GAMEDB_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <mysql.h>

using namespace std;

// 데이터베이스에 저장할 점수 항목 구조체
struct ScoreEntry {
    string name;
    int score;
};

class GameDB {
private:
    MYSQL* conn;
    string server;
    string user;
    string password;
    string database;

public:
    GameDB(const string& server, const string& user, const string& password, const string& database);
    ~GameDB();

    bool connect();
    void disconnect();
    void recordScore(const string& name, int score);
    vector<ScoreEntry> getTopScores(int count);
};

#endif // GAMEDB_H