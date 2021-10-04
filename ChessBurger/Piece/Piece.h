#pragma once

#include "raylib.h"
#include <memory>

#define ANIMATION_SECONDS 0.2f

enum class PieceType
{
	NONE = -1, W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
			   B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING
};

class Piece
{
public:
	Piece();

	Piece(PieceType type, const Vector2& position);

	void Update();
	void Draw(const Texture2D& texture) const;
	void DrawFlipped(const Texture2D& texture) const;
	bool GetDrag() const;
	void SetDrag(bool drag);
	PieceType GetType() const;
	void SetType(PieceType type);
	Vector2 GetPosition() const;
	void SetPosition(const Vector2& position);
	void StartAnimation(const Vector2& position);
	void StopAnimation();
	Vector2 GetPreviousPosition() const;
	void TellBoardBounds(const Rectangle& boardBounds);
	void AddToMoveCount(int amount);
	uint32_t GetMoveCount() const;
	int8_t GetSide() const;
	bool OverlapPoint(const Vector2& point) const;

public:
	static uint32_t Size;

private:
	PieceType m_Type;
	Vector2 m_Position;
	Vector2 m_PreviousPosition;
	Vector2 m_NextPosition;
	uint32_t m_MoveCount;
	Rectangle m_BoardBounds;
	double m_StartTime;
	double m_CurrentTime;
	bool m_Drag;
	bool m_Animating;
};