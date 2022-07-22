#include "SetupBoard.h"
#include "raymath.h"
#include "extras/raygui.h"
#include "AnalysisBoard.h"
#include "Game/Game.h"
#include "Utilities/Utilities.h"

SetupBoard::SetupBoard(const Rectangle& bounds, Game* owner)
	: IBoard(bounds, owner), m_SelectedPieceType(PieceType::NONE), m_SetupWhitePiecesY(0), m_SetupBlackPiecesY(0), m_SetupPiecesWidth(0), m_SetupPiecesHeight(0), m_SidePanelBounds({}), m_Delete(false), m_DragSetupPiece(false), m_UIWidth(0), m_SideToPlay(0), m_SideToPlayEditMode(false)
{
	m_ShowNametag = false;
	m_OnlyLegalMoves = false;
}

SetupBoard::~SetupBoard()
{
	IBoard::~IBoard();
}

void SetupBoard::Update()
{
	m_PointingHand = false;

	//Set cursor and set drag
	Vector2 mousePosition = GetMousePosition();
	if (CheckCollisionPointRec(mousePosition, m_BoardBounds))
	{
		Vector2 mouseSquare = _GetSquare(mousePosition);
		Piece& piece = m_Board[(int)mouseSquare.y][(int)mouseSquare.x];
		Vector2 square = _GetSquare(piece.GetPosition());

		if (piece.GetType() != PieceType::NONE && m_SelectedPieceType == PieceType::NONE && !m_Delete)
		{
			//Piece drag
			if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			{
				m_SelectedPiece = &piece;
				m_SelectionFrom = Highlight(m_SelectedPiece->GetPosition(), _GetSquare(m_SelectedPiece->GetPosition()), Fade(ORANGE, 0.4f), m_Flipped);
				m_DraggedPiece = m_SelectedPiece;
				m_DraggedPiece->SetDrag(true);
			}
			m_PointingHand = true;
		}
	}

	//Release piece on right click
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && m_DraggedPiece)
	{
		m_DraggedPiece->SetDrag(false);
		m_DraggedPiece = nullptr;
		m_SelectedPiece = nullptr;
		m_SelectedMoves.clear();
	}
	
	//Set mousedown position
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		m_MouseDownPosition = GetMousePosition();

	//Clear drag move, highlights, arrows and reset selected piece
	if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	{
		if (m_DraggedPiece)
		{
			if (CheckCollisionPointRec(mousePosition, m_BoardBounds))
				_Move(_GetRealSquare(m_DraggedPiece->GetPreviousPosition()), _GetSquare(mousePosition), false, false);
			else
			{
				Vector2 square = _GetRealSquare(m_DraggedPiece->GetPreviousPosition());
				m_Board[(int)square.y][(int)square.x].SetType(PieceType::NONE);
			}
		}
		
		m_Highlights.clear();
		m_Arrows.clear();
		m_SelectedPiece = nullptr;
		m_SelectedMoves.clear();
		if (m_DraggedPiece)
		{
			m_DraggedPiece->SetDrag(false);
			m_DraggedPiece = nullptr;
		}
	}

	//Check copy events
	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_C))
	{
		//Copy PGN
		if (IsKeyDown(KEY_LEFT_SHIFT))
			SetClipboardText(_GetPGN().c_str());
		//Copy FEN
		else
			SetClipboardText(_GetFEN().c_str());
	}

	//Update pieces
	for (int i = 0; i < 8; i++)
		for (int j = 0; j < 8; j++)
			m_Board[i][j].Update();

	//Check highlight and arrow
	if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
	{
		if (CheckCollisionPointRec(GetMousePosition(), m_BoardBounds))
			m_RightDragStart = GetMousePosition();
		else
			m_RightDragStart.x = -1;
	}
	else if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && !m_DraggedPiece && !IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		if (m_RightDragStart.x != -1)
		{
			Vector2 from = _GetSquare(m_RightDragStart);
			Vector2 to = _GetSquare(GetMousePosition());

			//Highlight
			if (from.x == to.x && from.y == to.y)
			{
				Highlight highlight(Vector2{ from.x * m_SquareSize + m_BoardBounds.x, from.y * m_SquareSize + m_BoardBounds.y }, from, Fade(RED, 0.8f), m_Flipped);
				int index = -1;
				for (int i = 0; i < m_Highlights.size(); i++)
				{
					if (m_Highlights[i] == highlight)
					{
						index = i;
						break;
					}
				}
				if (index != -1)
					m_Highlights.erase(m_Highlights.begin() + index);
				else
					m_Highlights.push_back(highlight);
			}
			//Arrow
			else
			{
				Arrow arrow(Vector2{ (from.x + 0.5f) * m_SquareSize + m_BoardBounds.x, (from.y + 0.5f) * m_SquareSize + m_BoardBounds.y },
					Vector2{ (to.x + 0.5f) * m_SquareSize + m_BoardBounds.x, (to.y + 0.5f) * m_SquareSize + m_BoardBounds.y }, from, to, Fade(DARKBLUE, GameData::ArrowOpacity), m_Flipped);
				int index = -1;
				for (int i = 0; i < m_Arrows.size(); i++)
				{
					if (m_Arrows[i] == arrow)
					{
						index = i;
						break;
					}
				}
				if (index != -1)
					m_Arrows.erase(m_Arrows.begin() + index);
				else
					m_Arrows.push_back(arrow);
			}
		}
	}

	//Flip board
	if (IsKeyReleased(KEY_F))
		Flip();

	//Check resize
	if (IsWindowResized())
		UpdateBounds();

	//Set cursor
	m_PointingHand = m_PointingHand || SetupPieces_CheckCursor() || SidePanel_CheckCursor();

	//Paste event
	if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
	{
		std::string text = GetClipboardText();

		//Paste FEN
		if (!LoadFEN(text))
			//Paste PGN
			if (!LoadPGN(text))
				std::invalid_argument("Incorrect FEN or PGN");
	}

	//Update UI
	SetupPieces_Update();
	SidePanel_UpdateAndDraw();
}

