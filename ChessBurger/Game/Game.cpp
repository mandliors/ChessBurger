#include "Game.h"
#include "Board/AnalysisBoard.h"
#include "Board/GameBoard.h"
#include "Board/SetupBoard.h"
#include "extras/raygui.h"

Game::Game(GameState state)
	: m_AnalysisBoard(nullptr), m_GameBoard(nullptr), m_State(GameState::MAIN_MENU)
{
	//Setup engines
	GameData::Engines.push_back(new Engine("assets/stockfish14.exe", "Stockfish 14"));
	if (GameData::Engines[0]->Init())
		GameData::CurrentEngine = GameData::Engines[0];
	else
		GameData::CurrentEngine = nullptr;

	//Setup color buffer
	GameData::Colors =
	{
		Color{ 245, 245, 245, 255 }, Color{ 250, 250, 250, 255 }, Color{ 80, 80, 80, 255 },    Color{ 230, 230, 230, 255 },
		Color{ 60, 60, 60, 255 },    Color{ 90, 90, 90, 255 },    Color{ 220, 220, 220, 255 }, Color{ 20, 20, 20, 255 }
	};

	//Setup texture buffer
	GameData::Textures.Atlas = LoadTexture("assets/themes/piece_sheet.png");
	SetTextureFilter(GameData::Textures.Atlas, TEXTURE_FILTER_BILINEAR);
	GenTextureMipmaps(&GameData::Textures.Atlas);
	float width = GameData::Textures.Atlas.width / 6.0f;
	float height = GameData::Textures.Atlas.height / 2.0f;
	for (int i = 0; i < 6; i++)
	{
		GameData::Textures.PieceRects[i] = Rectangle{ i * width, 0, width, height };
		GameData::Textures.PieceRects[i + 6] = Rectangle{ i * width, height, width, height };
	}
	Image darkImage = GenImageColor(200, 200, Color{ 181, 136, 99, 255 });
	GameData::Textures.Dark = LoadTextureFromImage(darkImage);
	UnloadImage(darkImage);
	Image lightImage = GenImageColor(200, 200, Color{ 240, 217, 181, 255 });
	GameData::Textures.Light = LoadTextureFromImage(lightImage);
	UnloadImage(lightImage);

	//Calculate avarage colours
	Image darkImg = LoadImageFromTexture(GameData::Textures.Dark);
	Color* darkColors = LoadImageColors(darkImage);
	double pixelCount = darkImg.width * darkImg.height;
	double r = 0, g = 0, b = 0, a = 0;
	for (int i = 0; i < pixelCount; i++)
	{
		r += (double)darkColors[i].r;
		g += (double)darkColors[i].g;
		b += (double)darkColors[i].b;
		a += (double)darkColors[i].a;
	}
	GameData::Colors.DarkAvarage.r = r / pixelCount;
	GameData::Colors.DarkAvarage.g = g / pixelCount;
	GameData::Colors.DarkAvarage.b = b / pixelCount;
	GameData::Colors.DarkAvarage.a = a / pixelCount;
	UnloadImageColors(darkColors);
	UnloadImage(darkImg);
	Image lightImg = LoadImageFromTexture(GameData::Textures.Light);
	Color* lightColors = LoadImageColors(lightImg);
	pixelCount = lightImg.width * lightImg.height;
	r = 0, g = 0, b = 0, a = 0;
	for (int i = 0; i < pixelCount; i++)
	{
		r += (double)lightColors[i].r;
		g += (double)lightColors[i].g;
		b += (double)lightColors[i].b;
		a += (double)lightColors[i].a;
	}
	GameData::Colors.LightAvarage.r = r / pixelCount;
	GameData::Colors.LightAvarage.g = g / pixelCount;
	GameData::Colors.LightAvarage.b = b / pixelCount;
	GameData::Colors.LightAvarage.a = a / pixelCount;
	UnloadImageColors(lightColors);
	UnloadImage(lightImg);

	//Initialite audio
	InitAudioDevice();
	
	//Setup sound buffer
	GameData::Sounds.MoveSound = LoadSound("assets/sounds/move.mp3");
	GameData::Sounds.CaptureSound = LoadSound("assets/sounds/capture.mp3");
	GameData::Sounds.CastleSound = LoadSound("assets/sounds/castle.mp3");
	GameData::Sounds.CheckSound = LoadSound("assets/sounds/check.mp3");
	GameData::Sounds.WinSound = LoadSound("assets/sounds/win.mp3");
	GameData::Sounds.LoseSound = LoadSound("assets/sounds/lose.mp3");
	GameData::Sounds.DrawSound = LoadSound("assets/sounds/draw.mp3");

	//Setup boards
	m_AnalysisBoard = new AnalysisBoard(Rectangle{ 0, 0, 1280, 720 }, this);
	m_AnalysisBoard->Reset();
	m_GameBoard = new GameBoard(Rectangle{ 0, 0, 1280, 720 }, this);
	m_GameBoard->Reset();
	m_SetupBoard = new SetupBoard(Rectangle{ 0, 0, 1280, 720 }, this);

	SetState(state);
}

