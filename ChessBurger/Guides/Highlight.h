#pragma once

#include "raylib.h"
#include "IGuide.h"
#include <memory>
#include <cmath>

class Highlight : public IGuide
{
public:
	Highlight()
		: IGuide() { }

	Highlight(const Vector2& position, const Vector2& square, const Color& color, bool flip = false)
		: IGuide(position, square, color, flip) { }

	void Draw(const Rectangle& boardBounds) const override
	{
		if (m_Flip)
		{
			float size = std::floor(boardBounds.width / 8.0f);
			float x = 2 * boardBounds.x + 7 * std::floor(boardBounds.width / 8.0f) - m_Position.x;
			float y = 2 * boardBounds.y + 7 * std::floor(boardBounds.width / 8.0f) - m_Position.y;
			DrawRectangleV(Vector2{ x, y }, Vector2{ (float)Size, (float)Size }, m_Color);
		}
		else
			DrawRectangleV(m_Position, Vector2{ (float)Size, (float)Size }, m_Color);
	}

	void UpdatePosition(const Rectangle& bounds, uint32_t size) override
	{
		m_Position.x = bounds.x + m_Square.x * (float)size;
		m_Position.y = bounds.y + m_Square.y * (float)size;
	}

	const bool operator==(const Highlight& other) const
	{
		return m_Position.x == other.m_Position.x && m_Position.y == other.m_Position.y;
	}
};