void SetupBoard::UpdateBounds()
{
	float boardSize = (GetScreenHeight() - 2 * DISTANCE_BL) * 0.8f;
	float totalWidth = boardSize + DISTANCE_BS + boardSize * SIDEPANEL_WIDTH_P;
	float actualBoardSize = (int)(boardSize / 8.0f) * 8;
	float setupHeight = boardSize / 8.0f;
	//Match height
	if (totalWidth < GetScreenWidth())
	{
		float offset = (GetScreenWidth() - totalWidth) * 0.5f;
		m_BoardBounds = Rectangle{ offset, setupHeight + DISTANCE_BL, boardSize, boardSize };
		m_SidePanelBounds = Rectangle{ m_BoardBounds.x + boardSize + DISTANCE_BS, m_BoardBounds.y, std::min(boardSize * SIDEPANEL_WIDTH_P, SIDEPANEL_MAX_WIDTH_A), boardSize };
		m_SetupWhitePiecesY = m_BoardBounds.y + boardSize + DISTANCE_BL;
		m_SetupBlackPiecesY = 0;
		m_SetupPiecesWidth = actualBoardSize;
		m_SetupPiecesHeight = setupHeight;
		m_UIWidth = m_SidePanelBounds.width - 2 * UI_OFFSET;
	}
	//Match width
	else
	{
		boardSize = (GetScreenWidth() - DISTANCE_BS) / (1.0f + SIDEPANEL_WIDTH_P);
		actualBoardSize = (int)(boardSize / 8.0f) * 8;
		setupHeight = boardSize / 8.0f;
		float offset = (GetScreenHeight() - boardSize - 2 * DISTANCE_BL - 2 * setupHeight) * 0.5f;
		m_BoardBounds = Rectangle{ 0, offset + setupHeight + DISTANCE_BL, boardSize, boardSize };
		m_SidePanelBounds = Rectangle{ boardSize + DISTANCE_BS, m_BoardBounds.y, std::min(boardSize * SIDEPANEL_WIDTH_P, SIDEPANEL_MAX_WIDTH_A), boardSize };
		m_SetupWhitePiecesY = m_BoardBounds.y + boardSize + DISTANCE_BL;
		m_SetupBlackPiecesY = offset;
		m_SetupPiecesWidth = actualBoardSize;
		m_SetupPiecesHeight = setupHeight;
		m_UIWidth = m_SidePanelBounds.width - 2 * UI_OFFSET;
	}
	m_SquareSize = boardSize / 8;


	//Update guides
	IGuide::Size = m_SquareSize;
	for (int i = 0; i < m_Highlights.size(); i++)
		m_Highlights[i].UpdatePosition(m_BoardBounds, m_SquareSize);
	for (int i = 0; i < m_Arrows.size(); i++)
		m_Arrows[i].UpdatePosition(m_BoardBounds, m_SquareSize);
	for (int i = 0; i < m_SelectedMoves.size(); i++)
		m_SelectedMoves[i].UpdatePosition(m_BoardBounds, m_SquareSize);

	//Update special highlights
	m_SelectionFrom.UpdatePosition(m_BoardBounds, m_SquareSize);
	m_LastMoveFrom.UpdatePosition(m_BoardBounds, m_SquareSize);
	m_LastMoveTo.UpdatePosition(m_BoardBounds, m_SquareSize);

	//Update pieces
	Piece::Size = PIECE_SIZE_P * (float)m_SquareSize;
	int offset = (1.0f - PIECE_SIZE_P) * (float)m_SquareSize * 0.5f;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			m_Board[i][j].TellBoardBounds(m_BoardBounds);
			m_Board[i][j].SetPosition(Vector2{ m_BoardBounds.x + j * m_SquareSize + offset, m_BoardBounds.y + i * m_SquareSize + offset });
		}
	}
}

