#pragma once

#include "IBoard.h"
#include "UI/UIText.h"

//Positional definitions
#define MOVELIST_WIDTH_P 0.6f
#define EVALBAR_WIDTH_A 20
#define DISTANCE_EB 10
#define DISTANCE_BM 20
#define BESTLINES_HEIGHT_P 0.25f

//Movelist definitions
#define MOVELIST_PADDING 10
#define MOVELIST_SPACING 6
#define MOVELIST_UITEXT_SIZE 24

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
	void Reset(bool resetEnginePosition= true) override;

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
	std::vector<UIText> m_Movelist_UITexts;

	//BestLines
	Rectangle m_BestLines_Bounds;
	std::vector<std::vector<UIText>> m_BestLines_UITexts;
};