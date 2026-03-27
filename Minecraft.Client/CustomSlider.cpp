#include "stdafx.h"
#include "CustomSlider.h"

#include "BufferedImage.h"
#include "Minecraft.h"
#include "Tesselator.h"
#include "Common\UI\UI.h"

// ── Default track dimensions (physical pixels, at 1080p) ─────────────────────
static const float kSliderWidth  = 600.0f;
static const float kSliderHeight =  32.0f;

const wchar_t *CustomSlider::kTrackPath   = L"/Graphics/Slider_Track.png";
const wchar_t *CustomSlider::kHandlePath  = L"/Graphics/Slider_Button.png";
const wchar_t *CustomSlider::kHoverBgPath = L"/Graphics/LeaderboardButton_Over.png";

// ─────────────────────────────────────────────────────────────────────────────
// Ctor
// ─────────────────────────────────────────────────────────────────────────────
CustomSlider::CustomSlider()
    : m_trackTexId(-1)
    , m_handleTexId(-1)
    , m_hoverBgTexId(-1)
    , m_checked(false)
    , m_ready(false)
    , m_x(0.0f)
    , m_y(0.0f)
    , m_width(kSliderWidth)
    , m_height(kSliderHeight)
    , m_handleW(kSliderHeight)      // square by default; corrected on first draw
    , m_handleH(kSliderHeight)
    , m_limitMax(100)
    , m_value(50)
    , m_hovered(false)
    , m_wasHovered(false)
    , m_dragging(false)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────
void CustomSlider::Setup(float x, float y, int limitMax)
{
    Setup(x, y, kSliderWidth, kSliderHeight, limitMax);
}