void SetupBoard::Draw() const
{
	//Draw board
	float scale = (float)m_SquareSize / (float)GameData::Textures.Light.height;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			//Light square
			if ((i + j) % 2 == 0)
				DrawTextureEx(GameData::Textures.Light, Vector2{ m_BoardBounds.x + j * m_SquareSize, m_BoardBounds.y + i * m_SquareSize }, 0.0f, scale, WHITE);
			//Dark square
			else
				DrawTextureEx(GameData::Textures.Dark, Vector2{ m_BoardBounds.x + j * m_SquareSize, m_BoardBounds.y + i * m_SquareSize }, 0.0f, scale, WHITE);
		}
	}

	//Draw line numbers and letters
	float offset = 5.0f;
	float fontSize = m_SquareSize * 0.3f;
	for (int i = 0; i < 8; i++)
	{
		if (i % 2 == 0)
			DrawTextEx(GameData::MainFont, std::to_string(8 - i).c_str(), Vector2{ m_BoardBounds.x + offset, m_BoardBounds.y + i * m_SquareSize + offset }, fontSize, 0.0f, GameData::Colors.DarkAvarage);
		else
			DrawTextEx(GameData::MainFont, std::to_string(8 - i).c_str(), Vector2{ m_BoardBounds.x + offset, m_BoardBounds.y + i * m_SquareSize + offset }, fontSize, 0.0f, GameData::Colors.LightAvarage);
	}
	for (int i = 0; i < 8; i++)
	{
		std::string letter = std::string(1, 'a' + (char)i);
		float width = MeasureTextEx(GameData::MainFont, letter.c_str(), fontSize, 0.0f).x;
		if (i % 2 == 0)
			DrawTextEx(GameData::MainFont, letter.c_str(), Vector2{ m_BoardBounds.x + (i + 1) * m_SquareSize - offset - width, m_BoardBounds.y + m_BoardBounds.height - offset - fontSize }, fontSize, 0.0f, GameData::Colors.LightAvarage);
		else
			DrawTextEx(GameData::MainFont, letter.c_str(), Vector2{ m_BoardBounds.x + (i + 1) * m_SquareSize - offset - width, m_BoardBounds.y + m_BoardBounds.height - offset - fontSize }, fontSize, 0.0f, GameData::Colors.DarkAvarage);
	}

	//Draw highlights
	for (int i = 0; i < m_Highlights.size(); i++)
		m_Highlights[i].Draw(m_BoardBounds);

	//Highlight selected piece
	if (m_SelectedPiece) m_SelectionFrom.Draw(m_BoardBounds);

	//Draw pieces
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			const Piece& piece = m_Board[i][j];
			if (piece.GetType() != PieceType::NONE && !piece.GetDrag())
				m_Flipped ? piece.DrawFlipped() : piece.Draw();
		}
	}

	//Draw dragged move
	if (m_DraggedPiece)
		m_DraggedPiece->Draw();

	//Draw arrows
	for (int i = 0; i < m_Arrows.size(); i++)
		m_Arrows[i].Draw(m_BoardBounds);

	//Set cursor
	if (m_PointingHand)
		SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
	else
		SetMouseCursor(MOUSE_CURSOR_DEFAULT);

	SetupPieces_Draw();
}

