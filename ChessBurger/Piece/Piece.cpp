#include "Piece.h"
#include "GameData/GameData.h"
#include "extras/easings.h"

uint32_t Piece::Size;

Piece::Piece(PieceType type, const Vector2& position)
	: m_Type(type), m_Position(position), m_PreviousPosition(position), m_NextPosition(position), m_MoveCount(0), m_BoardBounds(Rectangle{ -1, -1, -1, -1 }), m_StartTime(0.0), m_CurrentTime(0.0), m_Drag(false), m_Animating(false) { }

Piece::Piece()
	: m_Type(PieceType::NONE), m_Position(Vector2{ 0, 0 }), m_PreviousPosition(Vector2{ 0, 0 }), m_NextPosition(Vector2{ 0, 0 }), m_MoveCount(0), m_BoardBounds(Rectangle{ -1, -1, -1, -1 }), m_StartTime(0.0), m_CurrentTime(0), m_Drag(false), m_Animating(false) { }

void Piece::Update()
{
	if (m_Drag)
	{
		Vector2 mousePosition = GetMousePosition();
		m_Position.x = mousePosition.x - Size / 2;
		m_Position.y = mousePosition.y - Size / 2;
	}
	else if (m_Animating)
	{
		m_CurrentTime = GetTime();

		//Still animating
		if (m_CurrentTime - m_StartTime < ANIMATION_SECONDS)
		{
			m_Position.x = EaseLinearIn(m_CurrentTime - m_StartTime, m_PreviousPosition.x, m_NextPosition.x - m_PreviousPosition.x, ANIMATION_SECONDS);
			m_Position.y = EaseLinearIn(m_CurrentTime - m_StartTime, m_PreviousPosition.y, m_NextPosition.y - m_PreviousPosition.y, ANIMATION_SECONDS);
		}
		//Animation ends
		else
			StopAnimation();
	}
}

void Piece::Draw(const Texture2D& texture) const
{
	float scale = (float)Size / (float)texture.width;
	DrawTextureEx(texture, m_Position, 0.0f, scale, WHITE);
}

void Piece::DrawFlipped(const Texture2D& texture) const
{
	float squareSize = m_BoardBounds.width / 8.0f;
	float pieceOffset = squareSize - Size;
	float xOffset = 2 * m_BoardBounds.x - squareSize + pieceOffset;
	float yOffset = 2 * m_BoardBounds.y - squareSize + pieceOffset;

	Vector2 pos =
	{
		m_BoardBounds.width - m_Position.x + xOffset,
		m_BoardBounds.height - m_Position.y + yOffset
	};
	float scale = (float)Size / (float)texture.width;
	DrawTextureEx(texture, pos, 0.0f, scale, WHITE);
}

bool Piece::GetDrag() const
{
	return m_Drag;
}

void Piece::SetDrag(bool drag)
{
	m_Drag = drag;
	if (!drag)
		m_Position = m_PreviousPosition;
}

PieceType Piece::GetType() const
{
	return m_Type;
}

void Piece::SetType(PieceType type)
{
	m_Type = type;
}

Vector2 Piece::GetPosition() const
{
	return m_Position;
}

void Piece::SetPosition(const Vector2& position)
{
	m_Position = position;
	m_PreviousPosition = m_Position;
}

void Piece::StartAnimation(const Vector2& position)
{
	if (GameData::SupportAnimations)
	{
		m_PreviousPosition = m_Position;
		m_NextPosition = position;
		m_Animating = true;
		m_StartTime = GetTime();
	}
	else
		SetPosition(position);
}

void Piece::StopAnimation()
{
	m_Animating = false;
	m_Position = m_NextPosition;
	m_PreviousPosition = m_Position;
}

Vector2 Piece::GetPreviousPosition() const
{
	return m_PreviousPosition;
}

void Piece::TellBoardBounds(const Rectangle& boardBounds)
{
	m_BoardBounds = boardBounds;
}

void Piece::AddToMoveCount(int amount)
{
	m_MoveCount += amount;
}

uint32_t Piece::GetMoveCount() const
{
	return m_MoveCount;
}

int8_t Piece::GetSide() const
{
	if (m_Type == PieceType::NONE) return -1;
	else if ((int)m_Type < 6) return 0;
	else return 1;
}

bool Piece::OverlapPoint(const Vector2& point) const
{
	Rectangle bounds{ m_Position.x, m_Position.y, Size, Size };
	return CheckCollisionPointRec(point, bounds);
}