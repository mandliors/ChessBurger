#include "IBoard.h"
#include "GameData/GameData.h"
#include "Game/Game.h"
#include "Utilities/Utilities.h"
#include "extras/raygui.h"
#include <cmath>
#include <algorithm>

#include <iostream>

std::unordered_map<PieceType, char> IBoard::FEN_Codes;
std::unordered_map<char, PieceType> IBoard::FEN_Codes_Backwards;

IBoard::IBoard(const Rectangle& bounds, Game* owner)
	: m_Moves({}), m_MovesSN({}), m_LegalMoves({}), m_HalfMoveClock(0), m_AnalyseMode(false), m_WhiteName("Player 1"), m_Side(0), m_BlackName("Player 2"), m_EnPassant(""), m_StartingFEN_Movecount(-1), m_MoveIndex(-1), m_SelectedMoves({}), m_StartingFEN(STARTPOS_FEN), m_Arrows({}), m_Highlights({}), m_SelectionFrom({}), m_LastMoveFrom({}), m_LastMoveTo({}), m_LeftDragStart({}), m_RightDragStart(Vector2{-1, -1}), m_MouseDownPosition(Vector2{-1, -1}), m_Result(Result::NONE), m_BoardBounds({}), m_SquareSize(0), m_DraggedPiece(nullptr), m_SelectedPiece(nullptr), m_WhiteKing(nullptr), m_BlackKing(nullptr), m_Flipped(false), m_PointingHand(false), m_SideToMove(1), m_WhiteInCheck(false), m_BlackInCheck(false), m_WhiteShort(true), m_WhiteLong(true), m_BlackShort(true), m_BlackLong(true), m_ShowNametag(false), m_ShowLegalMoves(true), m_OnlyLegalMoves(true), m_Owner(owner)
{
	//Create arrow head texture
	int size = 500;
	float hScale = 0.866f;
	Image headImg = GenImageColor(size, size, Color{ 0, 0, 0, 0 });
	for (float i = 0; i < size * 0.5f; i++)
	{
		for (float j = 0; j < (i + 1) * hScale; j++)
		{
			ImageDrawPixel(&headImg, 2 * j, i, WHITE);
			ImageDrawPixel(&headImg, 2 * j + 1, i, WHITE);
		}
	}
	for (float i = 0; i < size * 0.5f; i++)
	{
		for (float j = 0; j < (i + 1) * hScale; j++)
		{
			ImageDrawPixel(&headImg, 2 * j, size - i - 1, WHITE);
			ImageDrawPixel(&headImg, 2 * j + 1, size - i - 1, WHITE);
		}
	}
	Arrow::Head = LoadTextureFromImage(headImg);
	UnloadImage(headImg);

	FEN_Codes =
	{
		std::make_pair<PieceType, char>(PieceType::W_PAWN, 'P'),
		std::make_pair<PieceType, char>(PieceType::W_KNIGHT, 'N'),
		std::make_pair<PieceType, char>(PieceType::W_BISHOP, 'B'),
		std::make_pair<PieceType, char>(PieceType::W_ROOK, 'R'),
		std::make_pair<PieceType, char>(PieceType::W_QUEEN, 'Q'),
		std::make_pair<PieceType, char>(PieceType::W_KING, 'K'),
		std::make_pair<PieceType, char>(PieceType::B_PAWN, 'p'),
		std::make_pair<PieceType, char>(PieceType::B_KNIGHT, 'n'),
		std::make_pair<PieceType, char>(PieceType::B_BISHOP, 'b'),
		std::make_pair<PieceType, char>(PieceType::B_ROOK, 'r'),
		std::make_pair<PieceType, char>(PieceType::B_QUEEN, 'q'),
		std::make_pair<PieceType, char>(PieceType::B_KING, 'k')
	};
	FEN_Codes_Backwards =
	{
		std::make_pair<char, PieceType>('P', PieceType::W_PAWN),
		std::make_pair<char, PieceType>('N', PieceType::W_KNIGHT),
		std::make_pair<char, PieceType>('B', PieceType::W_BISHOP),
		std::make_pair<char, PieceType>('R', PieceType::W_ROOK),
		std::make_pair<char, PieceType>('Q', PieceType::W_QUEEN),
		std::make_pair<char, PieceType>('K', PieceType::W_KING),
		std::make_pair<char, PieceType>('p', PieceType::B_PAWN),
		std::make_pair<char, PieceType>('n', PieceType::B_KNIGHT),
		std::make_pair<char, PieceType>('b', PieceType::B_BISHOP),
		std::make_pair<char, PieceType>('r', PieceType::B_ROOK),
		std::make_pair<char, PieceType>('q', PieceType::B_QUEEN),
		std::make_pair<char, PieceType>('k', PieceType::B_KING)
	};

	UpdateBounds();
}

