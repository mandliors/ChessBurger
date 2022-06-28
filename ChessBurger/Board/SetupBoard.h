#pragma once

#include "IBoard.h"

//Positional definitions
#define SIDEPANEL_WIDTH_P 0.6f
#define SIDEPANEL_MAX_WIDTH_A 300.0f
#define DISTANCE_BS 5
#define DISTANCE_BL 5
#define UI_OFFSET 5
#define UI_HEIGHT_A 40.0f

class Game;

class SetupBoard : public IBoard
{
public:
	SetupBoard(const Rectangle& bounds, Game* owner);
	~SetupBoard();

	void Update() override;
	void UpdateBounds() override;
	void Draw() const override;
	void Reset(bool resetEnginePosition = true) override;

protected:
	void SetupPieces_Update();
	void SetupPieces_Draw() const;
	bool SetupPieces_CheckCursor() const;

	void SidePanel_UpdateAndDraw();
	bool SidePanel_CheckCursor() const;

	std::string _GetFEN() const override;
	std::string _GetPGN() const override;

private:
	float m_SetupWhitePiecesY;
	float m_SetupBlackPiecesY;
	float m_SetupPiecesWidth;
	float m_SetupPiecesHeight;
	Rectangle m_SidePanelBounds;
	PieceType m_SelectedPieceType;
	bool m_Delete;
	bool m_DragSetupPiece;
	float m_UIWidth;
	int m_SideToPlay;
	bool m_SideToPlayEditMode;
};