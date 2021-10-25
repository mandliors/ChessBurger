#pragma once

#include "raylib.h"
#include "IGuide.h"
#include <memory>

#define SPOT_SIZE_P 0.4f

class Spot : public IGuide
{
public:
	Spot()
		: IGuide() { }

	Spot(const Vector2& position, const Vector2& square, const Color& color, bool flip = false)
		: IGuide(position, square, color, flip) { }

	void Draw(const Rectangle& boardBounds) const override
	{
		if (m_Flip)
		{
			float size = std::floor(boardBounds.width / 8.0f);
			Vector2 pos =
			{
				2 * boardBounds.x + 8 * std::floor(boardBounds.width / 8.0f) - m_Position.x,
				2 * boardBounds.y + 8 * std::floor(boardBounds.width / 8.0f) - m_Position.y
			};
			DrawCircle(pos.x, pos.y, Size / 2.0f * SPOT_SIZE_P, m_Color);
		}
		else
			DrawCircle(m_Position.x, m_Position.y, Size / 2.0f * SPOT_SIZE_P, m_Color);
	}

	void UpdatePosition(const Rectangle& bounds, uint32_t size) override
	{
		m_Position.x = bounds.x + (m_Square.x + 0.5f) * size;
		m_Position.y = bounds.y + (m_Square.y + 0.5f) * size;
	}

	const bool operator==(const Spot& other) const
	{
		return m_Position.x == other.m_Position.x && m_Position.y == other.m_Position.y;
	}
};