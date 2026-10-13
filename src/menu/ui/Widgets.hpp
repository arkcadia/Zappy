/**
 * @file Widgets.hpp
 * @brief Reusable immediate-mode UI widgets for the Zappy Launcher.
 *
 * @details All widgets follow a simple pattern:
 * - **Data** members hold geometry, text, and state.
 * - **update()** processes mouse/keyboard input and returns an interaction
 *   event (typically `bool pressed`).
 * - **draw() const** renders the widget at its current position.
 *
 * Widgets do **not** own their lifetime or drive their own layout; the
 * owning screen/panel is responsible for setting `bounds` each frame before
 * calling update() and draw().
 *
 * @ingroup UI
 */

#pragma once
#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

namespace zappy::ui {

// ─────────────────────────────────────────────────────────────────────────────
// Button
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Visual style parameters for a Button widget.
 *
 * Separating style from behaviour allows the same Button struct to be
 * rendered as a primary CTA, a ghost action, or an icon button by simply
 * swapping the style.
 *
 * @ingroup UI
 */
struct ButtonStyle {
    Color bgNormal = {124, 58, 237, 255};   ///< Background when idle.
    Color bgHover = {109, 40, 217, 255};    ///< Background when the cursor is over the button.
    Color bgActive = {91, 33, 182, 255};    ///< Background while the mouse button is held.
    Color textColor = {255, 255, 255, 255}; ///< Label text colour.
    Color borderColor = {0, 0, 0, 0};       ///< Border colour (alpha 0 = no border).
    float borderThick = 0.0f;               ///< Border thickness in pixels.
    float radius = 6.0f;                    ///< Corner radius in pixels.
    float fontSize = 13.0f;                 ///< Label font size in pixels.
    float letterSpacing = 1.2f;             ///< Letter spacing passed to `DrawTextEx`.
};

/**
 * @brief A clickable labelled button with hover / pressed state tracking.
 *
 * @par Example
 * @code
 * ui::Button btn;
 * btn.label  = "LAUNCH";
 * btn.bounds = {32, 100, 140, 44};
 * btn.style  = ui::ButtonStyle{};  // default primary style
 *
 * // Each frame:
 * if (btn.update()) {
 *     // button was clicked this frame
 * }
 * btn.draw();
 * @endcode
 *
 * @ingroup UI
 */
struct Button {
    Rectangle bounds = {}; ///< Screen-space bounding rectangle (set each frame by owner).
    std::string label;     ///< Text displayed centred inside the button.
    ButtonStyle style;     ///< Visual appearance settings.
    bool hovered = false;  ///< True when the cursor is inside `bounds`.
    bool pressed = false;  ///< True on the frame the button is clicked.

    /**
     * @brief Process mouse input and update hover/pressed state.
     * @return true on the frame the left mouse button is released inside `bounds`.
     */
    bool update();

    /**
     * @brief Render the button at its current `bounds`.
     *
     * Draws background, optional border, a left-side shimmer on hover,
     * and the centred monospace label.
     */
    void draw() const;

    /**
     * @brief Create a ghost (transparent-background) button style.
     *
     * Suitable for secondary actions such as *Cancel*.
     *
     * @return Pre-configured ButtonStyle for ghost buttons.
     */
    static ButtonStyle ghostStyle();

    /**
     * @brief Create a compact icon-button style.
     *
     * Square corners, subtle border, accent hover – suitable for
     * single-character icon buttons (e.g. the Settings "⚙" shortcut).
     *
     * @return Pre-configured ButtonStyle for icon buttons.
     */
    static ButtonStyle iconStyle();
};

// ─────────────────────────────────────────────────────────────────────────────
// TextInput
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Single-line text input field with optional numeric-only mode.
 *
 * Focus must be managed externally (e.g. by SettingsPanel::handleFocusClick()).
 * When `focused` is true, update() reads characters from raylib's char queue.
 *
 * @ingroup UI
 */
struct TextInput {
    Rectangle bounds = {};   ///< Screen-space bounding rectangle.
    std::string value;       ///< Current text content.
    std::string placeholder; ///< Hint text displayed when `value` is empty.
    std::string label;       ///< Human-readable label drawn above the field.
    std::string flag;        ///< CLI flag associated with this field (e.g. "-p").
    bool focused = false;    ///< Whether this field has keyboard focus.
    bool numeric = false;    ///< If true, only digits and '-' are accepted.
    int maxLength = 64;      ///< Maximum number of characters allowed.
    float fontSize = 13.0f;  ///< Text rendering size in pixels.

    /**
     * @brief Process keyboard input when focused.
     *
     * Consumes all pending `GetCharPressed()` events and handles
     * `KEY_BACKSPACE`.  Does nothing when `focused` is false.
     */
    void update();

    /**
     * @brief Render the input field.
     *
     * Draws background, border (accent-coloured when focused), the current
     * value or placeholder, and a blinking cursor when focused.
     * Content is scissor-clipped to `bounds`.
     */
    void draw() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// NavItem
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A sidebar navigation item with icon, label, and active/hover state.
 *
 * Used by Sidebar to build its navigation entries.
 *
 * @ingroup UI
 */
struct NavItem {
    std::string label;     ///< Navigation label text (upper-case, monospace).
    std::string icon;      ///< Single-character or short icon string (e.g. "[H]").
    bool active = false;   ///< Whether this item represents the current page.
    bool hovered = false;  ///< True when the cursor is inside `bounds`.
    Rectangle bounds = {}; ///< Screen-space bounding rectangle.

