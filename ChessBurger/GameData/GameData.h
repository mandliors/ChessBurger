#pragma once

#include "Engine/Engine.h"
#include <vector>
#include <mutex>

struct Font;

struct GameData
{
	static Font GameFont;
	static std::vector<Engine*> Engines;
	static Engine* CurrentEngine;
	static uint32_t EngineLines;
	static std::mutex EngineMutex;

	static bool SupportAnimations;
};