void SetupBoard::Reset(bool resetEnginePosition)
{
	IBoard::Reset(resetEnginePosition);
	m_SelectedPieceType = PieceType::NONE;
}

void SetupBoard::SetupPieces_Update()
{
	//Clicked on a setup piece
	if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
	{
		Vector2 mousePos = GetMousePosition();
		float width = m_SetupPiecesWidth / 8.0f;
		//Put down a piece
		if (CheckCollisionPointRec(GetMousePosition(), m_BoardBounds) && m_SelectedPieceType != PieceType::NONE)
		{
			Vector2 square = _GetSquare(GetMousePosition());
			m_Board[(int)square.y][(int)square.x].SetType(m_SelectedPieceType);
		}
		//Clicked on white setup area
		else if (CheckCollisionPointRec(mousePos, Rectangle{ m_BoardBounds.x, m_SetupWhitePiecesY, m_SetupPiecesWidth, m_SetupPiecesHeight }))
		{
			m_Delete = false;
			//Pointer is selected
			if (mousePos.x < m_BoardBounds.x + width)
				m_SelectedPieceType = PieceType::NONE;
			//Delete
			else if (mousePos.x > m_BoardBounds.x + m_SetupPiecesWidth - width)
			{
				m_Delete = true;
				m_SelectedPieceType = PieceType::NONE;
			}
			else
			{
				if (!m_Flipped)
					m_SelectedPieceType = (PieceType)(std::floor((mousePos.x - m_BoardBounds.x) / width) - 1);
				else
					m_SelectedPieceType = (PieceType)(std::floor((mousePos.x - m_BoardBounds.x) / width) + 5);
				m_DragSetupPiece = true;
			}
		}
		//Clicked on black setup area
		else if (CheckCollisionPointRec(mousePos, Rectangle{ m_BoardBounds.x, m_SetupBlackPiecesY, m_SetupPiecesWidth, m_SetupPiecesHeight }))
		{
			m_Delete = false;
			//Pointer is selected
			if (mousePos.x < m_BoardBounds.x + width)
				m_SelectedPieceType = PieceType::NONE;
			//Delete
			else if (mousePos.x > m_BoardBounds.x + m_SetupPiecesWidth - width)
			{
				m_Delete = true;
				m_SelectedPieceType = PieceType::NONE;
			}
			else
			{
				if (!m_Flipped)
					m_SelectedPieceType = (PieceType)(std::floor((mousePos.x - m_BoardBounds.x) / width) + 5);
				else
					m_SelectedPieceType = (PieceType)(std::floor((mousePos.x - m_BoardBounds.x) / width) - 1);
				m_DragSetupPiece = true;
			}
		}
	}
	//"Draws" with a piece
	else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
	{
		if (CheckCollisionPointRec(GetMousePosition(), m_BoardBounds))
		{
			//Deleted a piece
			if (m_Delete)
			{
				Vector2 square = _GetSquare(GetMousePosition());
				m_Board[(int)square.y][(int)square.x].SetType(PieceType::NONE);
			}
			else if (m_SelectedPieceType != PieceType::NONE && !m_DragSetupPiece)
			{
				Vector2 square = _GetSquare(GetMousePosition());
				m_Board[(int)square.y][(int)square.x].SetType(m_SelectedPieceType);
			}
		}
	}
	//Released a dragged piece
	else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	{
		if (CheckCollisionPointRec(GetMousePosition(), m_BoardBounds) && m_SelectedPieceType != PieceType::NONE)
		{
			Vector2 square = _GetSquare(GetMousePosition());
			m_Board[(int)square.y][(int)square.x].SetType(m_SelectedPieceType);
			if (m_DragSetupPiece)
				m_SelectedPieceType = PieceType::NONE;
		}
		m_DragSetupPiece = false;
	}
}

