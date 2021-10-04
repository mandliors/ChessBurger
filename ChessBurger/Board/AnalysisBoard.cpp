#include "extras/raygui.h"
#include "AnalysisBoard.h"
#include "Game/Game.h"
#include "Utilities/Utilities.h"

AnalysisBoard::AnalysisBoard(const ColorBuffer& colorBuffer, const Rectangle& bounds)
	: IBoard(colorBuffer, bounds), m_BestMoveArrow({}), EvalBar_Bounds({}), Movelist_Bounds({}), Movelist_UITexts({}), BestLines_Bounds({}), BestLines_UITexts({})
{
	m_AnalyseMode = true;
	GameData::CurrentEngine->SetAnalyseMode(true);
	GameData::CurrentEngine->Start();
}
AnalysisBoard::~AnalysisBoard()
{
	IBoard::~IBoard();
}
void AnalysisBoard::Update()
{
	IBoard::Update();

	//Set cursor
	m_PointingHand = m_PointingHand || Movelist_CheckCursor() || BestLines_CheckCursor();

	//Update best move arrow
	std::lock_guard<std::mutex> lock(GameData::EngineMutex);
	if (GameData::CurrentEngine->GetAnalysisData().BestLines.size() > 0 && GameData::CurrentEngine->GetAnalysisData().BestLines[0].size() > 0)
	{
		std::string bestMove = GameData::CurrentEngine->GetAnalysisData().BestLines[0][0];
		Vector2 fromSquare = _ToRealSquare(bestMove.substr(0, 2));
		Vector2 fromPosition = Vector2{ (fromSquare.x + 0.5f) * m_SquareSize + m_BoardBounds.x, (fromSquare.y + 0.5f) * m_SquareSize + m_BoardBounds.y };
		Vector2 toSquare = _ToRealSquare(bestMove.substr(2, 2));
		Vector2 toPosition = Vector2{ (toSquare.x + 0.5f) * m_SquareSize + m_BoardBounds.x, (toSquare.y + 0.5f) * m_SquareSize + m_BoardBounds.y };
		m_BestMoveArrow = Arrow(fromPosition, toPosition, fromSquare, toSquare, Color{ 0, 143, 21, 180 }, m_Flipped);
	}

	//Update UI elements
	Movelist_Update();
	BestLines_Update();
}