Game::~Game()
{
	delete m_AnalysisBoard;
	delete m_GameBoard;

	for (int i = 0; i < GameData::Engines.size(); i++)
		delete GameData::Engines[i];

	UnloadTexture(GameData::Textures.Atlas);
	UnloadTexture(GameData::Textures.Dark);
	UnloadTexture(GameData::Textures.Light);

	UnloadSound(GameData::Sounds.MoveSound);
	UnloadSound(GameData::Sounds.CaptureSound);
	UnloadSound(GameData::Sounds.CastleSound);
	UnloadSound(GameData::Sounds.CheckSound);
	UnloadSound(GameData::Sounds.WinSound);
	UnloadSound(GameData::Sounds.LoseSound);
	UnloadSound(GameData::Sounds.DrawSound);

	CloseAudioDevice();
}

void Game::SetState(GameState state)
{
	GameData::CurrentEngine->Stop();
	m_State = state;
	switch (m_State)
	{
	case GameState::MAIN_MENU:
	{
		RestoreWindow();
		SetWindowSize(800, 600);
		ClearWindowState(FLAG_WINDOW_RESIZABLE);
		CenterScreen();
		GameData::CurrentEngine->ResetForWaiting();
		break;
	}
	case GameState::ANALYSIS_BOARD:
	{
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		MaximizeWindow();
		m_AnalysisBoard->Reset();
		m_AnalysisBoard->LoadFEN(STARTPOS_FEN);
		m_AnalysisBoard->UpdateBounds();
		GameData::CurrentEngine->ResetForAnalyzing();
		GameData::CurrentEngine->GoInfinite();
		break;
	}
	case GameState::GAME_BOARD:
	{
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		MaximizeWindow();
		m_GameBoard->Reset();
		m_GameBoard->UpdateBounds();
		GameData::CurrentEngine->ResetForPlaying();
		break;
	}
	case GameState::SETUP_BOARD:
	{
		SetWindowState(FLAG_WINDOW_RESIZABLE);
		MaximizeWindow();
		m_SetupBoard->Reset();
		m_SetupBoard->UpdateBounds();
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
	case GameState::GAME_BOARD:
		_GameBoard();
		break;
	case GameState::SETUP_BOARD:
		_SetupBoard();
		break;
	case GameState::SETTINGS:
		_Settings();
		break;
	}
}

AnalysisBoard* Game::GetAnalysisBoard() const
{
	return m_AnalysisBoard;
}

GameBoard* Game::GetGameBoard() const
{
	return m_GameBoard;
}

SetupBoard* Game::GetSetupBoard() const
{
	return m_SetupBoard;
}

void Game::CenterScreen()
{
	SetWindowPosition((GetMonitorWidth(GetCurrentMonitor()) - GetScreenWidth()) / 2, (GetMonitorHeight(GetCurrentMonitor()) - GetScreenHeight()) / 2);
}

void Game::_MainMenu()
{
	DrawTextEx(GameData::MainFont, "ChessBurger", Vector2{ 200, 60 }, 70, 0, RAYWHITE);
	DrawTextEx(GameData::MainFont, "by Progmaster", Vector2{ 520, 85 }, 40, 0, RAYWHITE);

	Rectangle analysisBoardButton{ 260, 200, 300, 70 };
	Rectangle gameBoardButton{ 270, 280, 280, 70 };
	Rectangle setupBoardButton{ 265, 360, 290, 70 };
	Rectangle settingsButton{ 280, 440, 260, 70 };

	GuiSetStyle(DEFAULT, TEXT_SIZE, 32);
	if (GuiButton(analysisBoardButton, "Analysis Board"))
		SetState(GameState::ANALYSIS_BOARD);
	else if (GuiButton(gameBoardButton, "Game Board"))
		SetState(GameState::GAME_BOARD);
	else if (GuiButton(setupBoardButton, "Setup Board"))
		SetState(GameState::SETUP_BOARD);
	else if (GuiButton(settingsButton, "Settings"))
		SetState(GameState::SETTINGS);
}

void Game::_AnalysisBoard()
{
	m_AnalysisBoard->Update();
	m_AnalysisBoard->Draw();
}

void Game::_GameBoard()
{
	m_GameBoard->Update();
	m_GameBoard->Draw();
}

void Game::_SetupBoard()
{
	m_SetupBoard->Update();
	m_SetupBoard->Draw();
}

void Game::_Settings()
{

}