void IBoard::Update()
{
	//Set cursor and set drag
	bool overlap = false;
	Vector2 mousePosition = GetMousePosition();
	if (CheckCollisionPointRec(mousePosition, m_BoardBounds))
	{
		Vector2 mouseSquare = _GetSquare(mousePosition);
		Piece& piece = m_Board[(int)mouseSquare.y][(int)mouseSquare.x];
		Vector2 square = _GetRealSquare(piece.GetPosition());
		//Check spot overlap and/or click
		if (m_SelectedPiece)
		{
			if (!m_DraggedPiece)
			{
				for (int i = 0; i < m_SelectedMoves.size(); i++)
				{
					Vector2& spotSquare = m_SelectedMoves[i].GetSquare();
					//Spot overlap
					if (square.x == spotSquare.x && square.y == spotSquare.y)
					{
						//Spot click
						if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
							_Move(_GetRealSquare(m_SelectedPiece->GetPreviousPosition()), square, true);
						overlap = true;
					}
				}
				//Overlap selected piece
				if (!overlap && m_SelectedPiece->OverlapPoint(mousePosition))
					overlap = true;

				//Mouse down
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
					m_LeftDragStart = GetMousePosition();
				//Mouse moved while down and piece can move, start the drag
				if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
				{
					Vector2 mousePos = GetMousePosition();
					if (m_LeftDragStart.x != mousePos.x || m_LeftDragStart.y != mousePos.y)
					{
						m_DraggedPiece = m_SelectedPiece;
						m_DraggedPiece->SetDrag(true);
					}
				}
			}
			else if (m_SelectedPiece->GetSide() == m_Side || m_AnalyseMode)
				overlap = true;
		}
		//Piece overlap
		if (!overlap)
		{
			if (piece.GetType() != PieceType::NONE)
			{
				//Piece select and/or drag
				if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
				{
					m_SelectedPiece = &piece;
					Vector2 square = _GetSquare(m_SelectedPiece->GetPosition());
					m_SelectionFrom = Highlight(Vector2{ m_BoardBounds.x + square.x * m_SquareSize, m_BoardBounds.y + square.y * m_SquareSize }, square, Fade(ORANGE, 0.4f), m_Flipped);
					m_DraggedPiece = m_SelectedPiece;
					m_DraggedPiece->SetDrag(true);

					if (m_SelectedPiece->GetSide() == m_Side || m_AnalyseMode)
					{
						m_SelectedMoves.clear();
						std::string prevSquare = _ToChessNote(_GetRealSquare(m_SelectedPiece->GetPreviousPosition()));
						for (int i = 0; i < m_LegalMoves.size(); i++)
						{
							if (m_LegalMoves[i].substr(0, 2) == prevSquare)
							{
								Vector2 square = _ToRealSquare(m_LegalMoves[i].substr(2, 2));
								m_SelectedMoves.emplace_back(Vector2{ m_BoardBounds.x + (square.x + 0.5f) * m_SquareSize, m_BoardBounds.y + (square.y + 0.5f) * m_SquareSize }, square, Fade(BLACK, 0.4f), m_Flipped);
							}
						}
					}
				}
				if (piece.GetSide() == m_Side || m_AnalyseMode)
					overlap = true;
			}
		}
	}
	m_PointingHand = overlap;

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
		if (m_DraggedPiece && (m_DraggedPiece->GetSide() == m_Side || m_AnalyseMode))
			_Move(_GetRealSquare(m_DraggedPiece->GetPreviousPosition()), _GetSquare(GetMousePosition()));
		else
		{
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
	}

	//Check arrows
	if (IsKeyPressed(KEY_LEFT) && m_MoveIndex > 0)
	{
		m_MoveIndex--;
		_ReloadBoard(false);
	}
	else if (IsKeyPressed(KEY_RIGHT) && m_MoveIndex < m_Moves.size() - 1)
	{
		m_MoveIndex++;
		_ReloadBoard(true);
	}
	else if (IsKeyPressed(KEY_UP) && m_MoveIndex != 0 && m_Moves.size() > 0)
	{
		m_MoveIndex = 0;
		_ReloadBoard(false);
	}
	else if (IsKeyPressed(KEY_DOWN) && m_MoveIndex != m_Moves.size() - 1)
	{
		m_MoveIndex = m_Moves.size() - 1;
		_ReloadBoard(false);
	}

	//Check delete
	if (IsKeyPressed(KEY_DELETE))
	{
		if (m_MoveIndex > 0)
		{
			m_MoveIndex--;
			m_Moves.resize(m_MoveIndex + 1);
			m_MovesSN.resize(m_MoveIndex + 1);
			_ReloadBoard(false);
		}
		else if (m_MoveIndex == 0)
			LoadFEN(m_StartingFEN);
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

	//Update last move
	if (m_Moves.size() > 0)
	{
		std::string move = m_Moves[m_MoveIndex];
		Vector2 fromSquare = _ToRealSquare(move.substr(0, 2));
		Vector2 toSquare = _ToRealSquare(move.substr(2, 2));
		m_LastMoveFrom = Highlight(Vector2{ m_BoardBounds.x + fromSquare.x * m_SquareSize, m_BoardBounds.y + fromSquare.y * m_SquareSize }, fromSquare, Fade(ORANGE, 0.4f), m_Flipped);
		m_LastMoveTo = Highlight(Vector2{ m_BoardBounds.x + toSquare.x * m_SquareSize, m_BoardBounds.y + toSquare.y * m_SquareSize }, toSquare, Fade(ORANGE, 0.4f), m_Flipped);
	}

	//Check win
	if (m_LegalMoves.size() == 0)
	{
		if (m_SideToMove == 0)
		{
			//Black won
			if (m_WhiteInCheck)
				m_Result = Result::BLACK_WIN;
			//Stalemate
			else
				m_Result = Result::STALEMATE;
		}
		else
		{
			//White won
			if (m_BlackInCheck)
				m_Result = Result::WHITE_WIN;
			//Stalemate
			else
				m_Result = Result::STALEMATE;
		}
	}
	else if (m_HalfMoveClock >= 100)
		m_Result = Result::DRAW;

	//Flip board
	if (IsKeyReleased(KEY_F))
		Flip();

	//Check resize
	if (IsWindowResized())
		UpdateBounds();
}

void IBoard::UpdateBounds()
{
	float boardSize = GetScreenHeight() - 2 * NAMETAG_HEIGHT_A * m_ShowNametag;
	//Match height
	if (boardSize < GetScreenWidth())
	{
		float offset = (GetScreenWidth() - boardSize) * 0.5f;
		m_BoardBounds = Rectangle{ offset, NAMETAG_HEIGHT_A * (float)m_ShowNametag, boardSize, boardSize };
	}
	//Match width
	else
	{
		boardSize = GetScreenWidth();
		float offset = (GetScreenHeight() - boardSize - 2 * NAMETAG_HEIGHT_A * (float)m_ShowNametag) * 0.5f;
		m_BoardBounds = Rectangle{ 0, offset + NAMETAG_HEIGHT_A * (float)m_ShowNametag, boardSize, boardSize };
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

void IBoard::Draw() const
{
	//Show nametags
	if (m_ShowNametag)
	{
		//Draw player images
		DrawRectangleRounded(Rectangle{ m_BoardBounds.x, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f, PLAYER_IMAGE_SIZE_A, PLAYER_IMAGE_SIZE_A }, 0.2f, 4, GameData::Colors.BgFocused2);
		DrawRectangleRounded(Rectangle{ m_BoardBounds.x + 1, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f + 1, PLAYER_IMAGE_SIZE_A - 2, PLAYER_IMAGE_SIZE_A - 2 }, 0.2f, 4, GameData::Colors.BgFocused);
		DrawRectangleRounded(Rectangle{ m_BoardBounds.x, (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f, PLAYER_IMAGE_SIZE_A, PLAYER_IMAGE_SIZE_A }, 0.2f, 4, GameData::Colors.BgFocused);
		DrawRectangleRounded(Rectangle{ m_BoardBounds.x + 1, (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f + 1, PLAYER_IMAGE_SIZE_A - 2, PLAYER_IMAGE_SIZE_A - 2 }, 0.2f, 4, GameData::Colors.BgFocused2);

		//Draw nametags
		DrawTextEx(GameData::MainFont, m_WhiteName.c_str(), Vector2{ m_BoardBounds.x + PLAYER_IMAGE_SIZE_A + 10, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f }, 50, 0, RAYWHITE);
		DrawTextEx(GameData::MainFont, m_BlackName.c_str(), Vector2{ m_BoardBounds.x + PLAYER_IMAGE_SIZE_A + 10, (m_BoardBounds.y - 60) * 0.5f }, 50, 0, RAYWHITE);
	}

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
	if (!m_Flipped)
	{
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
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			if (i % 2 == 0)
				DrawTextEx(GameData::MainFont, std::to_string(i + 1).c_str(), Vector2{ m_BoardBounds.x + offset, m_BoardBounds.y + i * m_SquareSize + offset }, fontSize, 0.0f, GameData::Colors.DarkAvarage);
			else
				DrawTextEx(GameData::MainFont, std::to_string(i + 1).c_str(), Vector2{ m_BoardBounds.x + offset, m_BoardBounds.y + i * m_SquareSize + offset }, fontSize, 0.0f, GameData::Colors.LightAvarage);
		}
		for (int i = 0; i < 8; i++)
		{
			std::string letter = std::string(1, 'a' + (char)i);
			float width = MeasureTextEx(GameData::MainFont, letter.c_str(), fontSize, 0.0f).x;
			if (i % 2 == 0)
				DrawTextEx(GameData::MainFont, letter.c_str(), Vector2{ m_BoardBounds.x + (8 - i) * m_SquareSize - offset - width, m_BoardBounds.y + m_BoardBounds.height - offset - fontSize }, fontSize, 0.0f, GameData::Colors.DarkAvarage);
			else
				DrawTextEx(GameData::MainFont, letter.c_str(), Vector2{ m_BoardBounds.x + (8 - i) * m_SquareSize - offset - width, m_BoardBounds.y + m_BoardBounds.height - offset - fontSize }, fontSize, 0.0f, GameData::Colors.LightAvarage);
		}
	}

	//Draw highlights
	for (int i = 0; i < m_Highlights.size(); i++)
		m_Highlights[i].Draw(m_BoardBounds);

	//Draw last move
	if (m_Moves.size() > 0) m_LastMoveFrom.Draw(m_BoardBounds);
	if (m_Moves.size() > 0) m_LastMoveTo.Draw(m_BoardBounds);

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

	//Draw selected moves
	if (m_ShowLegalMoves)
	{
		for (int i = 0; i < m_SelectedMoves.size(); i++)
			m_SelectedMoves[i].Draw(m_BoardBounds);
	}

	//Draw dragged move
	if (m_DraggedPiece)
		m_DraggedPiece->Draw();

	//Draw arrows
	for (int i = 0; i < m_Arrows.size(); i++)
		m_Arrows[i].Draw(m_BoardBounds);

	//Draw winner popup
	if (m_Result == Result::WHITE_WIN)
		GuiMessageBox(Rectangle{ 300, 300, 300, 150 }, "Congratulations", "White won!", "Ok");
	else if (m_Result == Result::BLACK_WIN)
		GuiMessageBox(Rectangle{ 300, 300, 300, 150 }, "Congratulations", "Black won!", "Ok");
	else if (m_Result == Result::STALEMATE)
		GuiMessageBox(Rectangle{ 300, 300, 300, 150 }, "Game over", "Stalemate!", "Ok");
	else if (m_Result == Result::DRAW)
		GuiMessageBox(Rectangle{ 300, 300, 300, 150 }, "Game over", "Draw!", "Ok");

	//Set cursor
	if (m_PointingHand)
		SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
	else
		SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

IBoard::~IBoard()
{
	if (GameData::CurrentEngine)
		GameData::CurrentEngine->Stop();

	UnloadTexture(Arrow::Head);
}

void IBoard::Flip()
{
	m_Flipped = !m_Flipped;

	for (int i = 0; i < m_Highlights.size(); i++)
		m_Highlights[i].SetFlip(m_Flipped);

	for (int i = 0; i < m_Arrows.size(); i++)
		m_Arrows[i].SetFlip(m_Flipped);

	for (int i = 0; i < m_SelectedMoves.size(); i++)
		m_SelectedMoves[i].SetFlip(m_Flipped);

	m_SelectionFrom.SetFlip(m_Flipped);
	m_LastMoveFrom.SetFlip(m_Flipped);
	m_LastMoveTo.SetFlip(m_Flipped);
}

void IBoard::Reset(bool resetEnginePosition)
{
	Clear();

	if (m_StartingFEN_Movecount != -1)
		LoadFEN(m_StartingFEN);
	else
	{
		//Black
		m_Board[0][0].SetType(PieceType::B_ROOK);
		m_Board[0][1].SetType(PieceType::B_KNIGHT);
		m_Board[0][2].SetType(PieceType::B_BISHOP);
		m_Board[0][3].SetType(PieceType::B_QUEEN);
		m_Board[0][4].SetType(PieceType::B_KING);
		m_Board[0][5].SetType(PieceType::B_BISHOP);
		m_Board[0][6].SetType(PieceType::B_KNIGHT);
		m_Board[0][7].SetType(PieceType::B_ROOK);

		m_Board[1][7].SetType(PieceType::B_PAWN);
		m_Board[1][0].SetType(PieceType::B_PAWN);
		m_Board[1][1].SetType(PieceType::B_PAWN);
		m_Board[1][2].SetType(PieceType::B_PAWN);
		m_Board[1][3].SetType(PieceType::B_PAWN);
		m_Board[1][4].SetType(PieceType::B_PAWN);
		m_Board[1][5].SetType(PieceType::B_PAWN);
		m_Board[1][6].SetType(PieceType::B_PAWN);
		m_Board[1][7].SetType(PieceType::B_PAWN);


		//White
		m_Board[7][0].SetType(PieceType::W_ROOK);
		m_Board[7][1].SetType(PieceType::W_KNIGHT);
		m_Board[7][2].SetType(PieceType::W_BISHOP);
		m_Board[7][3].SetType(PieceType::W_QUEEN);
		m_Board[7][4].SetType(PieceType::W_KING);
		m_Board[7][5].SetType(PieceType::W_BISHOP);
		m_Board[7][6].SetType(PieceType::W_KNIGHT);
		m_Board[7][7].SetType(PieceType::W_ROOK);

		m_Board[6][7].SetType(PieceType::W_PAWN);
		m_Board[6][0].SetType(PieceType::W_PAWN);
		m_Board[6][1].SetType(PieceType::W_PAWN);
		m_Board[6][2].SetType(PieceType::W_PAWN);
		m_Board[6][3].SetType(PieceType::W_PAWN);
		m_Board[6][4].SetType(PieceType::W_PAWN);
		m_Board[6][5].SetType(PieceType::W_PAWN);
		m_Board[6][6].SetType(PieceType::W_PAWN);
		m_Board[6][7].SetType(PieceType::W_PAWN);

		for (int i = 0; i < 8; i++)
			for (int j = 0; j < 8; j++)
				m_Board[i][j].SetHasMoved(false);

		m_BlackKing = &m_Board[0][4];
		m_WhiteKing = &m_Board[7][4];

		m_HalfMoveClock = 0;

		m_WhiteInCheck = false;
		m_BlackInCheck = false;

		m_WhiteShort = true;
		m_WhiteLong = true;
		m_BlackShort = true;
		m_BlackLong = true;

		m_SideToMove = 0;
		m_Flipped = false;

		m_Moves.clear();
		m_MovesSN.clear();
		m_MoveIndex = -1;
		if (resetEnginePosition)
			_GetLegalMoves(m_SideToMove);
	}

	m_Result = Result::NONE;

	if (resetEnginePosition)
	{
		m_SelectedPiece = nullptr;
		m_SelectedMoves.clear();
	}

	if (GameData::CurrentEngine && resetEnginePosition)
		GameData::CurrentEngine->SetPosition(m_StartingFEN);
}

bool IBoard::LoadFEN(const std::string& fen)
{
	std::vector<std::string> splitted = Utils::Split(std::string(fen), " ");
	if (splitted.size() != 6)
		return false;
	
	//Backup current state
	Piece backup[8][8];
	memcpy(backup, m_Board, sizeof(m_Board));
	Piece* whiteKingBackup = m_WhiteKing;
	Piece* blackKingBackup = m_BlackKing;
	bool whiteShortBackup = m_WhiteShort;
	bool whiteLongBackup = m_WhiteLong;
	bool blackShortBackup = m_BlackShort;
	bool blacktLongBackup = m_BlackLong;

	//Set the board
	m_Highlights.clear();
	m_Arrows.clear();
	m_SelectedPiece = nullptr;
	m_SelectedMoves.clear();
	if (m_DraggedPiece)
	{
		m_DraggedPiece->SetDrag(false);
		m_DraggedPiece = nullptr;
	}
	Clear();
	std::string& token = splitted[0];
	if (std::count(token.begin(), token.end(), '/') != 7)
		return false;
	int idx = 0; bool valid = true;
	for (int i = 0, j = 0; j < token.size(); j++)
	{
		if (token[j] == '/')
		{
			i++;
			//invalid FEN, restore
			if (idx < 7)
			{
				valid = false;
				break;
			}
			idx = 0;
		}
		else
		{
			if (FEN_Codes_Backwards.find(token[j]) != FEN_Codes_Backwards.end())
			{
				m_Board[i][idx].SetType(FEN_Codes_Backwards[token[j]]);
				//invalid FEN, restore
				if (++idx > 8)
				{
					valid = false;
					break;
				}
			}
			else
			{
				int count = token[j] - '0';
				for (int c = 0; c < count; c++)
					m_Board[i][idx++].SetType(PieceType::NONE);
				
				//invalid FEN, restore
				if (idx > 8)
				{
					valid = false;
					break;
				}
			}
		}
	}
	//invalid FEN, restore
	if (!valid)
	{
		memcpy(m_Board, backup, sizeof(backup));
		m_WhiteKing = whiteKingBackup;
		m_BlackKing = blackKingBackup;
		return false;
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			for (int j = 0; j < 8; j++)
			{
				Piece& piece = m_Board[i][j];
				if (piece.GetType() == PieceType::W_KING)
					m_WhiteKing = &piece;
				else if (piece.GetType() == PieceType::B_KING)
					m_BlackKing = &piece;
			}
		}
	}

	//Set side to move
	if (splitted[1] == "w")
		m_SideToMove = 0;
	else if (splitted[1] == "b")
		m_SideToMove = 1;
	//invalid FEN
	else
		return false;
	
	//Set movecount for every piece
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			Piece& piece = m_Board[i][j];
			if (piece.GetType() == PieceType::W_PAWN && i == 6)
				piece.SetHasMoved(false);
			else if (piece.GetType() == PieceType::B_PAWN && i == 1)
				piece.SetHasMoved(false);
			else
				piece.SetHasMoved(true);
		}
	}

	//Set castling availability
	for (int i = 0; i < splitted[2].size(); i++)
	{
		if (splitted[2][i] == 'K')
			m_WhiteShort = true;
		else if (splitted[2][i] == 'Q')
			m_WhiteLong = true;
		else if (splitted[2][i] == 'k')
			m_BlackShort = true;
		else if (splitted[2][i] == 'q')
			m_BlackLong = true;
		else if (splitted[2][i] == '-')
		{
			m_WhiteShort = false;
			m_WhiteLong = false;
			m_BlackShort = false;
			m_BlackLong = false;
		}
		//invalid FEN
		else
			valid = false;
	}
	if (!valid)
	{
		m_WhiteShort = whiteShortBackup;
		m_WhiteLong = whiteLongBackup;
		m_BlackShort = blackShortBackup;
		m_BlackLong = blacktLongBackup;
		return false;
	}

	//Set en passant
	if (splitted[3] != "-")
	{
		//invalid FEN
		if (splitted[3].size() != 2 || splitted[3][0] - 'a' < 0 || splitted[3][0] - 'a' > 7)
			return false;
		if (splitted[3][1] - '3' != 0 && splitted[3][1] - '6' != 0)
			return false;

		m_EnPassant = "";
		m_EnPassant.push_back(splitted[3][0]);
		m_EnPassant.push_back(m_SideToMove ? '2' : '7');
		m_EnPassant.push_back(splitted[3][0]);
		m_EnPassant.push_back(m_SideToMove ? '4' : '5');
	}

	//Set halfmove clock
	if (!Utils::IsNumber(splitted[4]))
		return false;
	m_HalfMoveClock = stoi(splitted[4]);

	//Set fullmove number
	if (!Utils::IsNumber(splitted[5]))
		return false;

	m_StartingFEN_Movecount = stoi(splitted[5]) * 2 + m_SideToMove - 2;
	m_Moves.clear();
	m_MovesSN.clear();
	m_MoveIndex = -1;

	m_StartingFEN = fen;

	if (GameData::CurrentEngine)
		GameData::CurrentEngine->SetPosition(fen);

	_GetLegalMoves(m_SideToMove);

	return true;
}

bool IBoard::LoadPGN(std::string& pgn)
{
	bool censor = false;
	for (int i = 0; i < pgn.size(); i++)
	{
		if (pgn[i] == '{')
		{
			pgn[i] = '#';
			censor = true;
		}
		else if (pgn[i] == '}')
		{
			pgn[i] = '#';
			censor = false;
		}
		else if (censor)
			pgn[i] = '#';
	}
	censor = false;
	for (int i = 0; i < pgn.size(); i++)
	{
		if (pgn[i] == '[')
		{
			pgn[i] = '#';
			censor = true;
		}
		else if (pgn[i] == ']')
		{
			pgn[i] = '#';
			censor = false;
		}
		else if (censor)
			pgn[i] = '#';
	}
	
	//REMOVE ALL SIDELINES
	censor = false;
	for (int i = 0; i < pgn.size(); i++)
	{
		if (pgn[i] == '(')
		{
			pgn[i] = '#';
			censor = true;
		}
		else if (pgn[i] == ')')
		{
			pgn[i] = '#';
			censor = false;
		}
		else if (censor)
			pgn[i] = '#';
	}

	//Remove #, /r and /n
	pgn.erase(std::remove(pgn.begin(), pgn.end(), '#'), pgn.end());
	pgn.erase(std::remove(pgn.begin(), pgn.end(), '\r'), pgn.end());
	std::replace(pgn.begin(), pgn.end(), '\n', ' ');

	//Backup current state
	Piece backup[8][8];
	memcpy(backup, m_Board, sizeof(m_Board));
	Piece* whiteKingBackup = m_WhiteKing;
	Piece* blackKingBackup = m_BlackKing;
	bool whiteShortBackup = m_WhiteShort;
	bool whiteLongBackup = m_WhiteLong;
	bool blackShortBackup = m_BlackShort;
	bool blacktLongBackup = m_BlackLong;
	int8_t sideToMoveBackup = m_SideToMove;
	std::vector<std::string> movesBackup = m_Moves;
	std::vector<std::string> movesSNBackup = m_MovesSN;

	//Set the board
	m_Highlights.clear();
	m_Arrows.clear();
	m_SelectedPiece = nullptr;
	m_SelectedMoves.clear();
	if (m_DraggedPiece)
	{
		m_DraggedPiece->SetDrag(false);
		m_DraggedPiece = nullptr;
	}
	Reset();

	std::string move = ""; char c;
	for (int i = pgn.find_first_of('.') + 2; i < pgn.size(); i++)
	{
		c = pgn[i];
		if (c == '.')
		{
			i++;
			move = "";
		}
		else if (c == ' ')
		{
			move = _GetLongNotation(move);
			if (!_Move(move, false, false))
			{
				memcpy(m_Board, backup, sizeof(backup));
				m_WhiteKing = whiteKingBackup;
				m_BlackKing = blackKingBackup;
				m_WhiteShort = whiteShortBackup;
				m_WhiteLong = whiteLongBackup;
				m_BlackShort = blackShortBackup;
				m_BlackLong = blacktLongBackup;
				m_SideToMove = sideToMoveBackup;
				m_Moves = movesBackup;
				m_MovesSN = movesSNBackup;
				GameData::CurrentEngine->SetPosition(_GetFEN());
				return false;
			}
			move = "";
		}
		else
			move.push_back(c);
	}
	GameData::CurrentEngine->SetPosition(_GetFEN());
	return true;
}

void IBoard::Clear()
{
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			m_Board[i][j] = Piece(PieceType::NONE, m_Board[i][j].GetPosition());
			m_Board[i][j].TellBoardBounds(m_BoardBounds);
		}
	}
}

