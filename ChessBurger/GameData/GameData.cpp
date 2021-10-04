#include "GameData.h"
#include "Board/IBoard.h"
#include "raylib.h"

Font GameData::GameFont;
std::vector<Engine*> GameData::Engines;
Engine* GameData::CurrentEngine;
uint32_t GameData::EngineLines = 3;
std::mutex GameData::EngineMutex;

bool GameData::SupportAnimations = true;