#pragma once

#include "GameData/GameData.h"
#include "Board/IBoard.h"

class AnalysisBoard;
class GameBoard;
class SetupBoard;

enum class GameState
{
	MAIN_MENU = 0, ANALYSIS_BOARD, GAME_BOARD, SETUP_BOARD, SETTINGS
};

struct ColorBuffer
{
	Color FgNormal;
	Color FgHovered;
	Color FgFocused;
	Color FgFocused2;
	Color BgNormal;
	Color BgHovered;
	Color BgFocused;
	Color BgFocused2;
	Color DarkAvarage;
	Color LightAvarage;
};

struct TextureBuffer
{
	Texture2D Atlas;
	Rectangle PieceRects[(int)PieceType::COUNT];
	Texture2D Dark;
	Texture2D Light;
};

struct SoundBuffer
{
	Sound MoveSound;
	Sound CaptureSound;
	Sound CastleSound;
	Sound CheckSound;
	Sound WinSound;
	Sound LoseSound;
	Sound DrawSound;
};

class Game
{
public:
	Game(GameState state);
	~Game();

	void SetState(GameState state);
	void Update();

	AnalysisBoard* GetAnalysisBoard() const;
	GameBoard* GetGameBoard() const;
	SetupBoard* GetSetupBoard() const;

	static ColorBuffer DefaultColorBuffer;

public:
	static void CenterScreen();

private:
	void _MainMenu();
	void _AnalysisBoard();
	void _GameBoard();
	void _SetupBoard();
	void _Settings();

private:
	AnalysisBoard* m_AnalysisBoard;
	GameBoard* m_GameBoard;
	SetupBoard* m_SetupBoard;
	GameState m_State;
};