#include "Game.h"
#include "Board/AnalysisBoard.h"
#include "Board/GameBoard.h"
#include "extras/raygui.h"

ColorBuffer Game::DefaultColorBuffer =
{
	Color{ 74, 74, 74, 255 },    Color{ 220, 220, 220, 255 },
	Color{ 245, 245, 245, 255 }, Color{ 250, 250, 250, 255 }, Color{ 80, 80, 80, 255 },    Color{ 230, 230, 230, 255 },
	Color{ 60, 60, 60, 255 },    Color{ 90, 90, 90, 255 },    Color{ 220, 220, 220, 255 }, Color{ 20, 20, 20, 255 },
};

Game::Game(GameState state)
	: m_AnalysisBoard(nullptr), m_GameBoard(nullptr), m_State(GameState::MAIN_MENU)
{
	//Setup engines
	GameData::Engines.push_back(new Engine("assets/stockfish14.exe", "Stockfish 14"));
	if (GameData::Engines[0]->Init())
		GameData::CurrentEngine = GameData::Engines[0];
	else
		GameData::CurrentEngine = nullptr;
	
	//Setup boards
	m_AnalysisBoard = new AnalysisBoard(DefaultColorBuffer, Rectangle{ 0, 0, 1280, 720 });
	m_AnalysisBoard->Reset();
	m_GameBoard = new GameBoard(DefaultColorBuffer, Rectangle{ 0, 0, 1280, 720 });
	m_GameBoard->Reset();

	SetState(state);
}

Game::~Game()
{
	delete m_AnalysisBoard;
	delete m_GameBoard;

	for (int i = 0; i < GameData::Engines.size(); i++)
		delete GameData::Engines[i];
}

void Game::SetState(GameState state)
{
	m_State = state;
	switch (m_State)
	{
	case GameState::MAIN_MENU:
	{
		RestoreWindow();
		SetWindowSize(800, 600);
		ClearWindowState(FLAG_WINDOW_RESIZABLE);
		CenterScreen();
		break;
	}
	case GameState::ANALYSIS_BOARD:
	{
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		MaximizeWindow();
		m_AnalysisBoard->Reset();
		m_AnalysisBoard->UpdateBounds();
		break;
	}
	case GameState::GAME_ROOM:
	{
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		MaximizeWindow();
		m_GameBoard->Reset();
		m_GameBoard->UpdateBounds();
		break;
	}
	case GameState::SETTINGS:
	{
		RestoreWindow();
		SetWindowSize(800, 600);
		ClearWindowState(FLAG_WINDOW_RESIZABLE);
		CenterScreen();
		break;
	}
	}
}

void Game::Update()
{
	switch (m_State)
	{
	case GameState::MAIN_MENU:
		_MainMenu();
		break;
	case GameState::ANALYSIS_BOARD:
		_AnalysisBoard();
		break;
	case GameState::GAME_ROOM:
		_GameRoom();
		break;
	case GameState::SETTINGS:
		_Settings();
		break;
	}
}

void Game::CenterScreen()
{
	SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) - GetScreenWidth()) / 2, (GetMonitorHeight(GetCurrentMonitor()) - GetScreenHeight()) / 2);
}

void Game::_MainMenu()
{
	DrawTextEx(GameData::GameFont, "ChessBurger", Vector2{ 200, 60 }, 70, 0, RAYWHITE);
	DrawTextEx(GameData::GameFont, "by Progmaster", Vector2{ 520, 85 }, 40, 0, RAYWHITE);

	Rectangle mainMenuButton{ 260, 200, 280, 80 };
	Rectangle gameRoomButton{ 270, 300, 260, 80 };
	Rectangle settingsButton{ 280, 400, 240, 80 };

	if (GuiButton(mainMenuButton, "Analysis Board"))
		SetState(GameState::ANALYSIS_BOARD);
	else if (GuiButton(gameRoomButton, "Game Room"))
		SetState(GameState::GAME_ROOM);
	else if (GuiButton(settingsButton, "Settings"))
		SetState(GameState::SETTINGS);
}

void Game::_AnalysisBoard()
{
	m_AnalysisBoard->Update();
	m_AnalysisBoard->Draw();
}

void Game::_GameRoom()
{
	m_GameBoard->Update();
	m_GameBoard->Draw();
}

void Game::_Settings()
{

}