bool IBoard::_Move(const Vector2& fromSquare, const Vector2& toSquare, bool animated, bool updateEnginePosition)
{
	//Move should overwrite current move or should not be played
	if (m_MoveIndex != m_Moves.size() - 1)
	{
		//AnalyseMode, overwriting is allowed
		if (m_AnalyseMode)
		{
			std::string move = _ToChessNote(fromSquare) + _ToChessNote(toSquare);

			//Overwrite the current line if necessary
			if (move != m_Moves[m_MoveIndex + 1])
			{
				if ((std::find(m_LegalMoves.begin(), m_LegalMoves.end(), move) != m_LegalMoves.end() || !m_OnlyLegalMoves) && move.substr(0, 2) != move.substr(2, 2))
				{
					if (_IsOnBoard(fromSquare) && _IsOnBoard(toSquare))
					{
						_ReloadBoard(false);
						bool capture;
						_TestMove(move, &capture);
						m_Moves.resize(m_MoveIndex + 1); m_MovesSN.resize(m_MoveIndex + 1);	//TEMPORARY
						_DoMove(move, true, animated);
						if (updateEnginePosition)
							GameData::CurrentEngine->SetPosition(_GetFEN());
					}
					else
						m_Board[(int)fromSquare.y][(int)fromSquare.x].SetType(PieceType::NONE);
				}
				else
				{
					if (m_DraggedPiece)
					{
						m_DraggedPiece->SetDrag(false);
						m_DraggedPiece = nullptr;
					}
				}
			}
			else
			{
				m_MoveIndex++;
				_ReloadBoard(animated);
			}
		}
		//Move is not made, jump to the end of the line
		else
		{
			int8_t sideToMove = (m_SideToMove + m_Moves.size() - m_MoveIndex + 1) % 2;
			m_MoveIndex = m_Moves.size() - 1;
			_ReloadBoard(false);
			m_SideToMove = sideToMove;
			m_Highlights.clear();
			m_Arrows.clear();
			return false;
		}
	}
	//Move is added to the end of the movelist
	else
	{
		Vector2 piecePosition = m_Board[(int)fromSquare.y][(int)fromSquare.x].GetPosition();
		//If the dragged piece is the moved piece
		if (m_DraggedPiece && piecePosition.x == m_DraggedPiece->GetPosition().x && piecePosition.y == m_DraggedPiece->GetPosition().y)
		{
			m_DraggedPiece->SetDrag(false);
			m_DraggedPiece = nullptr;
		
			if (fromSquare.x != toSquare.x || fromSquare.y != toSquare.y)
			{
				m_SelectedPiece = nullptr;
				m_SelectedMoves.clear();
			}
		}
		std::string move = _ToChessNote(fromSquare) + _ToChessNote(toSquare);
		if ((std::find(m_LegalMoves.begin(), m_LegalMoves.end(), move) != m_LegalMoves.end()|| !m_OnlyLegalMoves) && move.substr(0, 2) != move.substr(2, 2))
		{
			if (_IsOnBoard(fromSquare) && _IsOnBoard(toSquare))
			{
				_DoMove(fromSquare, toSquare, true, animated);
				if (updateEnginePosition)
					GameData::CurrentEngine->SetPosition(_GetFEN());
				m_Highlights.clear();
				m_Arrows.clear();
			}
			else
				m_Board[(int)fromSquare.y][(int)fromSquare.x].SetType(PieceType::NONE);
			return true;
		}
		else return false;
	}
	return false;
}

bool IBoard::_Move(const std::string& move, bool animated, bool updateEnginePosition)
{
	Vector2 from = _ToRealSquare(move.substr(0, 2));
	Vector2 to = _ToRealSquare(move.substr(2, 2));
	return _Move(from, to, animated, updateEnginePosition);
}

