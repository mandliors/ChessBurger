#include "IBoard.h"
#include "GameData/GameData.h"
#include "extras/raygui.h"
#include <cmath>

IBoard::IBoard(const ColorBuffer& colorBuffer, const Rectangle& bounds)
	: m_Moves({}), m_LegalMoves({}), m_WhiteName("Player 1"), m_Side(0), m_AnalyseMode(false), m_BlackName("Player 2"), m_MoveIndex(-1), m_SelectedMoves({}), m_Arrows({}), m_Highlights({}), m_SelectionFrom({}), m_LastMoveFrom({}), m_LastMoveTo({}), m_LeftDragStart({}), m_RightDragStart(Vector2{-1, -1}), m_MouseDownPosition(Vector2{-1, -1}), m_Result(Result::NONE), m_ColorBuffer(colorBuffer), m_BoardBounds({}), m_SquareSize(0), m_DraggedPiece(nullptr), m_SelectedPiece(nullptr), m_WhiteKing(nullptr), m_BlackKing(nullptr), m_Flipped(false), m_PointingHand(false), m_SideToMove(1), m_WhiteInCheck(false), m_BlackInCheck(false)
{
	m_Pieces[0]  = LoadTexture("assets/themes/wpawn.png");   SetTextureFilter(m_Pieces[0],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[0]);
	m_Pieces[1]  = LoadTexture("assets/themes/wknight.png"); SetTextureFilter(m_Pieces[1],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[1]);
	m_Pieces[2]  = LoadTexture("assets/themes/wbishop.png"); SetTextureFilter(m_Pieces[2],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[2]);
	m_Pieces[3]  = LoadTexture("assets/themes/wrook.png");	 SetTextureFilter(m_Pieces[3],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[3]);
	m_Pieces[4]  = LoadTexture("assets/themes/wqueen.png");	 SetTextureFilter(m_Pieces[4],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[4]);
	m_Pieces[5]  = LoadTexture("assets/themes/wking.png");	 SetTextureFilter(m_Pieces[5],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[5]);
	m_Pieces[6]  = LoadTexture("assets/themes/bpawn.png");	 SetTextureFilter(m_Pieces[6],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[6]);
	m_Pieces[7]  = LoadTexture("assets/themes/bknight.png"); SetTextureFilter(m_Pieces[7],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[7]);
	m_Pieces[8]  = LoadTexture("assets/themes/bbishop.png"); SetTextureFilter(m_Pieces[8],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[8]);
	m_Pieces[9]  = LoadTexture("assets/themes/brook.png");	 SetTextureFilter(m_Pieces[9],  TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[9]);
	m_Pieces[10] = LoadTexture("assets/themes/bqueen.png");	 SetTextureFilter(m_Pieces[10], TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[10]);
	m_Pieces[11] = LoadTexture("assets/themes/bking.png");	 SetTextureFilter(m_Pieces[11], TEXTURE_FILTER_BILINEAR); GenTextureMipmaps(&m_Pieces[11]);

	UpdateBounds();
}

void IBoard::Update()
{
	//Get selected moves
	if (m_SelectedPiece && (m_SelectedPiece->GetSide() == m_Side || m_AnalyseMode))
	{
		m_SelectedMoves.clear();
		std::string square = _ToChessNote(_GetRealSquare(m_SelectedPiece->GetPreviousPosition()));
		for (int i = 0; i < m_LegalMoves.size(); i++)
		{
			if (m_LegalMoves[i].substr(0, 2) == square)
			{
				Vector2 square = _ToRealSquare(m_LegalMoves[i].substr(2, 2));
				m_SelectedMoves.emplace_back(Vector2{ m_BoardBounds.x + (square.x + 0.5f) * m_SquareSize, m_BoardBounds.y + (square.y + 0.5f) * m_SquareSize }, square, Fade(BLACK, 0.4f), m_Flipped);
			}
		}
	}

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
					m_SelectionFrom = Highlight(m_SelectedPiece->GetPosition(), _GetSquare(m_SelectedPiece->GetPosition()), Fade(ORANGE, 0.4f));
					m_DraggedPiece = m_SelectedPiece;
					m_DraggedPiece->SetDrag(true);
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
		_ReloadBoard();
	}
	else if (IsKeyPressed(KEY_RIGHT) && m_MoveIndex < m_Moves.size() - 1)
	{
		m_MoveIndex++;
		_ReloadBoard();
	}
	else if (IsKeyPressed(KEY_UP) && m_MoveIndex != 0 && m_Moves.size() > 0)
	{
		m_MoveIndex = 0;
		_ReloadBoard();
	}
	else if (IsKeyPressed(KEY_DOWN) && m_MoveIndex != m_Moves.size() - 1)
	{
		m_MoveIndex = m_Moves.size() - 1;
		_ReloadBoard();
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
					Vector2{ (to.x + 0.5f) * m_SquareSize + m_BoardBounds.x, (to.y + 0.5f) * m_SquareSize + m_BoardBounds.y }, from, to, Fade(RED, 0.8f), m_Flipped);
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

	//Flip board
	if (IsKeyReleased(KEY_R))
		Flip();

	//Check resize
	if (IsWindowResized())
		UpdateBounds();
}

