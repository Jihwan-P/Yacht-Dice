#ifndef GAMEUI_H
#define GAMEUI_H

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#include "gameLogic.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

using namespace std;

// =================== 상수 ===================
extern const int LEFT_W;
extern const int LEFT_X;
extern const int LEFT_Y;
extern const int RIGHT_X;
extern const int RIGHT_Y;

// =================== 콘솔 및 저수준 렌더링 함수 ===================
#ifdef _WIN32
extern HANDLE HOUT;
void hideCursor(bool hide);
void gotoXY(int x, int y);
void writeAt(int x, int y, const string& s);
void ensureConsoleSize(int cols, int rows);
void clearScreen();
int getConsoleCols();
int getConsoleRows();
#else
void hideCursor(bool hide);
void gotoXY(int x, int y);
void writeAt(int x, int y, const string& s);
void ensureConsoleSize(int, int);
void clearScreen();
int getConsoleCols();
int getConsoleRows();
#endif

// 유틸리티 및 렌더링 함수
int getVisualWidth(const string& s);
string fitName(const string& s, int maxCells = 10);
void drawVerticalSep(int col, int y_top, int y_bottom, const string& glyph = u8"│");
void drawHRule(int x, int y);
void clearRightPaneFrom(int start_y);
vector<string> renderDie(int v, bool held);
int scorePreviewLine(Category i, const Dice& d, const Scorecard& sc);

// 메인 렌더링 함수
void redrawAll(int round, int p_idx, int rollsLeft, const Dice& dice, const array<bool, 5>& held, const vector<Scorecard>& players, const string& promptMsg, const string& errorMsg = "", const string& statusMsg = "");
void redrawDiceOnly(const Dice& dice, const array<bool, 5>& held, int start_y);
void displayImpactEffect(const string& combinationName);
void animateRoll(Dice& dice, const array<bool, 5>& held);

#endif // GAMEUI_H