void IBoard::_DoMove(const Vector2& fromSquare, const Vector2& toSquare, bool registerMove, bool animated, bool playSound)
{
	Piece* piece = &m_Board[(int)fromSquare.y][(int)fromSquare.x];
	std::string move = _ToChessNote(fromSquare) + _ToChessNote(toSquare);

	bool moveIsLegal = std::find(m_LegalMoves.begin(), m_LegalMoves.end(), move) != m_LegalMoves.end();
	bool capture = false;
	if (m_Board[(int)toSquare.y][(int)toSquare.x].GetType() != PieceType::NONE)
		capture = true;

	if (moveIsLegal && m_OnlyLegalMoves)
	{
		//Is the move a castle
		if (piece->GetType() == PieceType::W_KING && (std::abs(toSquare.x - fromSquare.x) == 2 || std::abs(toSquare.x - fromSquare.x) == 3 || std::abs(toSquare.x - fromSquare.x) == 4))
			return _CastleKing(m_WhiteKing, (toSquare.x - fromSquare.x) / 2, registerMove, animated, playSound);
		else if (piece->GetType() == PieceType::B_KING && (std::abs(toSquare.x - fromSquare.x) == 2 || std::abs(toSquare.x - fromSquare.x) == 3 || std::abs(toSquare.x - fromSquare.x) == 4))
			return _CastleKing(m_BlackKing, (toSquare.x - fromSquare.x) / 2, registerMove, animated, playSound);

		//Check en passant
		if (fromSquare.x != toSquare.x && piece->GetType() == PieceType::W_PAWN && m_Board[(int)toSquare.y][(int)toSquare.x].GetType() == PieceType::NONE)
		{
			m_Board[(int)toSquare.y + 1][(int)toSquare.x] = Piece(PieceType::NONE, m_Board[(int)toSquare.y + 1][(int)toSquare.x].GetPosition());
			capture = true;
		}
		else if (fromSquare.x != toSquare.x && piece->GetType() == PieceType::B_PAWN && m_Board[(int)toSquare.y][(int)toSquare.x].GetType() == PieceType::NONE)
		{
			m_Board[(int)toSquare.y - 1][(int)toSquare.x] = Piece(PieceType::NONE, m_Board[(int)toSquare.y - 1][(int)toSquare.x].GetPosition());
			capture = true;
		}
	}

	//Update castling availability
	if (registerMove && m_OnlyLegalMoves)
	{
		if (piece->GetType() == PieceType::W_KING)
		{
			m_WhiteShort = false;
			m_WhiteLong = false;
		}
		else if (piece->GetType() == PieceType::B_KING)
		{
			m_BlackShort = false;
			m_BlackLong = false;
		}
		else if (piece->GetType() == PieceType::W_ROOK)
		{
			if (fromSquare.y == 7)
			{
				if (fromSquare.x == 0)
					m_WhiteLong = false;
				else if (fromSquare.x == 7)
					m_WhiteShort = false;
			}
		}
		else if (piece->GetType() == PieceType::B_ROOK)
		{
			if (fromSquare.y == 0)
			{
				if (fromSquare.x == 0)
					m_BlackLong = false;
				else if (fromSquare.x == 7)
					m_BlackShort = false;
			}
		}
	}

	//Move the piece in the array
	Vector2 tempPosition = piece->GetPosition();
	if (animated)
		piece->StartAnimation(m_Board[(int)toSquare.y][(int)toSquare.x].GetPosition());
	else 
		piece->SetPosition(m_Board[(int)toSquare.y][(int)toSquare.x].GetPosition());
	m_Board[(int)toSquare.y][(int)toSquare.x] = *piece;
	m_Board[(int)fromSquare.y][(int)fromSquare.x] = Piece(PieceType::NONE, tempPosition);
	piece = &m_Board[(int)toSquare.y][(int)toSquare.x];

	//Update king pointers if necessary
	if (piece->GetType() == PieceType::W_KING)
		m_WhiteKing = piece;
	else if (piece->GetType() == PieceType::B_KING)
		m_BlackKing = piece;

	if (moveIsLegal && m_OnlyLegalMoves)
	{
		//Check if pawn transforms
		if (piece->GetType() == PieceType::W_PAWN && toSquare.y == 0)
		{
			piece->SetType(PieceType::W_QUEEN);
			move += "q";
		}
		else if (piece->GetType() == PieceType::B_PAWN && toSquare.y == 7)
		{
			piece->SetType(PieceType::B_QUEEN);
			move += "q";
		}

		//Check en passant availability
		if (piece->GetType() == PieceType::W_PAWN && toSquare.y - fromSquare.y == -2)
			m_EnPassant = _ToChessNote(Vector2{ fromSquare.x, fromSquare.y - 1 });
		else if (piece->GetType() == PieceType::B_PAWN && toSquare.y - fromSquare.y == 2)
			m_EnPassant = _ToChessNote(Vector2{ fromSquare.x, fromSquare.y + 1 });
		else
			m_EnPassant = "";
		
		//Update check variables
		m_WhiteInCheck = _IsAttacked(m_WhiteKing);
		m_BlackInCheck = _IsAttacked(m_BlackKing);
		
		m_SideToMove = !m_SideToMove;
	}

	//Register if necessary
	if (registerMove)
	{
		_RegisterMove(piece, move, capture, !capture && piece->GetType() != PieceType::W_PAWN && piece->GetType() != PieceType::B_PAWN);
		_GetLegalMoves(m_SideToMove);

		//Play sound
		if (playSound)
		{
			if (capture)
				PlayASound(GameData::Sounds.CaptureSound);
			else
				PlayASound(GameData::Sounds.MoveSound);
		}
	}
}

void IBoard::_DoMove(const std::string& move, bool registerMove, bool animated, bool playSound)
{
	Vector2 fromSquare = _ToRealSquare(move.substr(0, 2));
	Vector2 toSquare = _ToRealSquare(move.substr(2, 2));
	IBoard::_DoMove(fromSquare, toSquare, registerMove, animated, playSound);
}

bool IBoard::_TestMove(const Vector2& fromSquare, const Vector2& toSquare, bool* capture)
{
	//Set capture pointer
	if (capture)
	{
		if (m_Board[(int)toSquare.y][(int)toSquare.x].GetType() != PieceType::NONE)
			*capture = true;
		else
			*capture = false;

		//Check en passant
		Piece* piece = &m_Board[(int)fromSquare.y][(int)fromSquare.x];
		if (fromSquare.x != toSquare.x && piece->GetType() == PieceType::W_PAWN && m_Board[(int)toSquare.y][(int)toSquare.x].GetType() == PieceType::NONE)
		{
			m_Board[(int)toSquare.y + 1][(int)toSquare.x] = Piece(PieceType::NONE, m_Board[(int)toSquare.y + 1][(int)toSquare.x].GetPosition());
			*capture = true;
		}
		else if (fromSquare.x != toSquare.x && piece->GetType() == PieceType::B_PAWN && m_Board[(int)toSquare.y][(int)toSquare.x].GetType() == PieceType::NONE)
		{
			m_Board[(int)toSquare.y - 1][(int)toSquare.x] = Piece(PieceType::NONE, m_Board[(int)toSquare.y - 1][(int)toSquare.x].GetPosition());
			*capture = true;
		}
	}

	//Is the move a castle
	Piece* piece = &m_Board[(int)fromSquare.y][(int)fromSquare.x];
	if (piece->GetType() == PieceType::W_KING && (std::abs(toSquare.x - fromSquare.x) == 2 || std::abs(toSquare.x - fromSquare.x) == 3 || std::abs(toSquare.x - fromSquare.x) == 4))
		return _TestCastle(m_WhiteKing, (toSquare.x - fromSquare.x) / 2);
	else if (piece->GetType() == PieceType::B_KING && (std::abs(toSquare.x - fromSquare.x) == 2 || std::abs(toSquare.x - fromSquare.x) == 3 || std::abs(toSquare.x - fromSquare.x) == 4))
		return _TestCastle(m_BlackKing, (toSquare.x - fromSquare.x) / 2);

	//Backup current state
	Piece backup[8][8];
	memcpy(backup, m_Board, sizeof(m_Board));
	Piece* whiteKingBackup = m_WhiteKing;
	Piece* blackKingBackup = m_BlackKing;

	//Move the piece in the array
	int side = piece->GetSide();
	Vector2 tempPosition = piece->GetPosition();
	piece->SetPosition(m_Board[(int)toSquare.y][(int)toSquare.x].GetPosition());
	m_Board[(int)toSquare.y][(int)toSquare.x] = *piece;
	m_Board[(int)fromSquare.y][(int)fromSquare.x] = Piece(PieceType::NONE, tempPosition);
	piece = &m_Board[(int)toSquare.y][(int)toSquare.x];
	
	//Update king pointers if necessary
	if (piece->GetType() == PieceType::W_KING)
		m_WhiteKing = piece;
	else if (piece->GetType() == PieceType::B_KING)
		m_BlackKing = piece;

	//Check if the king is in check, restore board
	if ((side == 0 && _IsAttacked(m_WhiteKing)) || (side == 1 && _IsAttacked(m_BlackKing)))
	{
		memcpy(m_Board, backup, sizeof(backup));
		m_WhiteKing = whiteKingBackup;
		m_BlackKing = blackKingBackup;
		return false;
	}
	else
	{
		memcpy(m_Board, backup, sizeof(backup));
		m_WhiteKing = whiteKingBackup;
		m_BlackKing = blackKingBackup;
		return true;
	}
}

bool IBoard::_TestMove(const std::string& move, bool* capture)
{
	Vector2 fromSquare = _ToRealSquare(move.substr(0, 2));
	Vector2 toSquare = _ToRealSquare(move.substr(2, 2));
	return IBoard::_TestMove(fromSquare, toSquare, capture);
}

