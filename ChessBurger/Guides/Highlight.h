#pragma once

#include "raylib.h"
#include "IGuide.h"
#include <memory>

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
			float size = boardBounds.width / 8.0f;
			Vector2 pos =
			{
				boardBounds.width - m_Position.x + 2 * boardBounds.x - size,
				boardBounds.height - m_Position.y + 2 * boardBounds.y - size
			};
			DrawRectangle(pos.x, pos.y, Size, Size, m_Color);
		}
		else
			DrawRectangle(m_Position.x, m_Position.y, Size, Size, m_Color);
	}

	void UpdatePosition(const Rectangle& bounds, uint32_t size) override
	{
		m_Position.x = bounds.x + m_Square.x * size;
		m_Position.y = bounds.y + m_Square.y * size;
	}

	const bool operator==(const Highlight& other) const
	{
		return m_Position.x == other.m_Position.x && m_Position.y == other.m_Position.y;
	}
};