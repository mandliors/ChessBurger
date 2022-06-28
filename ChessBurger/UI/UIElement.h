#pragma once

#include "raylib.h"
#include "Game/Game.h"
#include "GameData/GameData.h"

#include <iostream>

class UIElement
{
public:
	UIElement(const Rectangle& bounds = Rectangle{ 0, 0, 0, 0 })
		: m_Bounds(bounds) { }
	~UIElement() { }
	
	virtual void Update() {	}
	virtual void Draw() const { }
	virtual void SetBounds(const Rectangle& bounds)	{ m_Bounds = bounds; }
	const Rectangle& GetBounds() const { return m_Bounds; }

protected:
	Rectangle m_Bounds;
};