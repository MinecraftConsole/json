#pragma once

#include <string>

class Minecraft;
class Font;

class ChangeNameButton
{
public:
	ChangeNameButton();

	float GetWidth(Minecraft *minecraft);
	float GetHeight(Minecraft *minecraft);
	bool HitTest(float mouseX, float mouseY, float x, float y, float width, float height) const;
	void Render(Minecraft *minecraft, Font *font, const std::wstring &label, float x, float y, float width, float height, bool hovered, int viewport);

private:
	void EnsureTextures(Minecraft *minecraft);

	int m_normalTextureId;
	int m_hoverTextureId;
	int m_textureWidth;
	int m_textureHeight;
	bool m_checked;
	bool m_ready;
};