void SetupBoard::SetupPieces_Draw() const
{
	//Draw setup background
	DrawRectangleRec(Rectangle{ m_BoardBounds.x, m_SetupWhitePiecesY, m_SetupPiecesWidth, m_SetupPiecesHeight }, Fade(GRAY, 0.8f));
	DrawRectangleRec(Rectangle{ m_BoardBounds.x, m_SetupBlackPiecesY, m_SetupPiecesWidth, m_SetupPiecesHeight }, Fade(GRAY, 0.8f));
	
	float width = m_SetupPiecesWidth / 8.0f;
	//Draw delete background
	if (m_Delete)
	{
		DrawRectangle(m_BoardBounds.x + m_SetupPiecesWidth - width, m_SetupBlackPiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
		DrawRectangle(m_BoardBounds.x + m_SetupPiecesWidth - width, m_SetupWhitePiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
	}
	//Draw selected piece background
	else if ((int)m_SelectedPieceType > -1)
	{
		//White piece
		if ((int)m_SelectedPieceType < 6)
		{
			if (!m_Flipped)
				DrawRectangle(m_BoardBounds.x + ((int)m_SelectedPieceType + 1) * width, m_SetupWhitePiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
			else
				DrawRectangle(m_BoardBounds.x + ((int)m_SelectedPieceType + 1) * width, m_SetupBlackPiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
		}
		//Black piece
		else
		{
			if (!m_Flipped)
				DrawRectangle(m_BoardBounds.x + ((int)m_SelectedPieceType - 5) * width, m_SetupBlackPiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
			else
				DrawRectangle(m_BoardBounds.x + ((int)m_SelectedPieceType - 5) * width, m_SetupWhitePiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
		}
	}
	//Draw pointer background
	else
	{
		DrawRectangle(m_BoardBounds.x, m_SetupBlackPiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
		DrawRectangle(m_BoardBounds.x, m_SetupWhitePiecesY, width, m_SetupPiecesHeight, Fade(PURPLE, 0.4f));
	}

	//Draw setup pieces
	for (int i = 0; i < 6; i++)
	{
		DrawTexturePro(GameData::Textures.Atlas, GameData::Textures.PieceRects[i], Rectangle{ m_BoardBounds.x + (i + 1) * width, !m_Flipped ? m_SetupWhitePiecesY : m_SetupBlackPiecesY, width, width }, { 0, 0 }, 0.0f, WHITE);
		DrawTexturePro(GameData::Textures.Atlas, GameData::Textures.PieceRects[i + 6], Rectangle{ m_BoardBounds.x + (i + 1) * width, !m_Flipped ? m_SetupBlackPiecesY : m_SetupWhitePiecesY, width, width }, { 0, 0 }, 0.0f, WHITE);
	}

	//Draw pointer and dustbin
	DrawCircle(m_BoardBounds.x + width * 0.5f, m_SetupWhitePiecesY + m_SetupPiecesHeight * 0.5f, width * 0.3f, GREEN);
	DrawCircle(m_BoardBounds.x + width * 0.5f, m_SetupBlackPiecesY + m_SetupPiecesHeight * 0.5f, width * 0.3f, GREEN);
	DrawCircle(m_BoardBounds.x + m_BoardBounds.width - width * 0.5f, m_SetupWhitePiecesY + m_SetupPiecesHeight * 0.5f, width * 0.3f, ORANGE);
	DrawCircle(m_BoardBounds.x + m_BoardBounds.width - width * 0.5f, m_SetupBlackPiecesY + m_SetupPiecesHeight * 0.5f, width * 0.3f, ORANGE);

	//Draw dragged piece
	if (m_DragSetupPiece && m_SelectedPieceType != PieceType::NONE)
		DrawTexturePro(GameData::Textures.Atlas, GameData::Textures.PieceRects[(int)m_SelectedPieceType], Rectangle{ GetMouseX() - Piece::Size * 0.5f, GetMouseY() - Piece::Size * 0.5f, (float)Piece::Size, (float)Piece::Size}, {0, 0}, 0.0f, WHITE);
}

bool SetupBoard::SetupPieces_CheckCursor() const
{
	Vector2 mousePos = GetMousePosition();
	return CheckCollisionPointRec(mousePos, Rectangle{ m_BoardBounds.x, m_SetupWhitePiecesY, m_SetupPiecesWidth, m_SetupPiecesHeight }) || CheckCollisionPointRec(mousePos, Rectangle{ m_BoardBounds.x, m_SetupBlackPiecesY, m_SetupPiecesWidth, m_SetupPiecesHeight });
}

void SetupBoard::SidePanel_UpdateAndDraw()
{
	DrawRectangleRec(m_SidePanelBounds, Fade(GRAY, 0.8f));
	float x = m_SidePanelBounds.x + UI_OFFSET;
	float y = m_SidePanelBounds.y + 10;

	//Board
	GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	GuiLine(Rectangle{ x, y, m_UIWidth, UI_OFFSET * 0.5f }, "Board");
	y += UI_OFFSET * 2;
	GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
	if (GuiButton(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Reset"))
	{
		m_StartingFEN = STARTPOS_FEN;
		Reset();
	}
	y += UI_HEIGHT_A + UI_OFFSET;
	if (GuiButton(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Clear"))
	{
		for (int m = 0; m < 8; m++)
			for (int n = 0; n < 8; n++)
				m_Board[m][n].SetType(PieceType::NONE);
	}
	y += UI_HEIGHT_A + UI_OFFSET;
	if (GuiButton(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Flip"))
		Flip();
	y += UI_HEIGHT_A + UI_OFFSET;
	if (GuiButton(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Copy FEN"))
		SetClipboardText(_GetFEN().c_str());
	y += UI_HEIGHT_A * 2 + UI_OFFSET;
	
	//Castling
	GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	GuiLine(Rectangle{ x, y, m_UIWidth, UI_OFFSET * 0.5f }, "Castling");
	y += UI_OFFSET;	
	GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
	GuiLabel(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "White");
	m_WhiteShort = GuiCheckBox(Rectangle{ x + 80.0f, y + 12, 20, 20 }, "O-O", m_WhiteShort);
	m_WhiteLong = GuiCheckBox(Rectangle{ x + 160.0f, y + 12, 20, 20 }, "O-O-O", m_WhiteLong);
	y += 24 + UI_OFFSET;
	GuiLabel(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Black");
	m_BlackShort = GuiCheckBox(Rectangle{ x + 80.0f, y + 12, 20, 20 }, "O-O", m_BlackShort);
	m_BlackLong = GuiCheckBox(Rectangle{ x + 160.0f, y + 12, 20, 20 }, "O-O-O", m_BlackLong);
	y += 36 + UI_OFFSET;

	//Side to play
	GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
	if (GuiDropdownBox(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "White to play;Black to play", &m_SideToPlay, m_SideToPlayEditMode))
		m_SideToPlayEditMode = !m_SideToPlayEditMode;
	y = std::min(y + UI_HEIGHT_A * 4 + UI_OFFSET, m_SidePanelBounds.y + m_SidePanelBounds.height - 2 * UI_HEIGHT_A - 4.5f * UI_OFFSET);

	//Modes
	GuiSetStyle(DEFAULT, TEXT_SIZE, 20);
	GuiLine(Rectangle{ x, y, m_UIWidth, UI_OFFSET * 0.5f }, "Modes");
	y += UI_OFFSET * 2;
	GuiSetStyle(DEFAULT, TEXT_SIZE, 24);
	if (GuiButton(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Analysis Board"))
	{
		m_Owner->SetState(GameState::ANALYSIS_BOARD);
		m_Owner->GetAnalysisBoard()->LoadFEN(_GetFEN());
	}
	y += UI_HEIGHT_A + UI_OFFSET;
	if (GuiButton(Rectangle{ x, y, m_UIWidth, UI_HEIGHT_A }, "Play with computer"))
	{

	}
	y += UI_HEIGHT_A + UI_OFFSET;
}

bool SetupBoard::SidePanel_CheckCursor() const
{
	return false;
}

std::string SetupBoard::_GetFEN() const
{
	std::string fen = ""; int count = 0;
	for (int i = 0; i < 8; i++)
	{
		count = 0;
		for (int j = 0; j < 8; j++)
		{
			if (m_Board[i][j].GetType() == PieceType::NONE)
			{
				count++;
				if (j == 7)
					fen += std::to_string(count);
			}
			else
			{
				if (count != 0)
				{
					fen += std::to_string(count);
					count = 0;
				}
				fen += std::string(1, FEN_Codes[m_Board[i][j].GetType()]);
			}
		}
		fen += std::string(1, '/');
	}
	fen.back() = ' ';

	//side to move
	fen += m_SideToPlay ? "b " : "w ";

	//castling availability
	if (m_WhiteShort) fen += "K";
	if (m_WhiteLong) fen += "Q";
	if (m_BlackShort) fen += "k";
	if (m_BlackLong) fen += "q";
	if (fen.back() == ' ') fen += "-";
	fen += " ";

	//en passant target square
	fen += "- ";

	//halfmove clock
	fen += "0 ";

	//fullmove number
	fen += "1";

	return fen;
}

std::string SetupBoard::_GetPGN() const
{
	return "*";
}