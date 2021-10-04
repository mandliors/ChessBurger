#pragma once

#include "UIElement.h"
#include "Game/Game.h"
#include "GameData/GameData.h"
#include <string>

#define UITEXT_PADDING_P 0.2f

enum class UIState
{
	NORMAL = 0, HOVERED, FOCUSED, FOCUSED2
};

class UIText : public UIElement
{
public:
	UIText(const std::string& text = "", uint32_t textSize = 0, const Vector2& position = Vector2{ 0, 0 }, const ColorBuffer& colorBuffer = Game::DefaultColorBuffer)
		: UIElement(Rectangle{ 0, 0, 0, 0 }, colorBuffer), m_Text(text), m_TextSize(textSize), m_State(UIState::NORMAL)
	{
		int width = MeasureTextEx(GameData::GameFont, m_Text.c_str(), m_TextSize, 0).x;
		m_Padding = UITEXT_PADDING_P * textSize;
		m_Bounds = Rectangle{ position.x, position.y, width + 2 * m_Padding, textSize + 2 * m_Padding };
	}
	
	UIState GetState() const
	{
		return m_State;
	}

	void SetState(UIState state)
	{
		m_State = state;
	}

	void SetText(const std::string& text)
	{
		m_Text = text;
		int width = MeasureTextEx(GameData::GameFont, m_Text.c_str(), m_TextSize, 0).x;
		m_Bounds.width = width + 2 * m_Padding;
	}

	void SetTextSize(uint32_t textSize)
	{
		m_TextSize = textSize;
		m_Padding = UITEXT_PADDING_P * textSize;
		int width = MeasureTextEx(GameData::GameFont, m_Text.c_str(), m_TextSize, 0).x;
		m_Bounds.width = width + 2 * m_Padding;
		m_Bounds.height = m_TextSize + 2 * m_Padding;
	}

	void SetPosition(const Vector2& position)
	{
		m_Bounds.x = position.x;
		m_Bounds.y = position.y;
	}

	void Draw() const override
	{
		switch (m_State)
		{
		case UIState::NORMAL:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, m_ColorBuffer.BgNormal);
			DrawTextEx(GameData::GameFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0, m_ColorBuffer.FgNormal);
			break;
		case UIState::HOVERED:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, m_ColorBuffer.BgHovered);
			DrawTextEx(GameData::GameFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0, m_ColorBuffer.FgHovered);
			break;
		case UIState::FOCUSED:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, m_ColorBuffer.BgFocused);
			DrawTextEx(GameData::GameFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0, m_ColorBuffer.FgFocused);
			break;
		case UIState::FOCUSED2:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, m_ColorBuffer.BgFocused2);
			DrawTextEx(GameData::GameFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0, m_ColorBuffer.FgFocused2);
			break;
		}
	}

private:
	std::string m_Text;
	uint32_t m_TextSize;
	UIState m_State;
	Vector2 m_Size;
	float m_Padding;
};