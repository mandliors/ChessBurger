#pragma once

#include "IBoard.h"
#include "UI/UIText.h"
#include "Analysis/AnalyzedGame.h"
#include "Analysis/AnalyzedMove.h"

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

//EvalBar definitions
#define EVALBAR_ANIMATION_SECONDS (1.0f / (float)UPDATES_PER_SEC)

//Other definitions
#define UPDATES_PER_SEC 2
#define SWITCH_BORDER_WIDTH 1
#define SWITCH_ROUNDNESS 0.4f

enum class AnalysisState
{
	NONE = 0, IN_PROGRESS, DONE
};

class Game;

class AnalysisBoard : public IBoard
{
public:
	AnalysisBoard(const Rectangle& bounds, Game* owner);
	~AnalysisBoard();

	void Update() override;
	void Draw() const override;
	void _UpdateBounds() override;

	void Reset(bool resetEnginePosition = true) override;

protected:
	bool _Move(const std::string& move, bool animated = false, bool updateEnginePosition = true) override;
	bool _GuiSwitch(const Rectangle& bounds, bool value, bool draw) const;
	
private:
	void _AnalyzeGame();

	//UI functions
	void EvalBar_Update();
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
	bool m_EngineWorking;

	//EvalBar
	Rectangle m_EvalBar_Bounds;
	double m_StartTime;
	float m_PreviousHeight;
	float m_Height;
	float m_NextHeight;

	//Movelist
	Rectangle m_Movelist_Bounds;
	Rectangle m_MovelistContent_Bounds;
	Vector2 m_Movelist_Scroll;
	bool m_UpdateScrollbar;

	//BestLines
	Rectangle m_BestLines_Bounds;
	std::vector<std::vector<UIText>> m_BestLines_UITexts;

	//Analysis
	AnalysisState m_AnalysisState;
	AnalyzedGame m_AnalyzedGame;
	AnalyzedMove m_AnalysisBestMove;
	int m_AnalysisMoveIndex;
};