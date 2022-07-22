#pragma once

#include "raylib.h"
#include <memory>

#define ANIMATION_SECONDS 0.2f

enum class PieceType
{
	NONE = -1, W_KING, W_QUEEN, W_BISHOP, W_KNIGHT, W_ROOK, W_PAWN,
			   B_KING, B_QUEEN, B_BISHOP, B_KNIGHT, B_ROOK, B_PAWN, COUNT
};

class Piece
{
public:
	Piece();

	Piece(PieceType type, const Vector2& position);

	void Update();
	void Draw() const;
	void DrawFlipped() const;

	bool GetDrag() const;
	void SetDrag(bool drag);

	PieceType GetType() const;
	void SetType(PieceType type);

	Vector2 GetPosition() const;
	void SetPosition(const Vector2& position);
	Vector2 GetPreviousPosition() const;

	void StartAnimation(const Vector2& position);
	void StopAnimation();

	void SetHasMoved(bool value);
	bool GetHasMoved() const;

	int8_t GetSide() const;
	void TellBoardBounds(const Rectangle& boardBounds);
	bool OverlapPoint(const Vector2& point) const;

public:
	static uint32_t Size;

private:
	PieceType m_Type;
	Vector2 m_Position;
	Vector2 m_PreviousPosition;
	Vector2 m_NextPosition;
	Rectangle m_BoardBounds;
	double m_StartTime;
	double m_CurrentTime;
	bool m_HasMoved;
	bool m_Drag;
	bool m_Animating;
};