    /**
     * @brief Process mouse input and update hovered state.
     * @return true on the frame the item is clicked.
     */
    bool update();

    /**
     * @brief Render the navigation item.
     *
     * Draws a full-width background highlight when active or hovered,
     * a 3 px left accent bar when active, the icon, and the label.
     */
    void draw() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ProgressBar
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A horizontal progress bar with gradient fill and glow.
 * @ingroup UI
 */
struct ProgressBar {
    Rectangle bounds = {};                 ///< Screen-space bounding rectangle.
    float value = 0.0f;                    ///< Fill ratio in [0, 1].
    Color fillStart = {124, 58, 237, 255}; ///< Gradient start colour (left).
    Color fillEnd = {168, 85, 247, 255};   ///< Gradient end colour (right).
    Color bgColor = {255, 255, 255, 15};   ///< Track background colour.
    float radius = 3.0f;                   ///< Corner radius of the track.

    /**
     * @brief Render the progress bar at its current `bounds` and `value`.
     *
     * The fill colour is linearly interpolated between `fillStart` and
     * `fillEnd` based on `value`, with a secondary semi-transparent glow pass.
     */
    void draw() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Panel
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A plain rounded-rectangle panel (background + border).
 * @ingroup UI
 */
struct Panel {
    Rectangle bounds = {};               ///< Screen-space bounding rectangle.
    Color bgColor = {255, 255, 255, 13}; ///< Fill colour.
    Color border = {255, 255, 255, 18};  ///< Border colour.
    float radius = 8.0f;                 ///< Corner radius in pixels.

    /**
     * @brief Render the panel (fill + 1 px border).
     */
    void draw() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// ScrollView
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A scissor-clipped, mouse-wheel-scrollable content container.
 *
 * @par Usage
 * @code
 * m_scroll.viewport      = {panelX, panelY, panelW, panelH};
 * m_scroll.contentHeight = 820.f;
 * m_scroll.update();                    // consume mouse-wheel input
 *
 * m_scroll.beginClip();                 // starts scissor clipping
 * drawScrolledContent(panelY - m_scroll.getOffset());
 * m_scroll.endClip();                   // ends scissor, draws scrollbar thumb
 * @endcode
 *
 * @ingroup UI
 */
struct ScrollView {
    Rectangle viewport = {};    ///< Clipping rectangle in screen space.
    float contentHeight = 0.0f; ///< Total height of the scrollable content in pixels.
    float scrollOffset = 0.0f;  ///< Current vertical scroll position in pixels.

    /**
     * @brief Consume mouse-wheel events within `viewport` and clamp the offset.
     *
     * 30 pixels per wheel notch.  Only reacts when the cursor is inside
     * `viewport`.
     */
    void update();

    /**
     * @brief Begin the scissor clip region.
     *
     * Must be paired with endClip().  All draw calls between beginClip() and
     * endClip() are clipped to `viewport`.
     */
    void beginClip() const;

    /**
     * @brief End the scissor clip region and draw the scrollbar thumb.
     *
     * Ends scissor mode and, if the content is taller than the viewport,
     * renders a thin purple scrollbar thumb on the right edge.
     */
    void endClip() const;

    /**
     * @brief Current scroll offset.
     * @return Vertical scroll amount in pixels (0 = top).
     */
    float getOffset() const;
};

// ─────────────────────────────────────────────────────────────────────────────
// Free-function helpers
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Draw a section title with a leading icon, a label, and a right-extending rule.
 *
 * @param icon   Short icon character(s) drawn in accent colour.
 * @param title  Section label drawn in muted monospace text.
 * @param x      Left edge of the section title row.
 * @param y      Top edge of the section title row.
 * @param width  Total available width; the rule extends to x + width.
 *
 * @ingroup UI
 */
void drawSectionTitle(const std::string &icon, const std::string &title, float x, float y,
                      float width);

/**
 * @brief Draw a status badge indicating whether a binary was found.
 *
 * Renders a rounded pill with green (found) or red (not found) colouring.
 *
 * @param binary  Binary name to display.
 * @param found   Whether the binary exists on the file system.
 * @param x       Left edge.
 * @param y       Top edge.
 * @param width   Pill width.
 *
 * @ingroup UI
 */
void drawBinaryStatus(const std::string &binary, bool found, float x, float y, float width);

/**
 * @brief Draw a terminal-style command preview box.
 *
 * Renders each line with a purple `$` prompt, white binary name, purple
 * flags (`-x`), and green values, matching a typical shell presentation.
 *
 * @param lines   Vector of complete command strings.
 * @param x       Box left edge.
 * @param y       Box top edge.
 * @param width   Box width.
 * @param height  Box height.
 *
 * @ingroup UI
 */
void drawCmdPreview(const std::vector<std::string> &lines, float x, float y, float width,
                    float height);

/**
 * @brief Draw a field label with an optional CLI flag annotation.
 *
 * @param text  Human-readable label (e.g. "Port").
 * @param flag  CLI flag string (e.g. "-p"); empty to omit.
 * @param x     Label left edge.
 * @param y     Label top edge.
 *
 * @ingroup UI
 */
void drawLabel(const std::string &text, const std::string &flag, float x, float y);

} // namespace zappy::ui