void AnalysisBoard::UpdateBounds()
{
	//Try to match height
	float boardSize = GetScreenHeight() - 2 * NAMETAG_HEIGHT_A;
	float totalWidth = EVALBAR_WIDTH_A + DISTANCE_EB + boardSize + DISTANCE_BM + boardSize * MOVELIST_WIDTH_P;
	//Match height
	if (totalWidth < GetScreenWidth())
	{
		float offset = (GetScreenWidth() - totalWidth) * 0.5f;
		EvalBar_Bounds = Rectangle{ offset, NAMETAG_HEIGHT_A, EVALBAR_WIDTH_A, boardSize };
		m_BoardBounds = Rectangle{ EvalBar_Bounds.x + EVALBAR_WIDTH_A + DISTANCE_EB, NAMETAG_HEIGHT_A, boardSize, boardSize };
		BestLines_Bounds = Rectangle{ m_BoardBounds.x + boardSize + DISTANCE_BM, NAMETAG_HEIGHT_A, boardSize * MOVELIST_WIDTH_P, boardSize * BESTLINES_HEIGHT_P };
		Movelist_Bounds = Rectangle{ BestLines_Bounds.x, BestLines_Bounds.y + BestLines_Bounds.height, BestLines_Bounds.width, boardSize - BestLines_Bounds.height };
	}
	//Match width
	else
	{
		boardSize = (GetScreenWidth() - EVALBAR_WIDTH_A - DISTANCE_EB - DISTANCE_BM) / (1.0f + MOVELIST_WIDTH_P);
		float offset = (GetScreenHeight() - boardSize - 2 * NAMETAG_HEIGHT_A) * 0.5f;
		EvalBar_Bounds = Rectangle{ 0, offset + NAMETAG_HEIGHT_A, EVALBAR_WIDTH_A, boardSize };
		m_BoardBounds = Rectangle{ EVALBAR_WIDTH_A + DISTANCE_EB, offset + NAMETAG_HEIGHT_A, boardSize, boardSize };
		BestLines_Bounds = Rectangle{ m_BoardBounds.x + boardSize + DISTANCE_BM, offset + NAMETAG_HEIGHT_A, boardSize * MOVELIST_WIDTH_P, boardSize * BESTLINES_HEIGHT_P };
		Movelist_Bounds = Rectangle{ BestLines_Bounds.x, BestLines_Bounds.y + BestLines_Bounds.height, BestLines_Bounds.width, boardSize - BestLines_Bounds.height };
	}
	m_SquareSize = boardSize / 8.0f;

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

void AnalysisBoard::Draw() const
{
	IBoard::Draw();

	//Draw best move arrow
	m_BestMoveArrow.Draw(m_BoardBounds);

	//Draw UI elements
	std::lock_guard<std::mutex> lock(GameData::EngineMutex);
	EvalBar_Draw();
	Movelist_Draw();
	BestLines_Draw();

	//Temporary texts
	DrawTextEx(GameData::GameFont, std::to_string(GameData::CurrentEngine->GetAnalysisData().Depth).c_str(), Vector2{ 10, 10 }, 30, 0, RAYWHITE);
}

void AnalysisBoard::Reset(bool resetEnginePosition)
{
	IBoard::Reset(resetEnginePosition);
	Movelist_UITexts.clear();
	BestLines_UITexts.clear();
}

void AnalysisBoard::EvalBar_Draw() const
{
	std::string eval = GameData::CurrentEngine->GetAnalysisData().Evaluations[0];
	float blackSize;

	if (eval.find("M") != -1)
		blackSize = eval.find("+") != -1 ? 0 : m_BoardBounds.height;
	else
		blackSize = Math::Map(stof(eval), -5, 5, m_BoardBounds.height, 0);

	blackSize = Math::Clamp(blackSize, 0, EvalBar_Bounds.height);
	DrawRectangle(EvalBar_Bounds.x, EvalBar_Bounds.y, EvalBar_Bounds.width, blackSize, m_ColorBuffer.BgNormal);
	DrawRectangle(EvalBar_Bounds.x, EvalBar_Bounds.y + blackSize, EvalBar_Bounds.width, EvalBar_Bounds.height - blackSize, m_ColorBuffer.FgNormal);
}

void AnalysisBoard::Movelist_Update()
{
	//Update UITexts
	if (m_Moves.size() != Movelist_UITexts.size())
		Movelist_UITexts.resize(m_Moves.size());

	for (int i = 0; i < Movelist_UITexts.size(); i++)
		Movelist_UITexts[i].SetState(UIState::NORMAL);

	if (m_Moves.size() > 0)
		Movelist_UITexts[m_MoveIndex].SetState(UIState::FOCUSED);

	int xOffset = 0;
	int yOffset = MOVELIST_PADDING;
	Vector2 mousePos = GetMousePosition();
	for (int i = 0; i < Movelist_UITexts.size(); i++)
	{
		//Set size, text and position
		UIText& text = Movelist_UITexts[i];
		text.SetTextSize(MOVELIST_UITEXT_SIZE);
		text.SetText(m_Moves[i]);
		float w = text.GetBounds().width;
		if (xOffset + w > Movelist_Bounds.width)
		{
			xOffset = MOVELIST_PADDING;
			yOffset += text.GetBounds().height + MOVELIST_SPACING;
		}
		else
			xOffset += MOVELIST_SPACING;
		text.SetPosition(Vector2{ Movelist_Bounds.x + xOffset, Movelist_Bounds.y + yOffset });
		xOffset += w;

		//Set state
		if (CheckCollisionPointRec(mousePos, text.GetBounds()))
		{
			if (text.GetState() != UIState::FOCUSED)
			{
				if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && i != m_MoveIndex)
				{
					if (CheckCollisionPointRec(m_MouseDownPosition, text.GetBounds()))
					{
						text.SetState(UIState::FOCUSED);
						m_MouseDownPosition = Vector2{ -1, -1 };
						Movelist_UITexts[m_MoveIndex].SetState(UIState::NORMAL);
						m_MoveIndex = i;
						_ReloadBoard();
					}
				}
				else
					text.SetState(UIState::HOVERED);
			}
		}
	}
}

void AnalysisBoard::Movelist_Draw() const
{
	DrawRectangle(Movelist_Bounds.x, Movelist_Bounds.y, Movelist_Bounds.width, Movelist_Bounds.height, m_ColorBuffer.BgNormal);
	for (int i = 0; i < Movelist_UITexts.size(); i++)
		Movelist_UITexts[i].Draw();
}

bool AnalysisBoard::Movelist_CheckCursor() const
{
	Vector2 mousePos = GetMousePosition();
	for (int i = 0; i < Movelist_UITexts.size(); i++)
		if (CheckCollisionPointRec(mousePos, Movelist_UITexts[i].GetBounds()))
			return true;
	return false;
}

