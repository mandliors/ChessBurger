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
	UIText(const std::string& text = "", float textSize = 0, const Vector2& position = Vector2{ 0, 0 })
		: UIElement(Rectangle{ 0, 0, 0, 0 }), m_Text(text), m_TextSize(textSize), m_State(UIState::NORMAL), m_Size({ 0.0f, 0.0f })
	{
		float width = MeasureTextEx(GameData::MainFont, m_Text.c_str(), m_TextSize, 0.0f).x;
		m_Padding = UITEXT_PADDING_P * textSize;
		m_Bounds = Rectangle{ position.x, position.y, width + 2 * m_Padding, textSize + 2 * m_Padding };
	}
	
	UIState GetState() const
	{
		return m_State;
	}

	std::string GetText() const
	{
		return m_Text;
	}

	void SetState(UIState state)
	{
		m_State = state;
	}

	void SetText(const std::string& text)
	{
		m_Text = text;
		float width = MeasureTextEx(GameData::MainFont, m_Text.c_str(), m_TextSize, 0.0f).x;
		m_Bounds.width = width + 2 * m_Padding;
	}

	void SetTextSize(float textSize)
	{
		m_TextSize = textSize;
		m_Padding = UITEXT_PADDING_P * textSize;
		float width = MeasureTextEx(GameData::MainFont, m_Text.c_str(), m_TextSize, 0.0f).x;
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
			DrawRectangleRounded(m_Bounds, 0.1f, 2, GameData::Colors.BgNormal);
			DrawTextEx(GameData::MainFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0.0f, GameData::Colors.FgNormal);
			break;
		case UIState::HOVERED:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, GameData::Colors.BgHovered);
			DrawTextEx(GameData::MainFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0.0f, GameData::Colors.FgHovered);
			break;
		case UIState::FOCUSED:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, GameData::Colors.BgFocused);
			DrawTextEx(GameData::MainFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0.0f, GameData::Colors.FgFocused);
			break;
		case UIState::FOCUSED2:
			DrawRectangleRounded(m_Bounds, 0.1f, 2, GameData::Colors.BgFocused2);
			DrawTextEx(GameData::MainFont, m_Text.c_str(), Vector2{ m_Bounds.x + m_Padding, m_Bounds.y + m_Padding }, m_TextSize, 0.0f, GameData::Colors.FgFocused2);
			break;
		}
	}

private:
	std::string m_Text;
	float m_TextSize;
	UIState m_State;
	Vector2 m_Size;
	float m_Padding;
};