void IBoard::UpdateBounds()
{
	float boardSize = GetScreenHeight() - 2 * NAMETAG_HEIGHT_A;
	//Match height
	if (boardSize < GetScreenWidth())
	{
		float offset = (GetScreenWidth() - boardSize) * 0.5f;
		m_BoardBounds = Rectangle{ offset, NAMETAG_HEIGHT_A, boardSize, boardSize };
	}
	//Match width
	else
	{
		boardSize = GetScreenWidth();
		float offset = (GetScreenHeight() - boardSize - 2 * NAMETAG_HEIGHT_A) * 0.5f;
		m_BoardBounds = Rectangle{ 0, NAMETAG_HEIGHT_A + offset, boardSize, boardSize };
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
	//Draw player images
	DrawRectangleRounded(Rectangle{ m_BoardBounds.x, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f, PLAYER_IMAGE_SIZE_A, PLAYER_IMAGE_SIZE_A }, 0.2f, 4, m_ColorBuffer.BgFocused2);
	DrawRectangleRounded(Rectangle{ m_BoardBounds.x + 1, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f + 1, PLAYER_IMAGE_SIZE_A - 2, PLAYER_IMAGE_SIZE_A - 2 }, 0.2f, 4, m_ColorBuffer.BgFocused);
	DrawRectangleRounded(Rectangle{ m_BoardBounds.x, (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f, PLAYER_IMAGE_SIZE_A, PLAYER_IMAGE_SIZE_A }, 0.2f, 4, m_ColorBuffer.BgFocused);
	DrawRectangleRounded(Rectangle{ m_BoardBounds.x + 1, (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f + 1, PLAYER_IMAGE_SIZE_A - 2, PLAYER_IMAGE_SIZE_A - 2 }, 0.2f, 4, m_ColorBuffer.BgFocused2);

	//Draw nametags
	DrawTextEx(GameData::GameFont, m_WhiteName.c_str(), Vector2{ m_BoardBounds.x + PLAYER_IMAGE_SIZE_A + 10, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - PLAYER_IMAGE_SIZE_A) * 0.5f }, 50, 0, RAYWHITE);
	DrawTextEx(GameData::GameFont, m_BlackName.c_str(), Vector2{ m_BoardBounds.x + PLAYER_IMAGE_SIZE_A + 10, (m_BoardBounds.y - 60) * 0.5f }, 50, 0, RAYWHITE);

	//Draw board
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			//Light square
			if ((i + j) % 2 == 0)
				DrawRectangle(m_BoardBounds.x + j * m_SquareSize, m_BoardBounds.y + i * m_SquareSize, m_SquareSize, m_SquareSize, m_ColorBuffer.Light);
			//Dark square
			else
				DrawRectangle(m_BoardBounds.x + j * m_SquareSize, m_BoardBounds.y + i * m_SquareSize, m_SquareSize, m_SquareSize, m_ColorBuffer.Dark);
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
				m_Flipped ? piece.DrawFlipped(m_Pieces[(int)piece.GetType()]) : piece.Draw(m_Pieces[(int)piece.GetType()]);
		}
	}

	//Draw selected moves
	for (int i = 0; i < m_SelectedMoves.size(); i++)
		m_SelectedMoves[i].Draw(m_BoardBounds);

	//Draw dragged move
	if (m_DraggedPiece)
		m_DraggedPiece->Draw(m_Pieces[(int)m_DraggedPiece->GetType()]);

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

	for (int i = 0; i < 12; i++)
		UnloadTexture(m_Pieces[i]);
}

void IBoard::Flip()
{
	m_Flipped = !m_Flipped;

	for (int i = 0; i < m_Highlights.size(); i++)
		m_Highlights[i].SetFlip(m_Flipped);

	for (int i = 0; i < m_Arrows.size(); i++)
		m_Arrows[i].SetFlip(m_Flipped);
}

