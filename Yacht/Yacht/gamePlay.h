#pragma once
#ifndef GAMEPLAY_H
#define GAMEPLAY_H

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <thread>
#include <algorithm>

#include "gameLogic.h"
#include "gameUI.h"
#include "gameAI.h"
#include "gameDB.h"

using namespace std;

// ���� ���� ���� �Լ�
void run_yahtzee_game(GameDB& db);

#endif // GAMEPLAY_H