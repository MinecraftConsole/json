#pragma once

#include <string>

class Minecraft;
class Font;

/// A horizontal slider control that renders using game PNG assets.
///
/// Normal state  : Slider_Track.png as background, white centred label.
/// Hovered state : LeaderboardButton_Over.png as tinted background,
///                 yellow border, yellow label.
/// Handle        : Slider_Button.png positioned proportionally.
///
/// Usage:
///   CustomSlider mySlider;
///   mySlider.Setup(300.0f, 400.0f);                      // 0-100, 600x32
///   mySlider.Setup(300.0f, 400.0f, 200);                 // 0-200, 600x32
///   mySlider.Setup(300.0f, 400.0f, 480.0f, 40.0f, 100); // 0-100, 480x40
///   bool changed = mySlider.Update(minecraft);
///   mySlider.Render(minecraft, font, IDS_MY_LABEL, viewport);
class CustomSlider
{
public:
    CustomSlider();

    /// Configure the slider using default 600x32 dimensions.
    /// @param limitMax  Maximum value (range 0…limitMax). Default 100.
    void Setup(float x, float y, int limitMax = 100);

    /// Configure the slider with explicit dimensions.
    /// @param width, height  Track size in physical pixels.
    /// @param limitMax       Maximum value (range 0…limitMax). Default 100.
    void Setup(float x, float y, float width, float height, int limitMax = 100);

    /// Returns true the frame the value changes (mouse dragging).
    bool Update(Minecraft *minecraft);

    /// Draw track, handle, hover overlay and centred label.
    /// @param stringId  String table ID (like IDS_SENSITIVITY).  The rendered
    ///                  text will be  "[label]: [value]%"  (or "/max" if max≠100).
    void Render(Minecraft *minecraft, Font *font, int stringId, int viewport);

    // ── Accessors ──────────────────────────────────────────────
    int   GetValue()    const { return m_value; }
    float GetX()        const { return m_x; }
    float GetY()        const { return m_y; }
    float GetWidth()    const { return m_width; }
    float GetHeight()   const { return m_height; }
    bool  IsHovered()   const { return m_hovered; }
    bool  IsDragging()  const { return m_dragging; }

private:
    void EnsureTextures(Minecraft *minecraft);
    void DrawBorder(float x, float y, float w, float h, float thickness, unsigned int colour);

    // Texture paths
    static const wchar_t *kTrackPath;
    static const wchar_t *kHandlePath;
    static const wchar_t *kHoverBgPath;

    // Texture IDs
    int  m_trackTexId;
    int  m_handleTexId;
    int  m_hoverBgTexId;
    bool m_checked;
    bool m_ready;

    // Layout  (track)
    float m_x, m_y;
    float m_width;   ///< 600 px by default
    float m_height;  ///< 32 px by default

    // Handle size (derived from Slider_Button.png aspect ratio)
    float m_handleW;
    float m_handleH;

    // Range & value
    int m_limitMax;
    int m_value;

    // Interaction
    bool m_hovered;
    bool m_wasHovered;
    bool m_dragging;
};