void AnalysisBoard::BestLines_Update()
{
	//Resize the vectors
	const Engine::AnalysisData& analysisData = GameData::CurrentEngine->GetAnalysisData();
	BestLines_UITexts.resize(analysisData.BestLines.size());
	for (int i = 0; i < BestLines_UITexts.size(); i++)
		BestLines_UITexts[i].resize(analysisData.BestLines[i].size() + 1);

	//Add the evaluations to the beginning
	Vector2 mousePos = GetMousePosition();
	bool reload = false;
	for (int i = 0; i < BestLines_UITexts.size(); i++)
	{
		UIText& eval = BestLines_UITexts[i][0];
		eval.SetTextSize(BESTLINES_UITEXT_SIZE * BESTLINES_UITEXT_SIZE_RATIO);
		eval.SetText(analysisData.Evaluations[i]);
		eval.SetPosition(Vector2{ BestLines_Bounds.x + BESTLINES_PADDING, BestLines_Bounds.y + BESTLINES_PADDING + i * (eval.GetBounds().height + BESTLINES_SPACING) });
		if (analysisData.Evaluations[i].find("+") != -1)
			eval.SetState(UIState::FOCUSED);
		else
			eval.SetState(UIState::FOCUSED2);

		if (CheckCollisionPointRec(mousePos, eval.GetBounds()))
		{
			if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
			{
				if (CheckCollisionPointRec(m_MouseDownPosition, eval.GetBounds()) && analysisData.BestLines[i].size() > 0)
				{
					m_MouseDownPosition = Vector2{ -1, -1 };
					_Move(analysisData.BestLines[i][0]);
					reload = true;
				}
			}
		}
	}

	//Update and add best lines
	for (int i = 0; i < BestLines_UITexts.size(); i++)
	{
		int xOffset = BESTLINES_SPACING + BestLines_UITexts[i][0].GetBounds().width + BESTLINES_SPACING;
		int screenWidth = GetScreenWidth();
		int yOffset = BESTLINES_PADDING + i * (BestLines_UITexts[i][0].GetBounds().height + BESTLINES_SPACING) + (BESTLINES_UITEXT_SIZE_RATIO - 1.0f) * BestLines_UITexts[i][0].GetBounds().height * 0.5f;
		for (int j = 1; j < BestLines_UITexts[i].size(); j++)
		{
			//Set size, text and position
			UIText& text = BestLines_UITexts[i][j];
			text.SetTextSize(BESTLINES_UITEXT_SIZE);
			text.SetText(analysisData.BestLines[i][j - 1]);
			float w = text.GetBounds().width;

			//Move it off the screen if it does not fit onto the bestlines area
			if (xOffset + w > BestLines_Bounds.width)
				xOffset = screenWidth + 10;
			else
				xOffset += BESTLINES_SPACING;
			text.SetPosition(Vector2{ BestLines_Bounds.x + xOffset, BestLines_Bounds.y + yOffset });
			xOffset += w;

			if (CheckCollisionPointRec(mousePos, BestLines_UITexts[i][j].GetBounds()))
			{
				if (BestLines_UITexts[i][j].GetState() != UIState::FOCUSED)
				{
					if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
					{
						//Click happened
						if (CheckCollisionPointRec(m_MouseDownPosition, BestLines_UITexts[i][j].GetBounds()))
						{
							m_MouseDownPosition = Vector2{ -1, -1 };
							for (int x = 0; x < j; x++)
								_Move(analysisData.BestLines[i][x]);
							reload = true;
						}
					}
					else
						BestLines_UITexts[i][j].SetState(UIState::HOVERED);
				}
			}
			else if (BestLines_UITexts[i][j].GetState() != UIState::FOCUSED)
				BestLines_UITexts[i][j].SetState(UIState::NORMAL);
		}

		if (reload)
			_ReloadBoard();
	}
}

void AnalysisBoard::BestLines_Draw() const
{
	DrawRectangle(BestLines_Bounds.x, BestLines_Bounds.y, BestLines_Bounds.width, BestLines_Bounds.height, m_ColorBuffer.BgNormal);
	for (int i = 0; i < BestLines_UITexts.size(); i++)
		for (int j = 0; j < BestLines_UITexts[i].size(); j++)
			BestLines_UITexts[i][j].Draw();
}

bool AnalysisBoard::BestLines_CheckCursor() const 
{
	Vector2 mousePos = GetMousePosition();
	for (int i = 0; i < BestLines_UITexts.size(); i++)
		for (int j = 0; j < BestLines_UITexts[i].size(); j++)
			if (CheckCollisionPointRec(mousePos, BestLines_UITexts[i][j].GetBounds()))
				return true;
	return false;
}