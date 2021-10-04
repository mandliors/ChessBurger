#include "extras/raygui.h"
#include "GameBoard.h"
#include "GameData/GameData.h"

GameBoard::GameBoard(const ColorBuffer& colorBuffer, const Rectangle& bounds)
	: IBoard(colorBuffer, bounds), m_WhiteClock(5 * 60.0), m_BlackClock(5 * 60.0), m_ComputerThinkClock(), m_ComputerSide(1), m_ComputerThinkSeconds(1.0), m_WhiteClockBounds({}), m_BlackClockBounds({})
{
	m_AnalyseMode = false;
	m_BlackName = "Computer";
	m_ComputerThinkSeconds = (double)GetRandomValue(COMPUTER_THINK_SECONDS_MIN * 10, COMPUTER_THINK_SECONDS_MAX * 10) / 10.0;
	GameData::CurrentEngine->SetAnalyseMode(false);
	//GameData::CurrentEngine->SendCommand("setoption name UCI_LimitStrength value true");
	//GameData::CurrentEngine->SendCommand("setoption name UCI_Elo value 300");
	GameData::CurrentEngine->Start();
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
			m_WhiteClock.Start();
			m_BlackClock.Pause();
			if (m_WhiteClock.GetSecondsLeft() <= 0)
			{
				m_WhiteClock.Pause();
				m_Result = Result::BLACK_WIN;
			}
		}
		//Black moves, check clock time
		else
		{
			m_BlackClock.Start();
			m_WhiteClock.Pause();
			if (m_BlackClock.GetSecondsLeft() <= 0)
			{
				m_BlackClock.Pause();
				m_Result = Result::WHITE_WIN;
			}
		}

		//Computer should move
		if (m_SideToMove == m_ComputerSide)
		{
			m_ComputerThinkClock.SetMaxSeconds(m_ComputerThinkSeconds);
			m_ComputerThinkClock.Start();
			if (m_ComputerThinkClock.GetSecondsLeft() <= 0.0)
			{
				_Move(GameData::CurrentEngine->GetAnalysisData().BestLines[0][0]);
				m_ComputerThinkClock.Pause();
				m_ComputerThinkClock.Reset();
				m_ComputerThinkSeconds = (double)GetRandomValue(COMPUTER_THINK_SECONDS_MIN * 10, COMPUTER_THINK_SECONDS_MAX * 10) / 10.0;
			}
		}

		//Update clocks
		m_WhiteClock.Update();
		m_BlackClock.Update();
		m_ComputerThinkClock.Update();
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
	DrawRectangleRounded(m_WhiteClockBounds, 0.2f, 4, m_ColorBuffer.BgFocused);
	std::string clockString = m_WhiteClock.GetString();
	int width = MeasureTextEx(GameData::GameFont, clockString.c_str(), CLOCK_TEXT_SIZE_A, 0).x;
	DrawTextEx(GameData::GameFont, clockString.c_str(), Vector2{ m_WhiteClockBounds.x + m_WhiteClockBounds.width - width - 10, m_WhiteClockBounds.y + (m_WhiteClockBounds.height - CLOCK_TEXT_SIZE_A) * 0.5f }, CLOCK_TEXT_SIZE_A, 0, m_ColorBuffer.BgFocused2);
	
	DrawRectangleRounded(m_BlackClockBounds, 0.2f, 4, m_ColorBuffer.BgFocused2);
	clockString = m_BlackClock.GetString();
	width = MeasureTextEx(GameData::GameFont, clockString.c_str(), CLOCK_TEXT_SIZE_A, 0).x;
	DrawTextEx(GameData::GameFont, clockString.c_str(), Vector2{ m_BlackClockBounds.x + m_BlackClockBounds.width - width - 10, m_BlackClockBounds.y + (m_BlackClockBounds.height - CLOCK_TEXT_SIZE_A) * 0.5f }, CLOCK_TEXT_SIZE_A, 0, m_ColorBuffer.BgFocused);
	
	//Make clock look disabled
	if (m_SideToMove == 0)
		DrawRectangleRounded(m_BlackClockBounds, 0.2f, 4, Fade(BLACK, 0.5f));
	else
		DrawRectangleRounded(m_WhiteClockBounds, 0.2f, 4, Fade(BLACK, 0.5f));
		
}

void GameBoard::Reset(bool resetEnginePosition)
{
	IBoard::Reset(resetEnginePosition);
	m_WhiteClock.Pause();
	m_BlackClock.Pause();
	m_WhiteClock.Reset();
	m_BlackClock.Reset();
}