void IBoard::Reset(bool resetEnginePosition)
{
	Clear();

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

	m_BlackKing = &m_Board[0][4];
	m_WhiteKing = &m_Board[7][4];

	m_WhiteInCheck = false;
	m_BlackInCheck = false;

	m_SideToMove = 0;
	m_Flipped = false;

	m_Moves.clear();
	m_MoveIndex = -1;
	_GetLegalMoves(m_SideToMove);

	m_Result = Result::NONE;

	m_SelectedPiece = nullptr;
	m_SelectedMoves.clear();

	if (GameData::CurrentEngine && resetEnginePosition)
		GameData::CurrentEngine->SetPosition("");
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

bool IBoard::_Move(const Vector2& fromSquare, const Vector2& toSquare, bool animated)
{
	//Move should overwrite current move or should not be made
	if (m_MoveIndex != m_Moves.size() - 1)
	{
		//AnalyseMode, overwriting is allowed
		if (m_AnalyseMode)
		{
			std::string move = _ToChessNote(fromSquare) + _ToChessNote(toSquare);
			
			//Overwrite the current line if necessary
			if (move != m_Moves[m_MoveIndex + 1])
			{
				if (std::find(m_LegalMoves.begin(), m_LegalMoves.end(), move) != m_LegalMoves.end())
				{
					m_Moves.resize(m_MoveIndex + 2);
					m_Moves[m_MoveIndex + 1] = move;
					m_MoveIndex++;
				}
			}
			else
				m_MoveIndex++;

			m_SideToMove = !m_SideToMove;
			_ReloadBoard();
		}
		//Move is not made, jump to the end of the line
		else
		{
			m_MoveIndex = m_Moves.size() - 1;
			_ReloadBoard();
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
		if (std::find(m_LegalMoves.begin(), m_LegalMoves.end(), move) != m_LegalMoves.end())
		{
			_DoMove(fromSquare, toSquare, true, animated);
			_GetLegalMoves(m_SideToMove);
			GameData::CurrentEngine->SetPosition(_MovesToString());
			m_Highlights.clear();
			m_Arrows.clear();
			return true;
		}
		else return false;
	}
}

bool IBoard::_Move(const std::string& move, bool animated)
{
	Vector2 from = _ToRealSquare(move.substr(0, 2));
	Vector2 to = _ToRealSquare(move.substr(2, 2));
	return _Move(from, to, animated);
}

void IBoard::_DoMove(const Vector2& fromSquare, const Vector2& toSquare, bool registerMove, bool animated)
{
	std::string move = _ToChessNote(fromSquare) + _ToChessNote(toSquare);

	//Is the move a castle
	Piece* piece = &m_Board[(int)fromSquare.y][(int)fromSquare.x];
	if (piece->GetType() == PieceType::W_KING && std::abs(toSquare.x - fromSquare.x) == 2)
		return _CastleKing(m_WhiteKing, (toSquare.x - fromSquare.x) / 2, registerMove, animated);
	else if (piece->GetType() == PieceType::B_KING && std::abs(toSquare.x - fromSquare.x) == 2)
		return _CastleKing(m_BlackKing, (toSquare.x - fromSquare.x) / 2, registerMove, animated);

	//Check en passant
	if (fromSquare.x != toSquare.x && piece->GetType() == PieceType::W_PAWN && m_Board[(int)toSquare.y][(int)toSquare.x].GetType() == PieceType::NONE)
		m_Board[(int)toSquare.y + 1][(int)toSquare.x] = Piece(PieceType::NONE, m_Board[(int)toSquare.y + 1][(int)toSquare.x].GetPosition());
	else if (fromSquare.x != toSquare.x && piece->GetType() == PieceType::B_PAWN && m_Board[(int)toSquare.y][(int)toSquare.x].GetType() == PieceType::NONE)
		m_Board[(int)toSquare.y - 1][(int)toSquare.x] = Piece(PieceType::NONE, m_Board[(int)toSquare.y - 1][(int)toSquare.x].GetPosition());

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

	//Register if necessary
	if (registerMove)
		_RegisterMove(piece, move);

	//Update check variables
	m_WhiteInCheck = _IsAttacked(m_WhiteKing);
	m_BlackInCheck = _IsAttacked(m_BlackKing);
}

void IBoard::_DoMove(const std::string& move, bool registerMove, bool animated)
{
	Vector2 fromSquare = _ToSquare(move.substr(0, 2));
	Vector2 toSquare = _ToSquare(move.substr(2, 2));
	IBoard::_DoMove(fromSquare, toSquare, registerMove, animated);
}

bool IBoard::_TestMove(const Vector2& fromSquare, const Vector2& toSquare)
{
	//Is the move a castle
	Piece* piece = &m_Board[(int)fromSquare.y][(int)fromSquare.x];
	if (piece->GetType() == PieceType::W_KING && std::abs(toSquare.x - fromSquare.x) == 2)
		return _TestCastle(m_WhiteKing, (toSquare.x - fromSquare.x) / 2);
	else if (piece->GetType() == PieceType::B_KING && std::abs(toSquare.x - fromSquare.x) == 2)
		return _TestCastle(m_BlackKing, (toSquare.x - fromSquare.x) / 2);

	//Backup current state
	Piece backup[8][8];
	memcpy(backup, m_Board, sizeof(m_Board));
	Piece* m_WhiteKingBackup = m_WhiteKing;
	Piece* m_BlackKingBackup = m_BlackKing;

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

	//Check if the king is in check
	if ((side == 0 && _IsAttacked(m_WhiteKing)) || (side == 1 && _IsAttacked(m_BlackKing)))
	{
		memcpy(m_Board, backup, sizeof(backup));
		m_WhiteKing = m_WhiteKingBackup;
		m_BlackKing = m_BlackKingBackup;
		return false;
	}
	else
	{
		memcpy(m_Board, backup, sizeof(backup));
		m_WhiteKing = m_WhiteKingBackup;
		m_BlackKing = m_BlackKingBackup;
		return true;
	}
}

void IBoard::_CastleKing(Piece* king, int8_t direction, bool registerMove, bool animated)
{
	Vector2 square = _GetRealSquare(king->GetPosition());

	//Short castle
	if (direction == 1)
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

		//Register if necessary
		if (registerMove)
			_RegisterMove(king, _ToChessNote(Vector2{ 4, square.y }) + _ToChessNote(Vector2{ 6, square.y }));
	}
	//Long castle
	else if (direction == -1)
	{
		//Move the king in the array
		Piece& rook = m_Board[(int)square.y][7];
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

		//Register if necessary
		if (registerMove)
			_RegisterMove(king, _ToChessNote(Vector2{ 4, square.y }) + _ToChessNote(Vector2{ 2, square.y }));
	}

	//Update king pointers
	if (king->GetSide() == 0)
		m_WhiteKing = king;
	else if (king->GetSide() == 1)
		m_BlackKing = king;
}

bool IBoard::_TestCastle(Piece* king, int8_t direction)
{
	if (king->GetMoveCount() > 0) return false;
	if (king->GetSide() == 0 && m_WhiteInCheck) return false;
	else if (king->GetSide() == 1 && m_BlackInCheck) return false;

	Vector2 square = _GetRealSquare(king->GetPosition());

	//Short castle
	if (direction == 1)
	{
		Piece& rook = m_Board[(int)square.y][7];
		if ((rook.GetType() == PieceType::W_ROOK || rook.GetType() == PieceType::B_ROOK) &&
			king->GetSide() == rook.GetSide() && rook.GetMoveCount() == 0 &&
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
	else if (direction == -1)
	{
		Piece& rook = m_Board[(int)square.y][0];
		if ((rook.GetType() == PieceType::W_ROOK || rook.GetType() == PieceType::B_ROOK) &&
			king->GetSide() == rook.GetSide() && rook.GetMoveCount() == 0 &&
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
					if (square.y > 1 && m_Board[(int)square.y - 1][(int)square.x].GetType() == PieceType::NONE && m_Board[(int)square.y - 2][(int)square.x].GetType() == PieceType::NONE && piece.GetMoveCount() == 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, square.y - 2 });
					//En passant
					if (square.x < 7 && square.y > 1 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x + 1, square.y - 2 }) + _ToChessNote(Vector2{ square.x + 1, square.y }) == m_Moves[m_MoveIndex])
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y - 1 });
					if (square.x > 0 && square.y > 1 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x - 1, square.y - 2 }) + _ToChessNote(Vector2{ square.x - 1, square.y }) == m_Moves[m_MoveIndex])
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

					//Short castle
					if (piece.GetMoveCount() == 0 && m_Board[(int)square.y][7].GetType() == PieceType::W_ROOK && m_Board[(int)square.y][7].GetMoveCount() == 0 &&
						m_Board[(int)square.y][5].GetType() == PieceType::NONE && m_Board[(int)square.y][6].GetType() == PieceType::NONE)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y });

					//Long castle
					if (piece.GetMoveCount() == 0 && m_Board[(int)square.y][0].GetType() == PieceType::W_ROOK && m_Board[(int)square.y][0].GetMoveCount() == 0 &&
						m_Board[(int)square.y][1].GetType() == PieceType::NONE && m_Board[(int)square.y][2].GetType() == PieceType::NONE && m_Board[(int)square.y][3].GetType() == PieceType::NONE)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y });
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
					if (square.y < 6 && m_Board[(int)square.y + 1][(int)square.x].GetType() == PieceType::NONE && m_Board[(int)square.y + 2][(int)square.x].GetType() == PieceType::NONE && piece.GetMoveCount() == 0) pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x, square.y + 2 });
					//En passant
					if (square.x < 7 && square.y < 6 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x + 1, square.y + 2 }) + _ToChessNote(Vector2{ square.x + 1, square.y }) == m_Moves[m_MoveIndex])
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 1, square.y + 1 });
					if (square.x > 0 && square.y < 6 && m_Moves.size() > 0 && _ToChessNote(Vector2{ square.x - 1, square.y + 2 }) + _ToChessNote(Vector2{ square.x - 1, square.y }) == m_Moves[m_MoveIndex])
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

					//Short castle
					if (piece.GetMoveCount() == 0 && m_Board[(int)square.y][7].GetType() == PieceType::B_ROOK && m_Board[(int)square.y][7].GetMoveCount() == 0 &&
						m_Board[(int)square.y][5].GetType() == PieceType::NONE && m_Board[(int)square.y][6].GetType() == PieceType::NONE)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x + 2, square.y });

					//Long castle
					if (piece.GetMoveCount() == 0 && m_Board[(int)square.y][0].GetType() == PieceType::B_ROOK && m_Board[(int)square.y][0].GetMoveCount() == 0 &&
						m_Board[(int)square.y][1].GetType() == PieceType::NONE && m_Board[(int)square.y][2].GetType() == PieceType::NONE && m_Board[(int)square.y][3].GetType() == PieceType::NONE)
						pseudoLegalMoves.emplace_back(Vector2{ square.x, square.y }, Vector2{ square.x - 2, square.y });
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

