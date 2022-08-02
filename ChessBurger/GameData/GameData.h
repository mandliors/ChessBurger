#pragma once

#include "Engine/Engine.h"
#include <vector>
#include <mutex>

struct Font;
struct Color;

struct ColorBuffer;
struct TextureBuffer;
struct SoundBuffer;

struct GameData
{
	static Font MainFont;
	static ColorBuffer Colors;
	static TextureBuffer Textures;
	static SoundBuffer Sounds;
	static std::vector<Engine*> Engines;
	static Engine* CurrentEngine;
	static uint32_t EngineLines;
	static std::mutex EngineMutex;
	static Color ArrowColor;
	static float ArrowOpacity;

	static uint32_t AnalysisDepth;

	static bool SupportAnimations;
	static bool MuteSounds;
};