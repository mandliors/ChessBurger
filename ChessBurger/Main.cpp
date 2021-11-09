#include "Game/Game.h"

#define RAYGUI_IMPLEMENTATION
#include "extras/raygui.h"

int main()
{
	//SetConfigFlags(FLAG_MSAA_4X_HINT);
	InitWindow(600, 400, "ChessBurger");
	SetWindowState(FLAG_WINDOW_UNDECORATED);
	Game::CenterScreen();
	SetTargetFPS(120);
	SetExitKey(-1);
	
	//Load GUI
	GameData::GameFont = LoadFontEx("assets/OpenSans.ttf", 70, NULL, -1);
	SetTextureFilter(GameData::GameFont.texture, TEXTURE_FILTER_BILINEAR);
	GuiLoadStyle("assets/style.rgs");

	//Loading screen
	Texture2D loadingScreen = LoadTexture("assets/loading_screen.png");
	BeginDrawing();
	DrawTexture(loadingScreen, 0, 0, WHITE);
	EndDrawing();
	Game game(GameState::MAIN_MENU);

	//Main menu
	SetWindowSize(800, 600);
	Game::CenterScreen();
	ClearWindowState(FLAG_WINDOW_UNDECORATED);

	while (!WindowShouldClose())
	{
		BeginDrawing();
		{
			ClearBackground(Color{ 48, 48, 48 });

			if (IsKeyReleased(KEY_ESCAPE))
			{
				SetMouseCursor(MOUSE_CURSOR_DEFAULT);
				game.SetState(GameState::MAIN_MENU);
			}

			game.Update();
		}
		EndDrawing();
	}

	//Free resources
	UnloadFont(GameData::GameFont);
	UnloadTexture(loadingScreen);

	CloseWindow();
	return 0;
}