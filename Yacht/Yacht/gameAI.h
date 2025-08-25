#pragma once
#ifndef GAMEAI_H
#define GAMEAI_H

#include <vector>
#include <array>
#include <utility>

#include "gameLogic.h"

using namespace std;

// AI 난이도 열거형
enum class AIDifficulty { EASY, NORMAL, HARD };

// AI 전략 함수
Category chooseBestScoringCategory_Easy(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used);
pair<Category, int> findBestExpectedCategory_Normal(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft);
array<bool, 5> chooseBestHoldStrategy_Hard(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int rollsLeft, int round);
Category chooseBestScoringCategory(const Dice& d, const array<bool, static_cast<size_t>(Category::CATEGORY_COUNT)>& used, int round, AIDifficulty difficulty);

#endif // GAMEAI_H