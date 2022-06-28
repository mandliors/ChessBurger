#pragma once

#include "IBoard.h"
#include "Clock/Clock.h"

#define CLOCK_WIDTH_P 3.0f/16.0f
#define CLOCK_HEIGHT_A (NAMETAG_HEIGHT_A - 10)
#define CLOCK_TEXT_SIZE_A (CLOCK_HEIGHT_A - 10)

#define COMPUTER_THINK_SECONDS_MIN 0.5
#define COMPUTER_THINK_SECONDS_MAX 3.0

class Game;

class GameBoard : public IBoard
{
public:
	GameBoard(const Rectangle& bounds, Game* owner);

	void Update() override;
	void UpdateBounds() override;
	void Draw() const override;

	virtual void Reset(bool resetEnginePosition = true) override;

protected:
	bool _Move(const Vector2& fromSquare, const Vector2& toSquare, bool animated = false, bool updateEnginePosition = true) override;

private:
	Clock m_WhiteClock;
	Clock m_BlackClock;
	int8_t m_ComputerSide;
	Rectangle m_WhiteClockBounds;
	Rectangle m_BlackClockBounds;
};