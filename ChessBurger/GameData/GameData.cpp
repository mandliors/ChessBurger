#include "raylib.h"
#include "GameData.h"
#include "Game/Game.h"
#include "Board/IBoard.h"

Font GameData::MainFont;
ColorBuffer GameData::Colors;
TextureBuffer GameData::Textures;
SoundBuffer GameData::Sounds;
std::vector<Engine*> GameData::Engines;
Engine* GameData::CurrentEngine;
uint32_t GameData::EngineLines = 3;
std::mutex GameData::EngineMutex;
Color GameData::ArrowColor = DARKBLUE;
float GameData::ArrowOpacity = 0.8f;

bool GameData::SupportAnimations = true;