#pragma once

#include "raylib.h"
#include "Game/Game.h"
#include "GameData/GameData.h"

#include <iostream>

class UIElement
{
public:
	UIElement(const Rectangle& bounds = Rectangle{ 0, 0, 0, 0 }, const ColorBuffer& colorBuffer = Game::DefaultColorBuffer)
		: m_Bounds(bounds), m_ColorBuffer(colorBuffer) { }
	~UIElement() { }
	
	virtual void Update() {	}
	virtual void Draw() const { }
	virtual void SetBounds(const Rectangle& bounds)	{ m_Bounds = bounds; }
	const Rectangle& GetBounds() const { return m_Bounds; }

protected:
	Rectangle m_Bounds;
	ColorBuffer m_ColorBuffer;
};