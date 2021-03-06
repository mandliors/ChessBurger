#pragma once

#include "IBoard.h"
#include "UI/UIText.h"

//Positional definitions
#define MOVELIST_WIDTH_P 0.6f
#define EVALBAR_WIDTH 20
#define DISTANCE_EB 10
#define DISTANCE_BM 20

//Movelist definitions
#define MOVELIST_PADDING 10
#define MOVELIST_SPACING 6
#define MOVELIST_UITEXT_SIZE 24
#define MOVELIST_MOVE_HEIGHT 30
#define MOVELIST_MOVENUMBER_WIDTH 60

//BestLines definitions
#define BESTLINES_PADDING 10
#define BESTLINES_SPACING 6
#define BESTLINES_UITEXT_SIZE 18
#define BESTLINES_UITEXT_SIZE_RATIO 1.2f
#define BESTLINES_UPDATES_PER_SEC 30

class Game;

class AnalysisBoard : public IBoard
{
public:
	AnalysisBoard(const Rectangle& bounds, Game* owner);
	~AnalysisBoard();

	void Update() override;
	void UpdateBounds() override;
	void Draw() const override;
	void Reset(bool resetEnginePosition = true) override;

protected:
	bool _Move(const Vector2& fromSquare, const Vector2& toSquare, bool animated = false, bool updateEnginePosition = true) override;

private:
	//UI functions
	void EvalBar_Draw() const;

	void Movelist_Update();
	void Movelist_Draw() const;
	bool Movelist_CheckCursor() const;

	void BestLines_Update();
	void BestLines_Draw() const;
	bool BestLines_CheckCursor() const;

private:
	Arrow m_BestMoveArrow;
	uint32_t m_FrameCounter;

	//EvalBar
	Rectangle m_EvalBar_Bounds;

	//Movelist
	Rectangle m_Movelist_Bounds;
	Rectangle m_MovelistContent_Bounds;
	Vector2 m_Movelist_Scroll;
	bool m_UpdateScrollbar;

	//BestLines
	Rectangle m_BestLines_Bounds;
	std::vector<std::vector<UIText>> m_BestLines_UITexts;
};