void IBoard::_RegisterMove(Piece* piece, const std::string& move)
{
	m_Moves.push_back(move);
	m_MoveIndex++;
	piece->AddToMoveCount(1);
	m_SideToMove = !m_SideToMove;
}

void IBoard::_ReloadBoard()
{
	//Reset board
	std::vector<std::string> movesBackup = m_Moves;
	bool flippedBackup = m_Flipped;
	int32_t moveIndexBackup = m_MoveIndex;
	int8_t sideToMoveBackup = m_SideToMove;
	if (m_DraggedPiece)
	{
		m_DraggedPiece->SetDrag(false);
		m_DraggedPiece = nullptr;
	}
	IBoard::Reset(false);
	m_Flipped = flippedBackup;

	//Redo moves
	for (int i = 0; i < moveIndexBackup + 1; i++)
		IBoard::_DoMove(movesBackup[i], true);

	//Add back remaining moves
	for (int i = moveIndexBackup + 1; i < movesBackup.size(); i++)
		m_Moves.push_back(movesBackup[i]);

	if (!m_AnalyseMode)
		m_SideToMove = sideToMoveBackup;

	//Check winner
	_GetLegalMoves(m_SideToMove);
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

	//Clear guides
	m_Highlights.clear();
	m_Arrows.clear();

	GameData::CurrentEngine->SetPosition(_MovesToString());
}