void IBoard::_CastleKing(Piece* king, int8_t direction, bool registerMove, bool animated, bool playSound)
{
	Vector2 square = _GetRealSquare(king->GetPosition());

	//Short castle
	if (direction > 0)
	{
		//Move the king in the array
		Piece& rook = m_Board[(int)square.y][7];
		Vector2 tempPosition = king->GetPosition();
		if (animated)
			king->StartAnimation(m_Board[(int)square.y][6].GetPosition());
		else
			king->SetPosition(m_Board[(int)square.y][6].GetPosition());
		m_Board[(int)square.y][6] = *king;
		m_Board[(int)square.y][4] = Piece(PieceType::NONE, tempPosition);
		king = &m_Board[(int)square.y][6];

		//Move the rook in the array
		tempPosition = rook.GetPosition();
		if (animated)
			rook.StartAnimation(m_Board[(int)square.y][5].GetPosition());
		else
			rook.SetPosition(m_Board[(int)square.y][5].GetPosition());
		m_Board[(int)square.y][5] = rook;
		m_Board[(int)square.y][7] = Piece(PieceType::NONE, tempPosition);

		m_SideToMove = !m_SideToMove;

		//Register if necessary
		if (registerMove)
		{
			_RegisterMove(king, _ToChessNote(Vector2{ 4, square.y }) + _ToChessNote(Vector2{ 6, square.y }), false, true);
			_GetLegalMoves(m_SideToMove);

			//Play sound
			if (playSound)
				PlayASound(GameData::Sounds.CastleSound);
		}

	}
	//Long castle
	else if (direction < 0)
	{
		//Move the king in the array
		Piece& rook = m_Board[(int)square.y][0];
		Vector2 tempPosition = king->GetPosition();
		if (animated)
			king->StartAnimation(m_Board[(int)square.y][2].GetPosition());
		else
			king->SetPosition(m_Board[(int)square.y][2].GetPosition());
		m_Board[(int)square.y][2] = *king;
		m_Board[(int)square.y][4] = Piece(PieceType::NONE, tempPosition);
		king = &m_Board[(int)square.y][2];

		//Move the rook in the array
		tempPosition = rook.GetPosition();
		if (animated)
			rook.StartAnimation(m_Board[(int)square.y][3].GetPosition());
		else
			rook.SetPosition(m_Board[(int)square.y][3].GetPosition());
		m_Board[(int)square.y][3] = rook;
		m_Board[(int)square.y][0] = Piece(PieceType::NONE, tempPosition);

		m_SideToMove = !m_SideToMove;

		//Register if necessary
		if (registerMove)
		{
			_RegisterMove(king, _ToChessNote(Vector2{ 4, square.y }) + _ToChessNote(Vector2{ 2, square.y }), false, true);
			_GetLegalMoves(m_SideToMove);

			//Play sound
			if (playSound)
				PlayASound(GameData::Sounds.CastleSound);
		}

	}

	//Update king pointers
	if (king->GetSide() == 0)
		m_WhiteKing = king;
	else if (king->GetSide() == 1)
		m_BlackKing = king;
}

bool IBoard::_TestCastle(Piece* king, int8_t direction)
{
	if (king->GetSide() == 0)
	{
		if (m_WhiteInCheck) return false;
		if (direction > 0 && !m_WhiteShort) return false;
		if (direction < 0 && !m_WhiteLong) return false;
	}
	else if (king->GetSide() == 1)
	{
		if (m_BlackInCheck) return false;
		if (direction > 0 && !m_BlackShort) return false;
		if (direction < 0 && !m_BlackLong) return false;
	}

	Vector2 square = _GetRealSquare(king->GetPosition());

	//Short castle
	if (direction > 0)
	{
		Piece& rook = m_Board[(int)square.y][7];
		if ((rook.GetType() == PieceType::W_ROOK || rook.GetType() == PieceType::B_ROOK) &&
			king->GetSide() == rook.GetSide() &&
			m_Board[(int)square.y][5].GetType() == PieceType::NONE &&
			m_Board[(int)square.y][6].GetType() == PieceType::NONE)
		{
			Piece backup[8][8];
			memcpy(backup, m_Board, sizeof(m_Board));

			//Move the king in the array once
			Vector2 tempPosition = king->GetPosition();
			king->SetPosition(m_Board[(int)square.y][5].GetPosition());
			m_Board[(int)square.y][5] = *king;
			m_Board[(int)square.y][4] = Piece(PieceType::NONE, tempPosition);
			king = &m_Board[(int)square.y][5];

			//Check if king is in check
			if (_IsAttacked(king))
			{
				memcpy(m_Board, backup, sizeof(backup));
				return false;
			}

			//Move the king in the array once again
			tempPosition = king->GetPosition();
			king->SetPosition(m_Board[(int)square.y][6].GetPosition());
			m_Board[(int)square.y][6] = *king;
			m_Board[(int)square.y][5] = Piece(PieceType::NONE, tempPosition);
			king = &m_Board[(int)square.y][6];

			//Check if king is in check
			if (_IsAttacked(king))
			{
				memcpy(m_Board, backup, sizeof(backup));
				return false;
			}

			memcpy(m_Board, backup, sizeof(backup));
			return true;
		}
		else return false;
	}
	//Long castle
	else if (direction < 0)
	{
		Piece& rook = m_Board[(int)square.y][0];
		if ((rook.GetType() == PieceType::W_ROOK || rook.GetType() == PieceType::B_ROOK) &&
			king->GetSide() == rook.GetSide() &&
			m_Board[(int)square.y][1].GetType() == PieceType::NONE &&
			m_Board[(int)square.y][2].GetType() == PieceType::NONE &&
			m_Board[(int)square.y][3].GetType() == PieceType::NONE)
		{
			Piece backup[8][8];
			memcpy(backup, m_Board, sizeof(m_Board));

			//Move the king in the array once
			Vector2 tempPosition = king->GetPosition();
			king->SetPosition(m_Board[(int)square.y][3].GetPosition());
			m_Board[(int)square.y][3] = *king;
			m_Board[(int)square.y][4] = Piece(PieceType::NONE, tempPosition);
			king = &m_Board[(int)square.y][3];

			//Check if king is in check
			if (_IsAttacked(king))
			{
				memcpy(m_Board, backup, sizeof(backup));
				return false;
			}

			//Move the king in the array once again
			tempPosition = king->GetPosition();
			king->SetPosition(m_Board[(int)square.y][2].GetPosition());
			m_Board[(int)square.y][2] = *king;
			m_Board[(int)square.y][3] = Piece(PieceType::NONE, tempPosition);
			king = &m_Board[(int)square.y][2];

			//Check if king is in check
			if (_IsAttacked(king))
			{
				memcpy(m_Board, backup, sizeof(backup));
				return false;
			}

			memcpy(m_Board, backup, sizeof(backup));
			return true;
		}
		else return false;
	}
	else return false;
}

void IBoard::_GetLegalMoves(uint8_t side)
{
	std::vector<std::pair<Vector2, Vector2>> pseudoLegalMoves;
	m_LegalMoves.clear();

	//White
	if (side == 0)
	{
		for (int m = 0; m < 8; m++)
		{
			for (int n = 0; n < 8; n++)
			{
				Piece& piece = m_Board[m][n];
				Vector2 square = _GetRealSquare(piece.GetPreviousPosition());
				switch (piece.GetType())
				{
				case PieceType::W_PAWN:
				{
					//1 move forward
					if (square.y > 0 && m_Board[(int)square.y - 1][(int)square.x].GetType() == PieceType::NONE) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, square.y - 1 });
					//Capture diagonally
					if (square.x > 0 && square.y > 0 && m_Board[(int)square.y - 1][(int)square.x - 1].GetSide() == 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y - 1 });
					if (square.x < 7 && square.y > 0 && m_Board[(int)square.y - 1][(int)square.x + 1].GetSide() == 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y - 1 });
					//2 moves forward
					if (square.y > 1 && m_Board[(int)square.y - 1][(int)square.x].GetType() == PieceType::NONE && m_Board[(int)square.y - 2][(int)square.x].GetType() == PieceType::NONE && !piece.GetHasMoved()) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, square.y - 2 });
					//En passant
					if (square.x < 7 && square.y > 1 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x + 1, square.y - 1 }) == m_EnPassant)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y - 1 });
					if (square.x > 0 && square.y > 1 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x - 1, square.y - 1 }) == m_EnPassant)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y - 1 });
					break;
				}
				case PieceType::W_KNIGHT:
				{
					//L shape
					if (square.y > 1 && square.x < 7 && m_Board[(int)square.y - 2][(int)square.x + 1].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y - 2 });
					if (square.y > 0 && square.x < 6 && m_Board[(int)square.y - 1][(int)square.x + 2].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y - 1 });
					if (square.y < 7 && square.x < 6 && m_Board[(int)square.y + 1][(int)square.x + 2].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y + 1 });
					if (square.y < 6 && square.x < 7 && m_Board[(int)square.y + 2][(int)square.x + 1].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y + 2 });
					if (square.y < 6 && square.x > 0 && m_Board[(int)square.y + 2][(int)square.x - 1].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y + 2 });
					if (square.y < 7 && square.x > 1 && m_Board[(int)square.y + 1][(int)square.x - 2].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y + 1 });
					if (square.y > 0 && square.x > 1 && m_Board[(int)square.y - 1][(int)square.x - 2].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y - 1 });
					if (square.y > 1 && square.x > 0 && m_Board[(int)square.y - 2][(int)square.x - 1].GetSide() != 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y - 2 });
					break;
				}
				case PieceType::W_BISHOP:
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					break;
				}
				case PieceType::W_ROOK:
				{
					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					break;
				}
				case PieceType::W_QUEEN:
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}

					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 1)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					break;
				}
				case PieceType::W_KING:
				{
					//Check surrounding squares
					int startI = square.x > 0 ? square.x - 1 : square.x;
					int startJ = square.y > 0 ? square.y - 1 : square.y;
					int endI = square.x < 7 ? square.x + 2 : square.x + 1;
					int endJ = square.y < 7 ? square.y + 2 : square.y + 1;
					for (int j = startJ; j < endJ; j++)
						for (int i = startI; i < endI; i++)
							if (m_Board[j][i].GetSide() != 0)
								pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });

					//Castling
					if (square.x == 4 && square.y == 7)
					{
						//Short
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y });
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 3, square.y });
						//Long
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y });
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 4, square.y });
					}
					break;
				}
				}
			}
		}
	}
	//Black
	else if (side == 1)
	{
		for (int m = 0; m < 8; m++)
		{
			for (int n = 0; n < 8; n++)
			{
				Piece& piece = m_Board[m][n];
				Vector2 square = _GetRealSquare(piece.GetPosition());
				switch (piece.GetType())
				{
				case PieceType::B_PAWN:
				{
					//1 move forward
					if (square.y < 7 && m_Board[(int)square.y + 1][(int)square.x].GetType() == PieceType::NONE) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, square.y + 1 });
					//Capture diagonally
					if (square.x > 0 && square.y < 7 && m_Board[(int)square.y + 1][(int)square.x - 1].GetSide() == 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y + 1 });
					if (square.x < 7 && square.y < 7 && m_Board[(int)square.y + 1][(int)square.x + 1].GetSide() == 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y + 1 });
					//2 moves forward
					if (square.y < 6 && m_Board[(int)square.y + 1][(int)square.x].GetType() == PieceType::NONE && m_Board[(int)square.y + 2][(int)square.x].GetType() == PieceType::NONE && !piece.GetHasMoved()) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, square.y + 2 });
					//En passant
					if (square.x < 7 && square.y < 6 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x + 1, square.y + 1 }) == m_EnPassant)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y + 1 });
					if (square.x > 0 && square.y < 6 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x - 1, square.y + 1 }) == m_EnPassant)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y + 1 });
					break;
				}
				case PieceType::B_KNIGHT:
				{
					//L shape
					if (square.y > 1 && square.x < 7 && m_Board[(int)square.y - 2][(int)square.x + 1].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y - 2 });
					if (square.y > 0 && square.x < 6 && m_Board[(int)square.y - 1][(int)square.x + 2].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y - 1 });
					if (square.y < 7 && square.x < 6 && m_Board[(int)square.y + 1][(int)square.x + 2].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y + 1 });
					if (square.y < 6 && square.x < 7 && m_Board[(int)square.y + 2][(int)square.x + 1].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y + 2 });
					if (square.y < 6 && square.x > 0 && m_Board[(int)square.y + 2][(int)square.x - 1].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y + 2 });
					if (square.y < 7 && square.x > 1 && m_Board[(int)square.y + 1][(int)square.x - 2].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y + 1 });
					if (square.y > 0 && square.x > 1 && m_Board[(int)square.y - 1][(int)square.x - 2].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y - 1 });
					if (square.y > 1 && square.x > 0 && m_Board[(int)square.y - 2][(int)square.x - 1].GetSide() != 1) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 1, square.y - 2 });
					break;
				}
				case PieceType::B_BISHOP:
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					break;
				}
				case PieceType::B_ROOK:
				{
					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					break;
				}
				case PieceType::B_QUEEN:
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });
							break;
						}
						else break;
					}

					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, square.y });
							break;
						}
						else break;
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == -1)
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
						else if (tempPiece.GetSide() == 0)
						{
							pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, (float)i });
							break;
						}
						else break;
					}
					break;
				}
				case PieceType::B_KING:
				{
					//Check surrounding squares
					int startI = square.x > 0 ? square.x - 1 : square.x;
					int startJ = square.y > 0 ? square.y - 1 : square.y;
					int endI = square.x < 7 ? square.x + 2 : square.x + 1;
					int endJ = square.y < 7 ? square.y + 2 : square.y + 1;
					for (int j = startJ; j < endJ; j++)
						for (int i = startI; i < endI; i++)
							if (m_Board[j][i].GetSide() != 1)
								pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ (float)i, (float)j });

					//Castling
					if (square.x == 4 && square.y == 0)
					{
						//Short
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y });
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 3, square.y });
						//Long
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y });
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 4, square.y });
					}
					break;
				}
				}
			}
		}
	}

	for (int i = 0; i < pseudoLegalMoves.size(); i++)
		if (_TestMove(pseudoLegalMoves[i].first, pseudoLegalMoves[i].second))
			m_LegalMoves.push_back(_ToChessNote(pseudoLegalMoves[i].first) + _ToChessNote(pseudoLegalMoves[i].second));
}

