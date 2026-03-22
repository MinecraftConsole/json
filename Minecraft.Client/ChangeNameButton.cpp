#include "stdafx.h"
#include "ChangeNameButton.h"

#include "BufferedImage.h"
#include "Minecraft.h"
#include "Tesselator.h"
#include "Common\UI\UI.h"

namespace
{
	const wchar_t *kChangeNameButtonNormalFile = L"/Graphics/ListButton_Norm.png";
	const wchar_t *kChangeNameButtonHoverFile = L"/Graphics/ListButton_Over.png";
}

ChangeNameButton::ChangeNameButton()
	: m_normalTextureId(-1)
	, m_hoverTextureId(-1)
	, m_textureWidth(0)
	, m_textureHeight(0)
	, m_checked(false)
	, m_ready(false)
{
}

float ChangeNameButton::GetWidth(Minecraft *minecraft)
{
	EnsureTextures(minecraft);
	if(m_textureWidth > 0)
	{
		return (float)m_textureWidth;
	}
	return 220.0f;
}

float ChangeNameButton::GetHeight(Minecraft *minecraft)
{
	EnsureTextures(minecraft);
	if(m_textureHeight > 0)
	{
		return (float)m_textureHeight;
	}
	return 40.0f;
}

void ChangeNameButton::EnsureTextures(Minecraft *minecraft)
{
	if(m_ready || m_checked)
	{
		return;
	}

	if(minecraft == NULL || minecraft->textures == NULL)
	{
		return;
	}

	m_checked = true;

	BufferedImage *normalImage = new BufferedImage(kChangeNameButtonNormalFile);
	if(normalImage != NULL && normalImage->getData() != NULL && normalImage->getWidth() > 0 && normalImage->getHeight() > 0)
	{
		m_textureWidth = normalImage->getWidth();
		m_textureHeight = normalImage->getHeight();
		// Get a texture ID from the engine instead of a hardcoded 1
		m_normalTextureId = minecraft->textures->getTexture(normalImage);
	}
	delete normalImage;

	BufferedImage *hoverImage = new BufferedImage(kChangeNameButtonHoverFile);
	if(hoverImage != NULL && hoverImage->getData() != NULL && hoverImage->getWidth() > 0 && hoverImage->getHeight() > 0)
	{
		m_hoverTextureId = minecraft->textures->getTexture(hoverImage);
	}
	delete hoverImage;

	if(m_hoverTextureId < 0)
	{
		m_hoverTextureId = m_normalTextureId;
	}

	m_ready = (m_normalTextureId >= 0);
	if(!m_ready)
	{
		m_checked = false;
	}
}

bool ChangeNameButton::HitTest(float mouseX, float mouseY, float x, float y, float width, float height) const
{
	return (mouseX >= x && mouseX <= (x + width) && mouseY >= y && mouseY <= (y + height));
}

void ChangeNameButton::Render(Minecraft *minecraft, Font *font, const std::wstring &label, float x, float y, float width, float height, bool hovered, int viewport)
{
	EnsureTextures(minecraft);
	if(!m_ready || minecraft == NULL || minecraft->textures == NULL)
	{
		return;
	}

	if(width <= 0.0f)
	{
		width = GetWidth(minecraft);
	}
	if(height <= 0.0f)
	{
		height = GetHeight(minecraft);
	}

	ui.setupRenderPosition((C4JRender::eViewportType)viewport);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	glDisable(GL_FOG);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0.0, (double)minecraft->width_phys, (double)minecraft->height_phys, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	minecraft->textures->bind(hovered ? m_hoverTextureId : m_normalTextureId);
	Tesselator *t = Tesselator::getInstance();
	t->begin();
	t->color(0xffffff);
	t->vertexUV(x,         y + height, 0.0f, 0.0f, 1.0f);
	t->vertexUV(x + width, y + height, 0.0f, 1.0f, 1.0f);
	t->vertexUV(x + width, y,          0.0f, 1.0f, 0.0f);
	t->vertexUV(x,         y,          0.0f, 0.0f, 0.0f);
	t->end();

	if(font != NULL && !label.empty())
	{
		const float textScale = 2.0f;
		const float textWidth = (float)font->width(label) * textScale;
		const float textX = x + ((width - textWidth) * 0.5f) + 5.0f;
		const float textY = y + ((height - 8.0f * textScale) * 0.5f) - 1.0f;
		
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glPushMatrix();
		glTranslatef(textX, textY, 0.0f);
		glScalef(textScale, textScale, 1.0f);
		font->draw(label, 1, 1, 0xff000000); // Black shadow
		font->draw(label, 0, 0, 0xffffffff); // White text
		glPopMatrix();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
