#pragma once

#include "raylib.h"
#include "IGuide.h"
#include <memory>
#include <cmath>

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
				2 * boardBounds.x + 8 * std::floor(boardBounds.width / 8.0f) - m_Position.x,
				2 * boardBounds.y + 8 * std::floor(boardBounds.width / 8.0f) - m_Position.y
			};
			Vector2 toPos =
			{
				2 * boardBounds.x + 8 * std::floor(boardBounds.width / 8.0f) - m_Position2.x,
				2 * boardBounds.y + 8 * std::floor(boardBounds.width / 8.0f) - m_Position2.y
			};
			float dx = toPos.x - fromPos.x;
			float dy = toPos.y - fromPos.y;
			float angle = std::atan2f(dy, dx);
			float cos = std::cosf(angle);
			float sin = std::sinf(angle);
			angle *= 180.0f / 3.1415926f;
			float size = Size * HeadSizeMultiplier;
			DrawLineEx(Vector2{ fromPos.x + cos * Size * OffsetMultiplier, fromPos.y + sin * Size * OffsetMultiplier }, Vector2{ toPos.x - cos * Size * 0.866f * HeadSizeMultiplier, toPos.y - sin * Size * 0.866f * HeadSizeMultiplier }, Size / ThicknessDivisor, m_Color);
			DrawTexturePro(Head, Rectangle{ 0, 0, (float)Head.width, (float)Head.height }, Rectangle{ toPos.x + cos * (0.134f * size - size * 0.5f), toPos.y + sin * (0.134f * size - size * 0.5f), size, size }, Vector2{ size * 0.5f, size * 0.5f }, angle, m_Color);

		}
		else
		{
			float dx = m_Position2.x - m_Position.x;
			float dy = m_Position2.y - m_Position.y;
			float angle = std::atan2f(dy, dx);
			float cos = std::cosf(angle);
			float sin = std::sinf(angle);
			angle *= 180.0f / 3.1415926f;
			float size = Size * HeadSizeMultiplier;
			DrawLineEx(Vector2{ m_Position.x + cos * Size * OffsetMultiplier, m_Position.y + sin * Size * OffsetMultiplier }, Vector2{ m_Position2.x - cos * Size * 0.866f * HeadSizeMultiplier, m_Position2.y - sin * Size * 0.866f * HeadSizeMultiplier }, Size / ThicknessDivisor, m_Color);
			DrawTexturePro(Head, Rectangle{ 0, 0, (float)Head.width, (float)Head.height }, Rectangle{ m_Position2.x + cos * (0.134f * size - size * 0.5f), m_Position2.y + sin * (0.134f * size - size * 0.5f), size, size}, Vector2{size * 0.5f, size * 0.5f}, angle, m_Color);
		}
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

public:
	static Texture2D Head;
	static float ThicknessDivisor;
	static float HeadSizeMultiplier;
	static float OffsetMultiplier;

private:
	Vector2 m_Position2;
	Vector2 m_Square2;
};