/**
 * @file TitleBar.hpp
 * @brief Top chrome bar containing window controls.
 *
 * @details TitleBar renders a 36 px tall horizontal bar across the full width
 * of the window, providing three simulated window-management buttons
 * (minimise, maximise, close) in the style of a custom-frame application.
 *
 * Only the **close** button triggers real behaviour: it sets
 * AppState::running to `false`, which causes Application::run() to exit the
 * main loop on the next iteration.  Minimise and maximise are visual-only
 * placeholders for future implementation.
 *
 * @par Layout
 * ```
 * ┌───────────────────────────────────────────────────────── [─] [□] [x] ┐
 * │ HEIGHT = 36 px                                                       │
 * └──────────────────────────────────────────────────────────────────────┘
 * ```
 * The three buttons are right-aligned at x = (sw − 88), (sw − 56), (sw − 24)
 * with a fixed width of 28 px and height of 22 px each.
 *
 * @ingroup Screens
 */

#pragma once
#include "../app/AppState.hpp"

namespace zappy {

/**
 * @brief Fixed top-chrome bar with decorative window controls.
 *
 * Owned by Application and drawn last-before-EndDrawing so it always
 * sits above page content (painter's algorithm order).  Unlike overlays,
 * TitleBar is always visible regardless of the active page or overlay.
 *
 * @ingroup Screens
 */
class TitleBar {
  public:
    /**
     * @brief Handle window-control button interaction.
     *
     * Tests the left mouse button against the close button rectangle
     * `{sw − 24, 7, 28, 22}`.  On click, sets AppState::running to `false`.
     * Minimise and maximise hit-tests are computed but produce no effect.
     *
     * @param state Mutable application state; `running` may be set to false.
     */
    void update(AppState &state);

    /**
     * @brief Render the title bar.
     *
     * Draw order:
     * 1. Solid background rectangle in `Theme::bgSidebar`.
     * 2. 1 px bottom border in `Theme::borderCol`.
     * 3. Three window-control buttons, each with:
     *    - Rounded fill (red tinted on close-hover, otherwise subtle white).
     *    - Rounded border stroke.
     *    - Centred icon label (`-`, `[]`, `x`) in the body font.
     */
    void draw() const;

    /// @brief Fixed pixel height of the title bar.
    static constexpr float HEIGHT = 36.0f;
};

} // namespace zappy