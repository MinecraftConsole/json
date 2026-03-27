#include "stdafx.h"
#include "UI.h"
#include "UIComponent_Logo.h"
#include "..\..\BufferedImage.h"
#include "..\..\Minecraft.h"
#include "..\..\Tesselator.h"

namespace
{
	const wchar_t *g_menuTitleResource = L"/title/mclogo.png";
	int g_menuTitleTextureLoaded = 0;
	int g_menuTitleTextureWidth = 0;
	int g_menuTitleTextureHeight = 0;

	bool IsSplitViewport(C4JRender::eViewportType viewport)
	{
		switch(viewport)
		{
		case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
		case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
		case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
		case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
		case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
		case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
		case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
		case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
			return true;
		}
		return false;
	}

	void EnsureMenuTitleTextureLoaded()
	{
		if(g_menuTitleTextureLoaded != 0)
		{
			return;
		}

		Minecraft *minecraft = Minecraft::GetInstance();
		if(minecraft == NULL || minecraft->textures == NULL)
		{
			return;
		}

		BufferedImage *image = new BufferedImage(g_menuTitleResource);
		if(image != NULL && image->getData() != NULL)
		{
			g_menuTitleTextureWidth = image->getWidth();
			g_menuTitleTextureHeight = image->getHeight();
			g_menuTitleTextureLoaded = 1;
		}
		else
		{
			app.DebugPrintf("UIComponent_Logo: failed to load %ls\n", g_menuTitleResource);
		}

		delete image;
	}

	void DrawTexturedQuad(float x, float y, float width, float height, float texWidth, float texHeight)
	{
		Tesselator *t = Tesselator::getInstance();
		const float uMax = (texWidth > 0.0f) ? 1.0f : 0.0f;
		const float vMax = (texHeight > 0.0f) ? 1.0f : 0.0f;

		t->begin();
		t->vertexUV(x,         y + height, 0.0f, 0.0f, vMax);
		t->vertexUV(x + width, y + height, 0.0f, uMax, vMax);
		t->vertexUV(x + width, y,          0.0f, uMax, 0.0f);
		t->vertexUV(x,         y,          0.0f, 0.0f, 0.0f);
		t->end();
	}
}

UIComponent_Logo::UIComponent_Logo(int iPad, void *initData, UILayer *parentLayer) : UIScene(iPad, parentLayer)
{
	// Setup all the Iggy references we need for this scene
#if defined(_WINDOWS64)
	// Windows64 draws the menu title manually from MenuTitle.png.
	// Do not initialise the Iggy logo movie here, otherwise the baked SWF logo still renders underneath.
#else
	initialiseMovie();
#endif
}

wstring UIComponent_Logo::getMoviePath()
{
	switch( m_parentLayer->getViewport() )
	{
	case C4JRender::VIEWPORT_TYPE_SPLIT_TOP:
	case C4JRender::VIEWPORT_TYPE_SPLIT_BOTTOM:
	case C4JRender::VIEWPORT_TYPE_SPLIT_LEFT:
	case C4JRender::VIEWPORT_TYPE_SPLIT_RIGHT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_LEFT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_TOP_RIGHT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_LEFT:
	case C4JRender::VIEWPORT_TYPE_QUADRANT_BOTTOM_RIGHT:
		return L"ComponentLogoSplit";
		break;
	case C4JRender::VIEWPORT_TYPE_FULLSCREEN:
	default:
		return L"ComponentLogo";
		break;
	}
}

void UIComponent_Logo::render(S32 width, S32 height, C4JRender::eViewportType viewport)
{
#if defined(_WINDOWS64)
	EnsureMenuTitleTextureLoaded();

	if(g_menuTitleTextureLoaded == 0 || g_menuTitleTextureWidth <= 0 || g_menuTitleTextureHeight <= 0)
	{
		return;
	}

	Minecraft *minecraft = Minecraft::GetInstance();
	if(minecraft == NULL || minecraft->textures == NULL)
	{
		return;
	}

	ui.setupRenderPosition(viewport);

	float drawWidth = (float)width * (IsSplitViewport(viewport) ? 0.62f : 0.72f);
	const float textureAspect = (float)g_menuTitleTextureWidth / (float)g_menuTitleTextureHeight;
	float drawHeight = drawWidth / textureAspect;
	float x = ((float)width - drawWidth) * 0.5f;
	float y = (float)height * (IsSplitViewport(viewport) ? 0.05f : 0.04f);

	if(drawHeight > ((float)height * 0.30f))
	{
		drawHeight = (float)height * 0.30f;
		drawWidth = drawHeight * textureAspect;
		x = ((float)width - drawWidth) * 0.5f;
	}

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
	glOrtho(0.0, (double)width, (double)height, 0.0, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();

	Tesselator::getInstance()->color(0xffffff);
	minecraft->textures->bindTexture(g_menuTitleResource);
	DrawTexturedQuad(x, y, drawWidth, drawHeight, (float)g_menuTitleTextureWidth, (float)g_menuTitleTextureHeight);

	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
#else
	UIScene::render(width, height, viewport);
#endif
}
