/**
 * @file Renderer.hpp
 * @brief Singleton rendering facade wrapping raylib primitives.
 *
 * @details Renderer owns the three application fonts (display, mono, body),
 * the global colour Theme, and provides convenience draw helpers used by
 * every screen and widget.  It is initialised once in Application::Application()
 * and shut down in Application::~Application().
 *
 * @par Usage
 * @code
 * auto &R = zappy::Renderer::instance();
 * R.drawTextMono("ZAPPY", 32.f, 48.f, 48.f, WHITE);
 * R.drawGlow(cx, cy, 40.f, R.theme().accent);
 * @endcode
 *
 * @ingroup Core
 */

#pragma once
#include <functional>
#include <raylib.h>
#include <string>

namespace zappy {

/**
 * @brief Centralised colour palette used across the entire UI.
 *
 * All colours are `raylib::Color` RGBA values.  Members are public and
 * intentionally not const so that a future theme-switcher can mutate them
 * at runtime.
 *
 * @ingroup Core
 */
struct Theme {
    Color bgApp = {14, 14, 20, 255};          ///< Main application background.
    Color bgSidebar = {10, 10, 16, 255};      ///< Sidebar and title-bar background.
    Color bgCard = {255, 255, 255, 13};       ///< Subtle card fill (5 % white).
    Color bgCard2 = {255, 255, 255, 20};      ///< Slightly brighter card fill.
    Color accent = {124, 58, 237, 255};       ///< Primary purple accent.
    Color accent2 = {168, 85, 247, 255};      ///< Lighter purple accent / hover state.
    Color accentGlow = {124, 58, 237, 128};   ///< Semi-transparent accent for glows.
    Color textPrimary = {232, 232, 240, 255}; ///< Main body text.
    Color textSub = {136, 136, 168, 255};     ///< Secondary / label text.
    Color textMuted = {74, 74, 106, 255};     ///< Disabled or hint text.
    Color borderCol = {255, 255, 255, 18};    ///< Subtle border (7 % white).
    Color green = {34, 197, 94, 255};         ///< Success / online indicator.
    Color red = {239, 68, 68, 255};           ///< Error / close button.
    Color yellow = {251, 191, 36, 255};       ///< Warning / wait indicator.
    Color bgOverlay = {8, 6, 16, 245};        ///< Near-opaque overlay background.
    Color transparent = {0, 0, 0, 0};         ///< Fully transparent helper.
};

/**
 * @brief Application-wide singleton rendering service.
 *
 * Wraps the raylib window, font management, and provides higher-level draw
 * helpers (rounded rectangles, glow effects, text variants).  Use
 * `Renderer::instance()` to obtain the singleton.
 *
 * The class is non-copyable and non-movable (Meyer's singleton pattern).
 *
 * @ingroup Core
 */
class Renderer {
  public:
    /**
     * @brief Access the singleton instance.
     * @return Reference to the unique Renderer object.
     */
    static Renderer &instance();

    /**
     * @brief Initialise the raylib window and load default fonts.
     *
     * Must be called exactly once before any draw operations.
     * Enables `FLAG_WINDOW_RESIZABLE`, `FLAG_MSAA_4X_HINT`, and
     * `FLAG_VSYNC_HINT`.  Sets the target FPS to 60.
     *
     * @param width   Initial window width in pixels.
     * @param height  Initial window height in pixels.
     * @param title   Window title string.
     */
    void init(int width, int height, const char *title);

    /**
     * @brief Unload fonts and close the raylib window.
     *
     * Called from Application::~Application().
     */
    void shutdown();

    /** @name Font accessors
     * Returns a mutable reference so callers can pass it directly to
     * raylib functions that expect `Font &`.
     * @{
     */
    Font &fontDisplay(); ///< Large display / heading font.
    Font &fontMono();    ///< Monospace font used for code and UI labels.
    Font &fontBody();    ///< Regular body / icon font.
    /** @} */

    /**
     * @brief Access the active colour theme.
     * @return Const reference to the Theme struct.
     */
    const Theme &theme() const;

    /**
     * @brief Current window width (accounts for resizing).
     * @return Width in pixels via raylib's `GetScreenWidth()`.
     */
    int width() const;

    /**
     * @brief Current window height (accounts for resizing).
     * @return Height in pixels via raylib's `GetScreenHeight()`.
     */
    int height() const;

    /** @name Text draw helpers
     * Thin wrappers around `DrawTextEx` with per-font kerning presets.
     * @{
     */

    /**
     * @brief Draw text with the display font.
     * @param text   UTF-8 string to render.
     * @param x      Top-left X position.
     * @param y      Top-left Y position.
     * @param size   Font size in pixels.
     * @param color  Text colour.
     */
    void drawTextDisplay(const std::string &text, float x, float y, float size, Color color);

    /**
     * @brief Draw text with the monospace font.
     * @param text   UTF-8 string to render.
     * @param x      Top-left X position.
     * @param y      Top-left Y position.
     * @param size   Font size in pixels.
     * @param color  Text colour.
     */
    void drawTextMono(const std::string &text, float x, float y, float size, Color color);

    /**
     * @brief Draw text with the body font.
     * @param text   UTF-8 string to render.
     * @param x      Top-left X position.
     * @param y      Top-left Y position.
     * @param size   Font size in pixels.
     * @param color  Text colour.
     */
    void drawTextBody(const std::string &text, float x, float y, float size, Color color);
    /** @} */

    /** @name Shape draw helpers
     * @{
     */

    /**
     * @brief Draw a filled rounded rectangle.
     *
     * The corner radius is normalised against the larger of @p rec.width and
     * @p rec.height so the value behaves consistently regardless of the rect
     * dimensions.
     *
     * @param rec    Bounding rectangle.
     * @param radius Corner radius in pixels (pre-normalisation).
     * @param color  Fill colour.
     */
    void drawRoundedRect(Rectangle rec, float radius, Color color);

    /**
     * @brief Draw the outline of a rounded rectangle.
     * @param rec    Bounding rectangle.
     * @param radius Corner radius in pixels (pre-normalisation).
     * @param thick  Line thickness in pixels.
     * @param color  Stroke colour.
     */
    void drawRoundedRectLines(Rectangle rec, float radius, float thick, Color color);

    /**
     * @brief Draw a multi-layer radial glow effect.
     *
     * Renders @p steps concentric semi-transparent circles from the outermost
     * (most transparent) to the innermost (most opaque), producing a soft glow
     * centred on (@p cx, @p cy).
     *
     * @param cx    Centre X in screen coordinates.
     * @param cy    Centre Y in screen coordinates.
     * @param r     Base glow radius in pixels.
     * @param color Base colour (alpha is overridden per layer).
     * @param steps Number of concentric rings (default: 6).
     */
    void drawGlow(float cx, float cy, float r, Color color, int steps = 6);
    /** @} */

  private:
    Renderer() = default;
    ~Renderer() = default;
    Renderer(const Renderer &) = delete;
    Renderer &operator=(const Renderer &) = delete;

    Font m_fontDisplay{}; ///< Display / heading font.
    Font m_fontMono{};    ///< Monospace font.
    Font m_fontBody{};    ///< Body / icon font.
    Theme m_theme;        ///< Active colour theme.
    int m_width = 0;      ///< Cached initial width (use GetScreenWidth() for current).
    int m_height = 0;     ///< Cached initial height (use GetScreenHeight() for current).
};

} // namespace zappy