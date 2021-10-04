#pragma once

#include "raylib.h"
#include "IGuide.h"
#include <memory>

class Arrow : public IGuide
{
public:
	Arrow()
		: IGuide(Vector2{ 0, 0 }, Vector2{ 0, 0 }, Color{ 0, 0, 0, 0 }), m_Square2(Vector2{ 0, 0 }), m_Position2(Vector2{ 0, 0 }) { }

	Arrow(const Vector2& position, const Vector2& position2, const Vector2& square, const Vector2& square2, const Color& color, bool flip)
		: IGuide(position, square, color, flip), m_Position2(position2), m_Square2(square2) { }

	void Draw(const Rectangle& boardBounds) const override
	{
		if (m_Flip)
		{
			Vector2 fromPos =
			{
				boardBounds.width - m_Position.x + 2 * boardBounds.x,
				boardBounds.height - m_Position.y + 2 * boardBounds.y
			};
			Vector2 toPos =
			{
				boardBounds.width - m_Position2.x + 2 * boardBounds.x,
				boardBounds.height - m_Position2.y + 2 * boardBounds.y
			};
			DrawLineEx(fromPos, toPos, Size / 7, m_Color);
		}
		else
			DrawLineEx(m_Position, m_Position2, Size / 7, m_Color);
	}

	void UpdatePosition(const Rectangle& bounds, uint32_t size) override
	{
		m_Position.x = bounds.x + (m_Square.x + 0.5f) * size;
		m_Position.y = bounds.y + (m_Square.y + 0.5f) * size;
		m_Position2.x = bounds.x + (m_Square2.x + 0.5f) * size;
		m_Position2.y = bounds.y + (m_Square2.y + 0.5f) * size;
	}

	void SetPosition2(const Vector2& position)
	{
		m_Position2 = position;
	}

	const bool operator==(const Arrow& other) const
	{
		return m_Position.x == other.m_Position.x && m_Position.y == other.m_Position.y &&
			m_Position2.x == other.m_Position2.x && m_Position2.y == other.m_Position2.y;
	}

private:
	Vector2 m_Position2;
	Vector2 m_Square2;
};