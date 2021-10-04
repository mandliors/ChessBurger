#pragma once

#include "GameData/GameData.h"
#include "Board/IBoard.h"

class AnalysisBoard;
class GameBoard;

enum class GameState
{
	MAIN_MENU = 0, ANALYSIS_BOARD, GAME_ROOM, SETTINGS
};

class Game
{
public:
	Game(GameState state);
	~Game();

	void SetState(GameState state);
	void Update();

	static ColorBuffer DefaultColorBuffer;

public:
	static void CenterScreen();

private:
	void _MainMenu();
	void _AnalysisBoard();
	void _GameRoom();
	void _Settings();

private:
	AnalysisBoard* m_AnalysisBoard;
	GameBoard* m_GameBoard;
	GameState m_State;
};