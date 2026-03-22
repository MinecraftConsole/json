#pragma once

#include <string>

class Minecraft;
class Font;

class CustomGenericButton
{
public:
	CustomGenericButton();

	/// Call once to configure the button.  Texture paths point to PNGs
	/// inside the game archive (e.g. L"/Graphics/MyButton_Norm.png").
	void Setup(const wchar_t *normalTexturePath,
	           const wchar_t *hoverTexturePath,
	           float x, float y, float width, float height);

	/// Convenience overload for standard main-menu buttons.
	/// Uses MainMenuButton_Norm.png / MainMenuButton_Over.png.
	/// Width and height are derived from the texture's natural size,
	/// scaled to fit the same width as the standard custom menu buttons.
	void SetupMenuButton(float x, float y);

	/// Returns true if the mouse is inside the button rect.
	bool HitTest(float mouseX, float mouseY) const;

	/// Call every tick.  Handles hover-enter sound (eSFX_Focus) and
	/// click sound (eSFX_Press).  Returns true the frame the button is clicked.
	bool Update(Minecraft *minecraft);

	/// Draw the button (texture + centred text label).
	/// Text is white normally, yellow on hover, with a black shadow.
	void Render(Minecraft *minecraft, Font *font,
	            const std::wstring &label, int viewport);

	// Accessors
	float GetX() const      { return m_x; }
	float GetY() const      { return m_y; }
	float GetWidth() const  { return m_width; }
	float GetHeight() const { return m_height; }
	bool  IsHovered() const { return m_hovered; }

private:
	void EnsureTextures(Minecraft *minecraft);

	// Texture paths (set by Setup, not owned)
	const wchar_t *m_normalPath;
	const wchar_t *m_hoverPath;

	// Loaded texture IDs
	int  m_normalTexId;
	int  m_hoverTexId;
	int  m_textureWidth;
	int  m_textureHeight;
	bool m_checked;
	bool m_ready;

	// Layout
	float m_x;
	float m_y;
	float m_width;
	float m_height;
	bool  m_useTextureHeight;  ///< if true, height is derived from texture aspect ratio

	// Hover state
	bool m_hovered;
	bool m_wasHovered;
};