bool IBoard::_IsAttacked(Piece* piece)
{
	if (!piece) return false;

	Vector2 square = _GetRealSquare(piece->GetPosition());
	//Get enemy piece types
	PieceType pawnType = (PieceType)((int)PieceType::W_PAWN + (piece->GetSide() + 6) % 7);
	PieceType knightType = (PieceType)((int)PieceType::W_KNIGHT + (piece->GetSide() + 6) % 7);
	PieceType bishopType = (PieceType)((int)PieceType::W_BISHOP + (piece->GetSide() + 6) % 7);
	PieceType rookType = (PieceType)((int)PieceType::W_ROOK + (piece->GetSide() + 6) % 7);
	PieceType queenType = (PieceType)((int)PieceType::W_QUEEN + (piece->GetSide() + 6) % 7);
	PieceType kingType = (PieceType)((int)PieceType::W_KING + (piece->GetSide() + 6) % 7);

	//Check right
	for (int i = square.x + 1; i < 8; i++)
	{
		Piece& tempPiece = m_Board[(int)square.y][i];
		if (tempPiece.GetType() == rookType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}
	//Check left
	for (int i = square.x - 1; i >= 0; i--)
	{
		Piece& tempPiece = m_Board[(int)square.y][i];
		if (tempPiece.GetType() == rookType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}
	//Check up
	for (int i = square.y + 1; i < 8; i++)
	{
		Piece& tempPiece = m_Board[i][(int)square.x];
		if (tempPiece.GetType() == rookType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}
	//Check down
	for (int i = square.y - 1; i >= 0; i--)
	{
		Piece& tempPiece = m_Board[i][(int)square.x];
		if (tempPiece.GetType() == rookType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}

	//Check down right
	for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
	{
		Piece& tempPiece = m_Board[j][i];
		if (tempPiece.GetType() == bishopType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}
	//Check down left
	for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
	{
		Piece& tempPiece = m_Board[j][i];
		if (tempPiece.GetType() == bishopType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}
	//Check up right
	for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
	{
		Piece& tempPiece = m_Board[j][i];
		if (tempPiece.GetType() == bishopType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}
	//Check up left
	for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
	{
		Piece& tempPiece = m_Board[j][i];
		if (tempPiece.GetType() == bishopType || tempPiece.GetType() == queenType)
			return true;
		else if (tempPiece.GetType() != PieceType::NONE)
			break;
	}

	//Check knight
	if (square.y > 2 && square.x < 7 && m_Board[(int)square.y - 2][(int)square.x + 1].GetType() == knightType) return true;
	if (square.y > 1 && square.x < 6 && m_Board[(int)square.y - 1][(int)square.x + 2].GetType() == knightType) return true;
	if (square.y < 7 && square.x < 6 && m_Board[(int)square.y + 1][(int)square.x + 2].GetType() == knightType) return true;
	if (square.y < 6 && square.x < 7 && m_Board[(int)square.y + 2][(int)square.x + 1].GetType() == knightType) return true;
	if (square.y < 6 && square.x > 1 && m_Board[(int)square.y + 2][(int)square.x - 1].GetType() == knightType) return true;
	if (square.y < 7 && square.x > 2 && m_Board[(int)square.y + 1][(int)square.x - 2].GetType() == knightType) return true;
	if (square.y > 1 && square.x > 2 && m_Board[(int)square.y - 1][(int)square.x - 2].GetType() == knightType) return true;
	if (square.y > 2 && square.x > 1 && m_Board[(int)square.y - 2][(int)square.x - 1].GetType() == knightType) return true;

	//Check pawn
	//White
	if (piece->GetSide() == 0)
	{
		if (square.y > 0 && square.x > 0 && m_Board[(int)square.y - 1][(int)square.x - 1].GetType() == pawnType) return true;
		if (square.y > 0 && square.x < 7 && m_Board[(int)square.y - 1][(int)square.x + 1].GetType() == pawnType) return true;
	}
	//Black
	else
	{
		if (square.y < 7 && square.x > 0 && m_Board[(int)square.y + 1][(int)square.x - 1].GetType() == pawnType) return true;
		if (square.y < 7 && square.x < 7 && m_Board[(int)square.y + 1][(int)square.x + 1].GetType() == pawnType) return true;
	}

	//Check king
	
	//int startI = square.x > 0 ? square.x - 1 : square.x;	
	//int startJ = square.y < 7 ? 6 - square.y : 7 - square.y;
	//int endI = square.x < 7 ? square.x + 2 : square.x + 1;
	//int endJ = square.y > 0 ? 9 - square.y : 8 - square.y;

	int startI = square.x > 0 ? square.x - 1 : square.x;
	int startJ = square.y > 0 ? square.y - 1 : square.y;
	int endI = square.x < 7 ? square.x + 2 : square.x + 1;
	int endJ = square.y < 7 ? square.y + 2 : square.y + 1;
	for (int j = startJ; j < endJ; j++)
		for (int i = startI; i < endI; i++)
			if (m_Board[j][i].GetType() == kingType)
				return true;

	//Otherwise there is no attack
	return false;
}

bool IBoard::_IsOnBoard(const Vector2& square)
{
	return square.x >= 0 && square.x <= 7 && square.y >= 0 && square.y <= 7;
}

void IBoard::_RegisterMove(Piece* piece, const std::string& move, bool capture, bool halfMove)
{
	m_Moves.push_back(move);
	m_MovesSN.push_back(_GetShortNotation(move, capture));
	m_MoveIndex++;
	if (halfMove)
		m_HalfMoveClock++;
	else
		m_HalfMoveClock = 0;
	if (m_OnlyLegalMoves)
		piece->SetHasMoved(true);
}

void IBoard::_ReloadBoard(bool lastMoveVisible)
{
	//Reset board
	std::vector<std::string> movesBackup = m_Moves;
	std::vector<std::string> movesSNBackup = m_MovesSN;
	bool flippedBackup = m_Flipped;
	int32_t moveIndexBackup = m_MoveIndex;
	int8_t sideToMoveBackup = m_SideToMove;

	if (m_DraggedPiece)
	{
		m_DraggedPiece->SetDrag(false);
		m_DraggedPiece = nullptr;
	}
	IBoard::Reset();
	m_Flipped = flippedBackup;

	//Redo moves
	if (moveIndexBackup > -1)
	{
		for (int i = 0; i < moveIndexBackup; i++)
			IBoard::_DoMove(movesBackup[i], true, false, false);
		IBoard::_DoMove(movesBackup[moveIndexBackup], true, lastMoveVisible, lastMoveVisible);
	}

	//Add back remaining moves
	for (int i = moveIndexBackup + 1; i < movesBackup.size(); i++)
	{
		m_Moves.push_back(movesBackup[i]);
		m_MovesSN.push_back(movesSNBackup[i]);
	}

	//Clear guides
	m_Highlights.clear();
	m_Arrows.clear();
	GameData::CurrentEngine->SetPosition(_GetFEN());
}

std::string IBoard::_ToChessNote(const Vector2& square) const
{
	std::string move = "";
	move += char(97 + square.x);
	move += std::to_string(8 - (int)square.y);
	return move;
}

std::string IBoard::_GetShortNotation(const std::string& move, bool capture)
{
	Vector2 from = _ToRealSquare(move.substr(0, 2));
	Vector2 to = _ToRealSquare(move.substr(2, 2));
	Piece& piece = m_Board[(int)to.y][(int)to.x];

	switch (piece.GetType())
	{
		//Pawn move
		case PieceType::W_PAWN:
		case PieceType::B_PAWN:
		{
			return (capture ? _ToChessNote(from).substr(0, 1) + "x" : "") + _ToChessNote(to);
		}
		case PieceType::W_KNIGHT:
		case PieceType::B_KNIGHT:
		{
			bool possible = false;
			bool sameCol = false;
			for (int i = 0; i < 8; i++)
			{
				for (int j = 0; j < 8; j++)
				{
					Piece& tempPiece = m_Board[i][j];
					if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide())
					{
						Vector2 diff = { std::abs(to.x - j), std::abs(to.y - i) };
						if (diff.x == 1 && diff.y == 2 && _TestMove(Vector2{ (float)j, (float)i }, to))
							possible = true;
						else if (diff.x == 2 && diff.y == 1 && _TestMove(Vector2{ (float)j, (float)i }, to))
							possible = true;
						if (from.x == j)
							sameCol = true;
					}
				}
			}
			return "N" + (possible ? (sameCol ? _ToChessNote(from).substr(1, 1) : _ToChessNote(from).substr(0, 1)) : "") + (capture ? "x" : "") + _ToChessNote(to);
		}
		case PieceType::W_BISHOP:
		case PieceType::B_BISHOP:
		{
			//Check down right
			for (int i = to.x + 1, j = to.y + 1; i < 8 && j < 8; i++, j++)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "B" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "B" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check down left
			for (int i = to.x - 1, j = to.y + 1; i >= 0 && j < 8; i--, j++)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "B" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "B" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check up right
			for (int i = to.x + 1, j = to.y - 1; i < 8 && j >= 0; i++, j--)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "B" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "B" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check up left
			for (int i = to.x - 1, j = to.y - 1; i >= 0 && j >= 0; i--, j--)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "B" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "B" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			return std::string("B") + (capture ? "x" : "") + _ToChessNote(to);
		}
		case PieceType::W_ROOK:
		case PieceType::B_ROOK:
		{
			//Check right
			for (int i = to.x + 1; i < 8; i++)
			{
				Piece& tempPiece = m_Board[(int)to.y][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ to.y, (float)i }, to))
				{
					if (from.x == i)
						return "R" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "R" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check left
			for (int i = to.x - 1; i >= 0; i--)
			{
				Piece& tempPiece = m_Board[(int)to.y][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ to.y, (float)i }, to))
				{
					if (from.x == i)
						return "R" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "R" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check down
			for (int i = to.y + 1; i < 8; i++)
			{
				Piece& tempPiece = m_Board[i][(int)to.x];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)i, to.x }, to))
				{
					if (from.x == to.x)
						return "R" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "R" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check up
			for (int i = to.y - 1; i >= 0; i--)
			{
				Piece& tempPiece = m_Board[i][(int)to.x];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)i, to.x }, to))
				{
					if (from.x == to.x)
						return "R" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "R" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			return std::string("R") + (capture ? "x" : "") + _ToChessNote(to);
		}
		case PieceType::W_QUEEN:
		case PieceType::B_QUEEN:
		{
			//Check down right
			for (int i = to.x + 1, j = to.y + 1; i < 8 && j < 8; i++, j++)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check down left
			for (int i = to.x - 1, j = to.y + 1; i >= 0 && j < 8; i--, j++)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check up right
			for (int i = to.x + 1, j = to.y - 1; i < 8 && j >= 0; i++, j--)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check up left
			for (int i = to.x - 1, j = to.y - 1; i >= 0 && j >= 0; i--, j--)
			{
				Piece& tempPiece = m_Board[j][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)j, (float)i }, to))
				{
					if (from.x == i)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			
			//Check right
			for (int i = to.x + 1; i < 8; i++)
			{
				Piece& tempPiece = m_Board[(int)to.y][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ to.y, (float)i }, to))
				{
					if (from.x == i)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check left
			for (int i = to.x - 1; i >= 0; i--)
			{
				Piece& tempPiece = m_Board[(int)to.y][i];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ to.y, (float)i }, to))
				{
					if (from.x == i)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check down
			for (int i = to.y + 1; i < 8; i++)
			{
				Piece& tempPiece = m_Board[i][(int)to.x];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)i, to.x }, to))
				{
					if (from.x == to.x)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			//Check up
			for (int i = to.y - 1; i >= 0; i--)
			{
				Piece& tempPiece = m_Board[i][(int)to.x];
				if (tempPiece.GetType() == piece.GetType() && tempPiece.GetSide() == piece.GetSide() && _TestMove(Vector2{ (float)i, to.x }, to))
				{
					if (from.x == to.x)
						return "Q" + _ToChessNote(from).substr(1, 1) + (capture ? "x" : "") + _ToChessNote(to);
					else
						return "Q" + _ToChessNote(from).substr(0, 1) + (capture ? "x" : "") + _ToChessNote(to);
				}
				else if (tempPiece.GetSide() != -1)
					break;
			}
			return std::string("Q") + (capture ? "x" : "") + _ToChessNote(to);
		}
		case PieceType::W_KING:
		case PieceType::B_KING:
		{
			//Short castle
			if (to.x - from.x == 2)
				return "O-O";
			//Long castle
			else if (to.x - from.x == -2)
				return "O-O-O";
			//Normal move
			else
				return "K" + _ToChessNote(to);
		}
	}	
	return "";
}