std::string IBoard::_ToChessNote(const Vector2& square) const
{
	std::string move = "";
	move += char(97 + square.x);
	move += std::to_string(8 - (int)square.y);
	return move;
}

Vector2 IBoard::_ToSquare(const std::string& move)const
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

Vector2 IBoard::_ToRealSquare(const std::string& move)const
{
	return Vector2
	{
		(float)(move[0] - 97),
		(float)(8 - move[1] + '0'),
	};
}

Vector2 IBoard::_GetSquare(const Vector2& position) const
{
	Vector2 square{ (float)(((int)position.x - (int)m_BoardBounds.x) / (int)m_SquareSize), (float)(((int)position.y - (int)m_BoardBounds.y) / (int)m_SquareSize) };
	if (m_Flipped)
	{
		square.x = 7 - square.x;
		square.y = 7 - square.y;
	}
	return square;
}

Vector2 IBoard::_GetRealSquare(const Vector2& position) const
{
	return Vector2{ (float)(((int)position.x - (int)m_BoardBounds.x) / (int)m_SquareSize), (float)(((int)position.y - (int)m_BoardBounds.y) / (int)m_SquareSize) };
}

std::string IBoard::_MovesToString() const
{
	std::string moves = "";
	for (int i = 0; i < m_MoveIndex + 1; i++)
		moves += m_Moves[i] + " ";
	return moves;
}