void CustomSlider::Setup(float x, float y, float width, float height, int limitMax)
{
    m_x        = x;
    m_y        = y;
    m_width    = (width  > 0.0f) ? width  : kSliderWidth;
    m_height   = (height > 0.0f) ? height : kSliderHeight;
    m_limitMax = (limitMax > 0)  ? limitMax : 100;
    m_value    = m_limitMax / 2;

    // Reset handle to match new height (will be refined by texture aspect ratio)
    m_handleW = m_height;
    m_handleH = m_height;

    // Force texture reload
    m_checked      = false;
    m_ready        = false;
    m_trackTexId   = -1;
    m_handleTexId  = -1;
    m_hoverBgTexId = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// EnsureTextures
// ─────────────────────────────────────────────────────────────────────────────
void CustomSlider::EnsureTextures(Minecraft *minecraft)
{
    if(m_ready || m_checked) return;
    if(minecraft == NULL || minecraft->textures == NULL) return;
    m_checked = true;

    // Track
    BufferedImage *trackImg = new BufferedImage(kTrackPath);
    if(trackImg && trackImg->getData() && trackImg->getWidth() > 0)
        m_trackTexId = minecraft->textures->getTexture(trackImg);
    delete trackImg;

    // Handle – derive its rendered width from the PNG aspect ratio
    BufferedImage *handleImg = new BufferedImage(kHandlePath);
    if(handleImg && handleImg->getData() && handleImg->getWidth() > 0 && handleImg->getHeight() > 0)
    {
        m_handleTexId = minecraft->textures->getTexture(handleImg);
        // The handle is rendered at the full track height; compute width
        float aspect = (float)handleImg->getWidth() / (float)handleImg->getHeight();
        m_handleH = m_height;
        m_handleW = m_height * aspect;
    }
    delete handleImg;

    // Hover background
    BufferedImage *hoverImg = new BufferedImage(kHoverBgPath);
    if(hoverImg && hoverImg->getData() && hoverImg->getWidth() > 0)
        m_hoverBgTexId = minecraft->textures->getTexture(hoverImg);
    delete hoverImg;

    m_ready = (m_trackTexId >= 0);
    if(!m_ready) m_checked = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Update
// ─────────────────────────────────────────────────────────────────────────────
bool CustomSlider::Update(Minecraft *minecraft)
{
    if(minecraft == NULL) return false;

    const float scaleX = (float)minecraft->width_phys  / 1920.0f;
    const float scaleY = (float)minecraft->height_phys / 1080.0f;
    const float mouseX = (float)Mouse::getX() * scaleX;
    const float mouseY = (float)Mouse::getY() * scaleY;

    // Hit test on the full track rect
    m_hovered = (mouseX >= m_x && mouseX <= m_x + m_width &&
                 mouseY >= m_y && mouseY <= m_y + m_height);

    // Hover-enter sound
    if(m_hovered && !m_wasHovered)
        ui.PlayUISFX(eSFX_Focus);
    m_wasHovered = m_hovered;

    // Start / continue drag on left-button
    const bool lmb = Mouse::isButtonDown(0);
    if(lmb && m_hovered && !m_dragging)
    {
        m_dragging = true;
        ui.PlayUISFX(eSFX_Press);
    }
    if(!lmb) m_dragging = false;

    if(m_dragging)
    {
        // Clamp the effective handle centre to the track
        const float trackStart = m_x + m_handleW * 0.5f;
        const float trackEnd   = m_x + m_width - m_handleW * 0.5f;
        const float clamped    = (mouseX < trackStart) ? trackStart
                               : (mouseX > trackEnd)   ? trackEnd
                               : mouseX;

        const float ratio = (clamped - trackStart) / (trackEnd - trackStart);
        const int   oldValue = m_value;
        m_value = (int)(ratio * (float)m_limitMax + 0.5f);
        if(m_value < 0)         m_value = 0;
        if(m_value > m_limitMax) m_value = m_limitMax;
        return (m_value != oldValue);
    }

    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// DrawBorder  – draws a solid-colour rectangle outline
// ─────────────────────────────────────────────────────────────────────────────
void CustomSlider::DrawBorder(float x, float y, float w, float h,
                              float t, unsigned int colour)
{
    Tesselator *ts = Tesselator::getInstance();
    const float rf = (float)((colour >> 16) & 0xFF) / 255.0f;
    const float gf = (float)((colour >>  8) & 0xFF) / 255.0f;
    const float bf = (float)( colour        & 0xFF) / 255.0f;

    glDisable(GL_TEXTURE_2D);
    glColor4f(rf, gf, bf, 1.0f);

    // Top
    ts->begin(); ts->vertex(x,     y,     0); ts->vertex(x+w,   y,     0);
    ts->vertex(x+w,   y+t,   0); ts->vertex(x,     y+t,   0); ts->end();
    // Bottom
    ts->begin(); ts->vertex(x,     y+h-t, 0); ts->vertex(x+w,   y+h-t, 0);
    ts->vertex(x+w,   y+h,   0); ts->vertex(x,     y+h,   0); ts->end();
    // Left
    ts->begin(); ts->vertex(x,     y+t,   0); ts->vertex(x+t,   y+t,   0);
    ts->vertex(x+t,   y+h-t, 0); ts->vertex(x,     y+h-t, 0); ts->end();
    // Right
    ts->begin(); ts->vertex(x+w-t, y+t,   0); ts->vertex(x+w,   y+t,   0);
    ts->vertex(x+w,   y+h-t, 0); ts->vertex(x+w-t, y+h-t, 0); ts->end();

    glEnable(GL_TEXTURE_2D);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Render
// ─────────────────────────────────────────────────────────────────────────────
void CustomSlider::Render(Minecraft *minecraft, Font *font, int stringId, int viewport)
{
    EnsureTextures(minecraft);
    if(!m_ready || minecraft == NULL || minecraft->textures == NULL) return;

    const float w = m_width;
    const float h = m_height;

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

    // ── 1. Draw track background ──────────────────────────────────────────
    minecraft->textures->bind(m_trackTexId);
    Tesselator *t = Tesselator::getInstance();
    t->begin();
    t->color(0xffffff);
    t->vertexUV(m_x,     m_y + h, 0.0f, 0.0f, 1.0f);
    t->vertexUV(m_x + w, m_y + h, 0.0f, 1.0f, 1.0f);
    t->vertexUV(m_x + w, m_y,     0.0f, 1.0f, 0.0f);
    t->vertexUV(m_x,     m_y,     0.0f, 0.0f, 0.0f);
    t->end();

    // ── 2. If hovered: Draw hover overlay (LeaderboardButton_Over) ────────
    if(m_hovered && m_hoverBgTexId >= 0)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        minecraft->textures->bind(m_hoverBgTexId);
        t->begin();
        t->color(0xffffff);
        t->vertexUV(m_x,     m_y + h, 0.0f, 0.0f, 1.0f);
        t->vertexUV(m_x + w, m_y + h, 0.0f, 1.0f, 1.0f);
        t->vertexUV(m_x + w, m_y,     0.0f, 1.0f, 0.0f);
        t->vertexUV(m_x,     m_y,     0.0f, 0.0f, 0.0f);
        t->end();

        // Yellow border  (2 px thick)
        DrawBorder(m_x, m_y, w, h, 2.0f, 0xFFFF00);
    }

    // ── 3. Draw handle ────────────────────────────────────────────────────
    if(m_handleTexId >= 0)
    {
        const float trackUsable = w - m_handleW;
        const float ratio  = (m_limitMax > 0) ? (float)m_value / (float)m_limitMax : 0.0f;
        const float hx     = m_x + ratio * trackUsable;
        const float hy     = m_y;

        minecraft->textures->bind(m_handleTexId);
        t->begin();
        t->color(0xffffff);
        t->vertexUV(hx,            hy + m_handleH, 0.0f, 0.0f, 1.0f);
        t->vertexUV(hx + m_handleW,hy + m_handleH, 0.0f, 1.0f, 1.0f);
        t->vertexUV(hx + m_handleW,hy,             0.0f, 1.0f, 0.0f);
        t->vertexUV(hx,            hy,             0.0f, 0.0f, 0.0f);
        t->end();
    }

    // ── 4. Draw centred label ─────────────────────────────────────────────
    if(font != NULL)
    {
        // Build label: "[string]: [value]%"  or  "[string]: [value]/[max]"
        const wchar_t *baseStr = app.GetString(stringId);
        wchar_t labelBuf[256] = {};
        if(m_limitMax == 100)
            _snwprintf_s(labelBuf, _countof(labelBuf), _TRUNCATE, L"%s: %d%%", baseStr, m_value);
        else
            _snwprintf_s(labelBuf, _countof(labelBuf), _TRUNCATE, L"%s: %d/%d", baseStr, m_value, m_limitMax);

        std::wstring label(labelBuf);

        const float textScale  = 2.0f;
        const float textWidth  = (float)font->width(label) * textScale;
        const float textX      = m_x + (w - textWidth) * 0.5f;
        const float textY      = m_y + (h - 8.0f * textScale) * 0.5f;

        // Colour: yellow when hovered, white otherwise
        const unsigned int textColour = m_hovered ? 0xFFFFFF00 : 0xFFFFFFFF;

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(textX, textY, 0.0f);
        glScalef(textScale, textScale, 1.0f);
        font->draw(label, 1.0f, 1.0f, 0xFF000000);  // shadow
        font->draw(label, 0.0f, 0.0f, textColour);
        glPopMatrix();
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}