std::string IBoard::_GetLongNotation(std::string& move)
{
	//Check castles
	if (move == "O-O")
	{
		if (!m_SideToMove)
			return "e1g1";
		else
			return "e8g8";
	}
	else if (move == "O-O-O")
	{
		if (!m_SideToMove)
			return "e1c1";
		else
			return "e8c8";
	}

	//Remove check or checkmate sign
	if (move.back() == '+' || move.back() == '#')
		move.pop_back();

	//Remove capture sign
	int idx = move.find('x');
	if (idx != -1)
		move.erase(idx, 1);

	//Pawn moves
	if (move.size() == 2)
	{
		Vector2 square = _ToRealSquare(move);
		//White moves
		if (!m_SideToMove)
		{
			if (m_Board[(int)square.y + 1][(int)square.x].GetType() == PieceType::W_PAWN)
				return _ToChessNote(Vector2{ square.x, square.y + 1 }) + move;
			else if (m_Board[(int)square.y + 2][(int)square.x].GetType() == PieceType::W_PAWN)
				return _ToChessNote(Vector2{ square.x, square.y + 2 }) + move;
			else
				return "?";
		}
		//Black moves
		else
		{
			if (m_Board[(int)square.y - 1][(int)square.x].GetType() == PieceType::B_PAWN)
				return _ToChessNote(Vector2{ square.x, square.y - 1 }) + move;
			else if (m_Board[(int)square.y - 2][(int)square.x].GetType() == PieceType::B_PAWN)
				return _ToChessNote(Vector2{ square.x, square.y - 2 }) + move;
			else
				return "?";
		}
	}
	else if (move.size() == 3)
	{
		Vector2 square = _ToRealSquare(move.substr(1, 2));
		//Pawn takes
		if (islower(move[0]))
		{
			//White moves
			if (!m_SideToMove)
			{
				move.insert(move.begin() + 1, move[2] - 1);
				return move;
			}
			//Black moves
			else
			{
				move.insert(move.begin() + 1, move[2] + 1);
				return move;
			}
		}
		//Piece moves or takes unambiguously
		else
		{
			//White moves
			if (!m_SideToMove)
			{
				if (move[0] == 'N')
				{
					if (m_Board[(int)square.y - 2][(int)square.x + 1].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x + 1, square.y - 2 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y - 1][(int)square.x + 2].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x + 2, square.y - 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 1][(int)square.x + 2].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x + 2, square.y + 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 2][(int)square.x + 1].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x + 1, square.y + 2 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 2][(int)square.x - 1].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x - 1, square.y + 2 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 1][(int)square.x - 2].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x - 2, square.y + 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y - 1][(int)square.x - 2].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x - 2, square.y - 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y - 2][(int)square.x - 1].GetType() == PieceType::W_KNIGHT) return _ToChessNote(Vector2{ square.x - 1, square.y - 2 }) + move.substr(1, 2);
				}
				else if (move[0] == 'B')
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'R')
				{
					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_ROOK)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_ROOK)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_ROOK)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_ROOK)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'Q')
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}

					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 1)
							break;
						else if (tempPiece.GetType() == PieceType::W_QUEEN)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'K')
				{
					//Check surrounding squares
					int startI = square.x > 0 ? square.x - 1 : square.x;
					int startJ = square.y > 0 ? square.y - 1 : square.y;
					int endI = square.x < 7 ? square.x + 2 : square.x + 1;
					int endJ = square.y < 7 ? square.y + 2 : square.y + 1;
					for (int j = startJ; j < endJ; j++)
						for (int i = startI; i < endI; i++)
							if (m_Board[j][i].GetType() == PieceType::W_KING)
								return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
				}
			}
			//Black moves
			else
			{
				if (move[0] == 'N')
				{
					if (m_Board[(int)square.y - 2][(int)square.x + 1].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x + 1, square.y - 2 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y - 1][(int)square.x + 2].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x + 2, square.y - 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 1][(int)square.x + 2].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x + 2, square.y + 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 2][(int)square.x + 1].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x + 1, square.y + 2 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 2][(int)square.x - 1].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x - 1, square.y + 2 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y + 1][(int)square.x - 2].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x - 2, square.y + 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y - 1][(int)square.x - 2].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x - 2, square.y - 1 }) + move.substr(1, 2);
					else if (m_Board[(int)square.y - 2][(int)square.x - 1].GetType() == PieceType::B_KNIGHT) return _ToChessNote(Vector2{ square.x - 1, square.y - 2 }) + move.substr(1, 2);
				}
				else if (move[0] == 'B')
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_BISHOP)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'R')
				{
					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_ROOK)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_ROOK)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_ROOK)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_ROOK)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'Q')
				{
					//Check down right
					for (int i = square.x + 1, j = square.y + 1; i < 8 && j < 8; i++, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check down left
					for (int i = square.x - 1, j = square.y + 1; i >= 0 && j < 8; i--, j++)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up right
					for (int i = square.x + 1, j = square.y - 1; i < 8 && j >= 0; i++, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}
					//Check up left
					for (int i = square.x - 1, j = square.y - 1; i >= 0 && j >= 0; i--, j--)
					{
						Piece& tempPiece = m_Board[j][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
					}

					//Check right
					for (int i = square.x + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check left
					for (int i = square.x - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[(int)square.y][i];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
					}
					//Check down
					for (int i = square.y + 1; i < 8; i++)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
					//Check up
					for (int i = square.y - 1; i >= 0; i--)
					{
						Piece& tempPiece = m_Board[i][(int)square.x];
						if (tempPiece.GetSide() == 0)
							break;
						else if (tempPiece.GetType() == PieceType::B_QUEEN)
							return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'K')
				{
					//Check surrounding squares
					int startI = square.x > 0 ? square.x - 1 : square.x;
					int startJ = square.y > 0 ? square.y - 1 : square.y;
					int endI = square.x < 7 ? square.x + 2 : square.x + 1;
					int endJ = square.y < 7 ? square.y + 2 : square.y + 1;
					for (int j = startJ; j < endJ; j++)
						for (int i = startI; i < endI; i++)
							if (m_Board[j][i].GetType() == PieceType::B_KING)
								return _ToChessNote(Vector2{ (float)i, (float)j }) + move.substr(1, 2);
				}
			}
		}
	}
	//Piece moves ambiguously or promotion
	else if (move.size() == 4)
	{
		Vector2 square = _ToRealSquare(move.substr(2, 2));
		//Promotion
		if (move.find('=') != -1)
			return move;
		//White moves
		if (!m_SideToMove)
		{
			//Row is the same
			if (!Utils::IsNumber(move.substr(1, 1)))
			{
				if (move[0] == 'N')
				{
					if (std::abs(move[2] - move[1]) == 1)
					{
						if (square.x >= 2 && m_Board[(int)square.y][(int)square.x - 2].GetType() == PieceType::W_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x - 2 }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x + 2 }) + move.substr(1, 2);
					}
					else
					{
						if (square.x >= 1 && m_Board[(int)square.y][(int)square.x - 1].GetType() == PieceType::W_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x - 1 }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x + 1 }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'B')
				{
					int diff = std::abs(move[2] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::W_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::W_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
				}
				else if (move[0] == 'R')
				{
					move = move.substr(1, 3);
					if (move[0] != move[1])
					{
						move.insert(move.begin() + 1, move[2]);
						return move;
					}
					else
					{
						//Check down
						for (int i = square.y + 1; i < 8; i++)
						{
							Piece& tempPiece = m_Board[i][(int)square.x];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::W_ROOK)
								return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
						}
						//Check up
						for (int i = square.y - 1; i >= 0; i--)
						{
							Piece& tempPiece = m_Board[i][(int)square.x];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::W_ROOK)
								return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
						}
					}
				}
				else if (move[0] == 'Q')
				{
					int diff = std::abs(move[2] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::W_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::W_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
					else
					{
						move = move.substr(1, 3);
						if (move[0] != move[1])
						{
							move.insert(move.begin() + 1, move[2]);
							return move;
						}
						else
						{
							//Check down
							for (int i = square.y + 1; i < 8; i++)
							{
								Piece& tempPiece = m_Board[i][(int)square.x];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::W_QUEEN)
									return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
							}
							//Check up
							for (int i = square.y - 1; i >= 0; i--)
							{
								Piece& tempPiece = m_Board[i][(int)square.x];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::W_QUEEN)
									return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
							}
						}
					}
				}
			}
			//Column is the same
			else
			{
				if (move[0] == 'N')
				{
					if (std::abs(move[3] - move[1]) == 1)
					{
						if (square.y >= 2 && m_Board[(int)square.y - 2][(int)square.x].GetType() == PieceType::W_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y - 2, (float)square.x }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y + 2, (float)square.x }) + move.substr(1, 2);
					}
					else
					{
						if (square.y >= 1 && m_Board[(int)square.y - 1][(int)square.x].GetType() == PieceType::W_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y - 1, (float)square.x }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y + 1, (float)square.x }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'B')
				{
					int diff = std::abs(move[3] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::W_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::W_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
				}
				else if (move[0] == 'R')
				{
					move = move.substr(1, 3);
					if (move[0] != move[1])
					{
						move.insert(move.begin(), move[1]);
						return move;
					}
					else
					{
						//Check right
						for (int i = square.x + 1; i < 8; i++)
						{
							Piece& tempPiece = m_Board[(int)square.y][i];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::W_ROOK)
								return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
						}
						//Check left
						for (int i = square.x - 1; i >= 0; i--)
						{
							Piece& tempPiece = m_Board[(int)square.y][i];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::W_ROOK)
								return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
						}
					}
				}
				else if (move[0] == 'Q')
				{
					int diff = std::abs(move[3] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::W_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::W_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
					else
					{
						move = move.substr(1, 3);
						if (move[0] != move[1])
						{
							move.insert(move.begin(), move[1]);
							return move;
						}
						else
						{
							//Check right
							for (int i = square.x + 1; i < 8; i++)
							{
								Piece& tempPiece = m_Board[(int)square.y][i];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::W_QUEEN)
									return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
							}
							//Check left
							for (int i = square.x - 1; i >= 0; i--)
							{
								Piece& tempPiece = m_Board[(int)square.y][i];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::W_QUEEN)
									return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
							}
						}
					}
				}
			}
		}
		//Black moves
		else
		{
			//Row is the same
			if (!Utils::IsNumber(move.substr(1, 1)))
			{
				if (move[0] == 'N')
				{
					if (std::abs(move[2] - move[1]) == 1)
					{
						if (square.x >= 2 && m_Board[(int)square.y][(int)square.x - 2].GetType() == PieceType::B_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x - 2 }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x + 2 }) + move.substr(1, 2);
					}
					else
					{
						if (square.x >= 1 && m_Board[(int)square.y][(int)square.x - 1].GetType() == PieceType::B_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x - 1 }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y, (float)square.x + 1 }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'B')
				{
					int diff = std::abs(move[2] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::B_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::B_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
				}
				else if (move[0] == 'R')
				{
					move = move.substr(1, 3);
					if (move[0] != move[1])
					{
						move.insert(move.begin() + 1, move[2]);
						return move;
					}
					else
					{
						//Check down
						for (int i = square.y + 1; i < 8; i++)
						{
							Piece& tempPiece = m_Board[i][(int)square.x];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::B_ROOK)
								return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
						}
						//Check up
						for (int i = square.y - 1; i >= 0; i--)
						{
							Piece& tempPiece = m_Board[i][(int)square.x];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::B_ROOK)
								return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
						}
					}
				}
				else if (move[0] == 'Q')
				{
					int diff = std::abs(move[2] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::B_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::B_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
					else
					{
						move = move.substr(1, 3);
						if (move[0] != move[1])
						{
							move.insert(move.begin() + 1, move[2]);
							return move;
						}
						else
						{
							//Check down
							for (int i = square.y + 1; i < 8; i++)
							{
								Piece& tempPiece = m_Board[i][(int)square.x];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::B_QUEEN)
									return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
							}
							//Check up
							for (int i = square.y - 1; i >= 0; i--)
							{
								Piece& tempPiece = m_Board[i][(int)square.x];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::B_QUEEN)
									return _ToChessNote(Vector2{ square.x, (float)i }) + move.substr(1, 2);
							}
						}
					}
				}
			}
			//Column is the same
			else
			{
				if (move[0] == 'N')
				{
					if (std::abs(move[3] - move[1]) == 1)
					{
						if (square.y >= 2 && m_Board[(int)square.y - 2][(int)square.x].GetType() == PieceType::B_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y - 2, (float)square.x }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y + 2, (float)square.x }) + move.substr(1, 2);
					}
					else
					{
						if (square.y >= 1 && m_Board[(int)square.y - 1][(int)square.x].GetType() == PieceType::B_KNIGHT)
							return _ToChessNote(Vector2{ (float)square.y - 1, (float)square.x }) + move.substr(1, 2);
						else
							return _ToChessNote(Vector2{ (float)square.y + 1, (float)square.x }) + move.substr(1, 2);
					}
				}
				else if (move[0] == 'B')
				{
					int diff = std::abs(move[3] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::B_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::B_BISHOP)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
				}
				else if (move[0] == 'R')
				{
					move = move.substr(1, 3);
					if (move[0] != move[1])
					{
						move.insert(move.begin(), move[1]);
						return move;
					}
					else
					{
						//Check right
						for (int i = square.x + 1; i < 8; i++)
						{
							Piece& tempPiece = m_Board[(int)square.y][i];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::B_ROOK)
								return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
						}
						//Check left
						for (int i = square.x - 1; i >= 0; i--)
						{
							Piece& tempPiece = m_Board[(int)square.y][i];
							if (tempPiece.GetSide() == 1)
								break;
							else if (tempPiece.GetType() == PieceType::B_ROOK)
								return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
						}
					}
				}
				else if (move[0] == 'Q')
				{
					int diff = std::abs(move[3] - move[1]);
					if (square.x >= diff && square.y >= diff && m_Board[(int)square.y - diff][(int)square.x - diff].GetType() == PieceType::B_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y - diff, (float)square.x - diff }) + move.substr(1, 2);
					else if (square.x + diff < 8 && square.y + diff < 8 && m_Board[(int)square.y + diff][(int)square.x + diff].GetType() == PieceType::B_QUEEN)
						return _ToChessNote(Vector2{ (float)square.y + diff, (float)square.x + diff }) + move.substr(1, 2);
					else
					{
						move = move.substr(1, 3);
						if (move[0] != move[1])
						{
							move.insert(move.begin(), move[1]);
							return move;
						}
						else
						{
							//Check right
							for (int i = square.x + 1; i < 8; i++)
							{
								Piece& tempPiece = m_Board[(int)square.y][i];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::B_QUEEN)
									return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
							}
							//Check left
							for (int i = square.x - 1; i >= 0; i--)
							{
								Piece& tempPiece = m_Board[(int)square.y][i];
								if (tempPiece.GetSide() == 1)
									break;
								else if (tempPiece.GetType() == PieceType::B_QUEEN)
									return _ToChessNote(Vector2{ (float)i, square.y }) + move.substr(1, 2);
							}
						}
					}
				}
			}
		}
	}
	return "";
}

