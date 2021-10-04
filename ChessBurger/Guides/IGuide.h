#pragma once

#pragma once

#include "raylib.h"
#include <memory>

class IGuide
{
public:
	IGuide();
	IGuide(const Vector2& position, const Vector2& square, const Color& color, bool flip = false);

	virtual void Draw(const Rectangle& boardBounds) const = 0;
	virtual void UpdatePosition(const Rectangle& bounds, uint32_t size) = 0;

	Vector2 GetSquare() const;
	void SetPosition(const Vector2& position);
	void SetFlip(bool flip);
	const bool operator==(const IGuide& other) const;

public:
	static uint32_t Size;

protected:
	Vector2 m_Position;
	Vector2 m_Square;
	Color m_Color;
	bool m_Flip;
};