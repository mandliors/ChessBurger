#pragma once

#include "raylib.h"
#include <string>

static float MoveEvals[4] { -5.09f, -1.80f, -0.68f, -0.30f };
static Color MoveColors[10]
{
	Color{ 255, 4, 0, 255 },
	Color{ 237, 123, 0, 255 },
	Color{ 255, 200, 0, 255 },
	Color{ 120, 120, 120, 255 },
	Color{ 139, 209, 125, 255 },
	Color{ 37, 224, 0, 255 },
	Color{ 108, 0, 232, 255 },
	Color{ 34, 92, 240, 255 },
	Color{ 232, 111, 224, 255 },
	Color{ 25, 25, 25, 255 } //232, 192, 111, 255
};

enum class MoveType
{
	BLUNDER = 0, MISTAKE, INACCURACY, GOOD, GREAT, BEST, GENIUS, ONLY, FORCED, BOOK
};

struct AnalyzedMove
{
	MoveType Type = MoveType::BLUNDER;
	std::string Move = "";
	std::string Eval = "";
};