Vector2 IBoard::_ToSquare(const std::string& move) const
{
	if (m_Flipped)
	{
		return Vector2
		{
			(float)(104 - move[0]),
			(float)(move[1] - '1'),
		};
	}
	else
	{
		return Vector2
		{
			(float)(move[0] - 97),
			(float)(8 - move[1] + '0'),
		};
	}
}

Vector2 IBoard::_ToRealSquare(const std::string& move) const
{
	return Vector2
	{
		(float)(move[0] - 97),
		(float)(8 - move[1] + '0'),
	};
}

Vector2 IBoard::_GetSquare(const Vector2& position) const
{
	Vector2 square{ (int)((position.x - m_BoardBounds.x) / m_SquareSize), (int)((position.y - m_BoardBounds.y) / m_SquareSize) };
	if (m_Flipped)
	{
		square.x = 7 - square.x;
		square.y = 7 - square.y;
	}
	return square;
}

Vector2 IBoard::_GetRealSquare(const Vector2& position) const
{
	return Vector2{ (float)((int)((position.x - m_BoardBounds.x) / m_SquareSize)), (float)((int)((position.y - m_BoardBounds.y) / m_SquareSize)) };
}

std::string IBoard::_MovesToString() const
{
	std::string moves = "";
	for (int i = 0; i < m_MoveIndex + 1; i++)
		moves += m_Moves[i] + " ";
	return moves;
}

std::string IBoard::_GetFEN() const
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
	fen += m_SideToMove ? "b " : "w ";

	//castling availability
	if (m_WhiteShort) fen += "K";
	if (m_WhiteLong) fen += "Q";
	if (m_BlackShort) fen += "k";
	if (m_BlackLong) fen += "q";
	if (fen.back() == ' ') fen += "-";
	fen += " ";

	//en passant target square (- or e3 or c6 etc)
	if (m_Moves.size() == 0)
		fen += "- ";
	else
	{
		const std::string& move = m_Moves.back();
		Vector2 from = _ToRealSquare(move.substr(0, 2));
		Vector2 to = _ToRealSquare(move.substr(2, 2));
		if (m_Board[(int)to.y][(int)to.x].GetType() == PieceType::W_PAWN && from.y - to.y == 2) fen += move.substr(0, 1) + std::to_string(3) + " ";
		else if (m_Board[(int)to.y][(int)to.x].GetType() == PieceType::B_PAWN && to.y - from.y == 2) fen += move.substr(0, 1) + std::to_string(6) + " ";
		else fen += "- ";
	}

	//halfmove clock
	fen += std::to_string(m_HalfMoveClock) + " ";

	//fullmove number
	fen += std::to_string(m_Moves.size() / 2 + 1 + (m_StartingFEN_Movecount == -1 ? 0 : m_StartingFEN_Movecount));

	return fen;
}

std::string IBoard::_GetPGN() const
{
	std::string pgn = "";
	for (int i = 0; i < m_MovesSN.size(); i++)
	{
		if (i % 2 == 0)
			pgn += std::to_string(i / 2 + 1) + ". ";

		pgn += m_MovesSN[i] + " ";
	}
	pgn += "*";
	return pgn;
}