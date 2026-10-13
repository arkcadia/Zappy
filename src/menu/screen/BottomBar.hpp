/**
 * @file BottomBar.hpp
 * @brief Fixed bottom status bar with simulation progress and pause control.
 *
 * @details The BottomBar is a 72 px tall strip anchored to the bottom edge of
 * the window.  It is always visible and drawn on top of all page and overlay
 * content (rendered last in Application::render()).
 *
 * @par Visual layout (left → right)
 * ```
 * ┌──────────────────────────────────────────────────────────────────────┐
 * │ [SERVER v1.0]  <status text>  ══════════════════  42%  [||/▶]       │
 * │                               progress track                         │
 * └──────────────────────────────────────────────────────────────────────┘
 * HEIGHT = 72 px
 * ```
 *
 * | Element         | Description                                             |
 * |-----------------|---------------------------------------------------------|
 * | Server badge    | Rounded pill "SERVER v1.0" – static label                |
 * | Status text     | Derived from LaunchManager and AppState (see below)      |
 * | Progress track  | 5 px gradient bar reflecting AppState::progress [0, 1]  |
 * | Percentage      | Integer percentage or "--" when progress == 0            |
 * | Pause button    | Toggles AppState::paused; icon alternates "||" / ">"    |
 *
 * @par Status text logic
 * | Condition                                    | Text                  |
 * |----------------------------------------------|-----------------------|
 * | LaunchManager not running                    | "Ready"               |
 * | LaunchManager running, not complete          | "Launching..."        |
 * | Complete + paused                            | "Paused"              |
 * | Complete + progress == 1                     | "Simulation complete" |
 * | Complete + in progress                       | "Running..."          |
 *
 * @ingroup Screens
 */

#pragma once
#include "../app/AppState.hpp"

namespace zappy {

/**
 * @brief Always-visible bottom chrome bar providing status and progress feedback.
 *
 * Owned by Application; updated and drawn on every frame regardless of the
 * active page or overlay.
 *
 * @ingroup Screens
 */
class BottomBar {
  public:
    /**
     * @brief Handle pause-button click input.
     *
     * Computes the pause button rectangle based on the live screen width and
     * toggles AppState::paused when the left mouse button is pressed inside it.
     *
     * @param state Mutable application state; `paused` may be toggled.
     */
    void update(AppState &state);

    /**
     * @brief Render the full bottom bar.
     *
     * Draw order:
     * 1. Solid near-opaque background strip `{8, 8, 14, 242}`.
     * 2. 1 px top border in `Theme::borderCol`.
     * 3. "SERVER v1.0" badge (rounded pill).
     * 4. Status text string (derived at runtime).
     * 5. Progress track background (5 px rounded rect).
     * 6. Gradient fill rect proportional to AppState::progress.
     * 7. Sub-label row ("Processes active" / "Waiting…", right-aligned component names).
     * 8. Percentage label.
     * 9. Pause / resume button with hover highlight.
     *
     * @param state Read-only application state.
     */
    void draw(const AppState &state) const;

    /// @brief Fixed pixel height of the bottom bar.
    static constexpr float HEIGHT = 72.0f;
};

} // namespace zappy