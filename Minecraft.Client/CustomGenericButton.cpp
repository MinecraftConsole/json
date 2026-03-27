#include "stdafx.h"
#include "CustomGenericButton.h"

#include "BufferedImage.h"
#include "Minecraft.h"
#include "Tesselator.h"
#include "Common\UI\UI.h"

// Natural width of the MainMenuButton textures (the height will be derived from
// the actual PNG aspect ratio when loaded in EnsureTextures).
static const float kMenuButtonWidth = 480.0f;

CustomGenericButton::CustomGenericButton()
	: m_normalPath(L"/Graphics/ListButton_Norm.png")
	, m_hoverPath(L"/Graphics/ListButton_Over.png")
	, m_normalTexId(-1)
	, m_hoverTexId(-1)
	, m_textureWidth(0)
	, m_textureHeight(0)
	, m_checked(false)
	, m_ready(false)
	, m_x(0.0f)
	, m_y(0.0f)
	, m_width(220.0f)
	, m_height(40.0f)
	, m_useTextureHeight(false)
	, m_hovered(false)
	, m_wasHovered(false)
{
}

void CustomGenericButton::Setup(const wchar_t *normalTexturePath,
                                const wchar_t *hoverTexturePath,
                                float x, float y, float width, float height)
{
	// Only override texture paths if non-NULL (keep defaults otherwise)
	if(normalTexturePath != NULL) m_normalPath = normalTexturePath;
	if(hoverTexturePath  != NULL) m_hoverPath  = hoverTexturePath;
	m_x      = x;
	m_y      = y;
	m_width  = width;
	m_height = height;

	// Reset texture state so they'll be reloaded with the new paths
	m_checked = false;
	m_ready   = false;
	m_normalTexId = -1;
	m_hoverTexId  = -1;
	m_useTextureHeight = false;
}

// ─────────────────────────────────────────────────────────────
// SetupMenuButton – convenience for standard main-menu buttons.
// ─────────────────────────────────────────────────────────────

void CustomGenericButton::SetupMenuButton(float x, float y)
{
	Setup(L"/Graphics/MainMenuButton_Norm.png",
	      L"/Graphics/MainMenuButton_Over.png",
	      x, y,
	      kMenuButtonWidth,
	      40.0f);        // will be refined once the texture loads

	// Mark that we need to derive the height from the texture aspect ratio.
	m_useTextureHeight = true;
}


void CustomGenericButton::EnsureTextures(Minecraft *minecraft)
{
	if(m_ready || m_checked) return;
	if(minecraft == NULL || minecraft->textures == NULL) return;
	if(m_normalPath == NULL) return;

	m_checked = true;

	BufferedImage *normalImage = new BufferedImage(m_normalPath);
	if(normalImage != NULL && normalImage->getData() != NULL &&
	   normalImage->getWidth() > 0 && normalImage->getHeight() > 0)
	{
		m_textureWidth  = normalImage->getWidth();
		m_textureHeight = normalImage->getHeight();
		m_normalTexId   = minecraft->textures->getTexture(normalImage);
	}
	delete normalImage;

	if(m_hoverPath != NULL)
	{
		BufferedImage *hoverImage = new BufferedImage(m_hoverPath);
		if(hoverImage != NULL && hoverImage->getData() != NULL &&
		   hoverImage->getWidth() > 0 && hoverImage->getHeight() > 0)
		{
			m_hoverTexId = minecraft->textures->getTexture(hoverImage);
		}
		delete hoverImage;
	}

	// Fallback: if hover texture failed, use the normal one
	if(m_hoverTexId < 0)
	{
		m_hoverTexId = m_normalTexId;
	}

	m_ready = (m_normalTexId >= 0);
	if(m_ready && m_useTextureHeight && m_textureWidth > 0 && m_textureHeight > 0)
	{
		// Derive the draw height from the natural texture aspect ratio,
		// scaled so that the width matches m_width.
		const float aspect = (float)m_textureWidth / (float)m_textureHeight;
		m_height = m_width / aspect;
		m_useTextureHeight = false; // only do this once
	}
	if(!m_ready)
	{
		m_checked = false;
	}
}

// ─────────────────────────────────────────────────────────────
// Hit-test
// ─────────────────────────────────────────────────────────────

bool CustomGenericButton::HitTest(float mouseX, float mouseY) const
{
	return (mouseX >= m_x && mouseX <= (m_x + m_width) &&
	        mouseY >= m_y && mouseY <= (m_y + m_height));
}

// ─────────────────────────────────────────────────────────────
// Update – call every tick.
//   • Plays eSFX_Focus the frame the mouse enters the button.
//   • Plays eSFX_Press and returns true the frame the button is clicked.
// ─────────────────────────────────────────────────────────────

bool CustomGenericButton::Update(Minecraft *minecraft)
{
	if(minecraft == NULL) return false;

	const float mouseX = ((float)Mouse::getX() / 1920.0f) * (float)minecraft->width_phys;
	const float mouseY = ((float)Mouse::getY() / 1080.0f) * (float)minecraft->height_phys;

	m_hovered = HitTest(mouseX, mouseY);

	// Hover-enter sound (only on transition from not-hovered → hovered)
	if(m_hovered && !m_wasHovered)
	{
		ui.PlayUISFX(eSFX_Focus);
	}
	m_wasHovered = m_hovered;

	// Click
	if(m_hovered && Mouse::isButtonPressed(0))
	{
		ui.PlayUISFX(eSFX_Press);
		return true;
	}

	return false;
}

// ─────────────────────────────────────────────────────────────
// Render – draws button texture + centred text label.
//   Normal: white text with black shadow.
//   Hovered: yellow text with black shadow.
// ─────────────────────────────────────────────────────────────

void CustomGenericButton::Render(Minecraft *minecraft, Font *font,
                                 const std::wstring &label, int viewport)
{
	EnsureTextures(minecraft);
	if(!m_ready || minecraft == NULL || minecraft->textures == NULL) return;

	float w = m_width;
	float h = m_height;

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

	// Draw button texture (normal or hover)
	minecraft->textures->bind(m_hovered ? m_hoverTexId : m_normalTexId);
	Tesselator *t = Tesselator::getInstance();
	t->begin();
	t->color(0xffffff);
	t->vertexUV(m_x,     m_y + h, 0.0f, 0.0f, 1.0f);
	t->vertexUV(m_x + w, m_y + h, 0.0f, 1.0f, 1.0f);
	t->vertexUV(m_x + w, m_y,     0.0f, 1.0f, 0.0f);
	t->vertexUV(m_x,     m_y,     0.0f, 0.0f, 0.0f);
	t->end();

	// Draw centred text label
	if(font != NULL && !label.empty())
	{
		const float textScale = 2.0f;
		const float textWidth = (float)font->width(label) * textScale;
		const float textX = m_x + ((w - textWidth) * 0.5f) + 5.0f;
		const float textY = m_y + ((h - 8.0f * textScale) * 0.5f) - 1.0f;

		// Text colour: white normally, yellow on hover
		const unsigned int textColor = m_hovered ? 0xffffff00 : 0xffffffff;

		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glPushMatrix();
		glTranslatef(textX, textY, 0.0f);
		glScalef(textScale, textScale, 1.0f);
		const float shadowOffset = 1.0f;
		font->draw(label, shadowOffset, shadowOffset, 0xff000000); // Black shadow
		font->draw(label, 0, 0, textColor);   // Coloured text
		glPopMatrix();
	}

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
