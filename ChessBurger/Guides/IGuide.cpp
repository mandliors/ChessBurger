#include "IGuide.h"

uint32_t IGuide::Size;

IGuide::IGuide()
	: m_Position(Vector2{ 0, 0 }), m_Color(Color{ 0, 0, 0, 0 }), m_Square(Vector2{ 0, 0 }), m_Flip(false) { }

IGuide::IGuide(const Vector2& position, const Vector2& square, const Color& color, bool flip)
	: m_Position(position), m_Color(color), m_Square(square), m_Flip(flip) { }

Vector2 IGuide::GetSquare() const
{
	return m_Square;
}

void IGuide::SetPosition(const Vector2& position)
{
	m_Position = position;
}

void IGuide::SetFlip(bool flip)
{
	m_Flip = flip;
}

const bool IGuide::operator==(const IGuide& other) const
{
	return m_Position.x == other.m_Position.x && m_Position.y == other.m_Position.y;
}