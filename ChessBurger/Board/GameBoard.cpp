#include "extras/raygui.h"
#include "GameBoard.h"
#include "Game/Game.h"

GameBoard::GameBoard(const Rectangle& bounds, Game* owner)
	: IBoard(bounds, owner), m_WhiteClock(5 * 60.0), m_BlackClock(5 * 60.0), m_ComputerSide(1), m_WhiteClockBounds({}), m_BlackClockBounds({})
{
	m_WhiteName = "Player";
	m_BlackName = "Computer";
	m_ShowNametag = true;
	//GameData::CurrentEngine->SendCommand("setoption name UCI_LimitStrength value true");
	//GameData::CurrentEngine->SendCommand("setoption name UCI_Elo value 1350");
}

void GameBoard::Update()
{
	IBoard::Update();

	//Update timers
	if (m_Moves.size() > 0)
	{
		//White moves, check clock time
		if (m_SideToMove == 0)
		{
			if (m_WhiteClock.GetSecondsLeft() <= 0)
			{
				m_WhiteClock.Pause();
				m_Result = Result::BLACK_WIN;
			}
			else
			{
				m_WhiteClock.Start();
				m_BlackClock.Pause();
			}
		}
		//Black moves, check clock time
		else if (m_SideToMove == 1)
		{
			if (m_BlackClock.GetSecondsLeft() <= 0)
			{
				m_BlackClock.Pause();
				m_Result = Result::WHITE_WIN;
			}
			else
			{
				m_BlackClock.Start();
				m_WhiteClock.Pause();
			}
		}
		//Computer should move
		if (m_SideToMove == m_ComputerSide)
		{
			std::string& bestMove = GameData::CurrentEngine->GetBestMove();
			if (bestMove != "")
			{
				if (!IBoard::_Move(bestMove, true))
					std::invalid_argument("Computer made an illegal move");
				bestMove = "";
			}
		}
		//Update clocks
		m_WhiteClock.Update();
		m_BlackClock.Update();
	}
}

void GameBoard::UpdateBounds()
{
	IBoard::UpdateBounds();

	//Update timer bounds
	m_WhiteClockBounds = Rectangle{ m_BoardBounds.x + m_BoardBounds.width - m_BoardBounds.width * CLOCK_WIDTH_P, m_BoardBounds.y + m_BoardBounds.height + (m_BoardBounds.y - CLOCK_HEIGHT_A) * 0.5f, m_BoardBounds.width * CLOCK_WIDTH_P, CLOCK_HEIGHT_A };
	m_BlackClockBounds = Rectangle{ m_BoardBounds.x + m_BoardBounds.width - m_BoardBounds.width * CLOCK_WIDTH_P, (m_BoardBounds.y - CLOCK_HEIGHT_A) * 0.5f, m_BoardBounds.width * CLOCK_WIDTH_P, CLOCK_HEIGHT_A };
}

void GameBoard::Draw() const
{
	IBoard::Draw();

	//Draw timers
	DrawRectangleRounded(m_WhiteClockBounds, 0.2f, 4, GameData::Colors.BgFocused);
	std::string clockString = m_WhiteClock.GetString();
	int width = MeasureTextEx(GameData::MainFont, clockString.c_str(), CLOCK_TEXT_SIZE_A, 0).x;
	DrawTextEx(GameData::MainFont, clockString.c_str(), Vector2{ m_WhiteClockBounds.x + m_WhiteClockBounds.width - width - 10, m_WhiteClockBounds.y + (m_WhiteClockBounds.height - CLOCK_TEXT_SIZE_A) * 0.5f }, CLOCK_TEXT_SIZE_A, 0, GameData::Colors.BgFocused2);
	
	DrawRectangleRounded(m_BlackClockBounds, 0.2f, 4, GameData::Colors.BgFocused2);
	clockString = m_BlackClock.GetString();
	width = MeasureTextEx(GameData::MainFont, clockString.c_str(), CLOCK_TEXT_SIZE_A, 0).x;
	DrawTextEx(GameData::MainFont, clockString.c_str(), Vector2{ m_BlackClockBounds.x + m_BlackClockBounds.width - width - 10, m_BlackClockBounds.y + (m_BlackClockBounds.height - CLOCK_TEXT_SIZE_A) * 0.5f }, CLOCK_TEXT_SIZE_A, 0, GameData::Colors.BgFocused);
	
	//Make clock look disabled
	if (m_SideToMove == 0)
		DrawRectangleRounded(m_BlackClockBounds, 0.2f, 4, Fade(BLACK, 0.5f));
	else
		DrawRectangleRounded(m_WhiteClockBounds, 0.2f, 4, Fade(BLACK, 0.5f));
}

bool GameBoard::_Move(const Vector2& fromSquare, const Vector2& toSquare, bool animated, bool updateEnginePosition)
{
	if (IBoard::_Move(fromSquare, toSquare, animated))
	{
		GameData::CurrentEngine->SearchMove(m_WhiteClock.GetSecondsLeft() * 1000, m_BlackClock.GetSecondsLeft() * 1000, 0, 0);
		return true;
	}
	else return false;
}

void GameBoard::Reset(bool resetEnginePosition)
{
	IBoard::Reset(resetEnginePosition);
	m_WhiteClock.Pause();
	m_BlackClock.Pause();
	m_WhiteClock.Reset();
	m_BlackClock.Reset();
}