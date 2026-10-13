/**
 * @file LaunchModal.hpp
 * @brief Animated modal overlay displaying the scripted launch-sequence log.
 *
 * @details LaunchModal is shown as a centred 540 × 300 px dialog when
 * `AppState::currentOverlay == Overlay::LaunchModal`.  It drives
 * LaunchManager::update() and renders the timed log entries as they
 * become visible, giving the user step-by-step visual feedback while
 * the three Zappy processes are (simulated as) starting up.
 *
 * @par State machine
 * ```
 *  [Settings confirmed]
 *        │
 *        ▼
 *  LaunchManager::startSequence()
 *        │
 *        ▼  (each frame)
 *  LaunchModal::update() ──► LaunchManager::update(dt)
 *        │                        │
 *        │                        ▼  (when Done step visible)
 *        │                   isComplete() == true
 *        │                        │
 *        ▼                        ▼
 *  progress ticks 0→1    [CLOSE] button appears
 *        │
 *        ▼  (user clicks CLOSE or backdrop)
 *  state.closeOverlay()
 *  LaunchManager::reset()
 * ```
 *
 * @par Progress coupling
 * Once `LaunchManager::isComplete()` returns true, LaunchModal sets
 * `AppState::progressRunning = true` and increments `AppState::progress`
 * by `GetFrameTime() * 0.04f` per frame (~25 seconds to reach 1.0),
 * unless `AppState::paused` is true.
 *
 * @par Dismiss behaviour
 * The overlay can be dismissed only after the sequence is complete, via:
 * - The "CLOSE" button rendered inside the modal.
 * - A left-click anywhere outside the modal rectangle.
 *
 * @ingroup Screens
 */

#pragma once
#include "../app/AppState.hpp"

namespace zappy {

/**
 * @brief Launch-sequence log modal overlay.
 *
 * Does **not** inherit from Screen because it is an overlay, not a
 * full-page replacement.  Owned by Application and only updated/drawn
 * when AppState::isOverlay(Overlay::LaunchModal) is true.
 *
 * @ingroup Screens
 */
class LaunchModal {
  public:
    /**
     * @brief Advance the launch sequence, update progress, and handle dismiss input.
     *
     * Per-frame responsibilities:
     * 1. Calls `LaunchManager::instance().update(GetFrameTime())`.
     * 2. When the sequence completes: initialises `state.progressRunning`
     *    and increments `state.progress` (unless paused).
     * 3. Clamps `state.progress` to 1.0 and stops `progressRunning` on completion.
     * 4. Handles the "CLOSE" button click (bottom-right of the modal).
     * 5. Handles a backdrop click (outside the modal rect) when complete.
     *
     * @param state Mutable application state; advances progress, may call
     *              closeOverlay() and LaunchManager::reset().
     */
    void update(AppState &state);

    /**
     * @brief Render the modal backdrop, panel, log entries, and close button.
     *
     * Draw order:
     * 1. Full-screen semi-transparent black backdrop `{0, 0, 0, 204}`.
     * 2. Centred modal panel (540 × 300, rounded corners, accent border).
     * 3. Three outer glow rings (expanding rounded-rect outlines, fading).
     * 4. "// LAUNCH SEQUENCE" header and horizontal rule.
     * 5. Visible log entries (colour and prefix icon per LaunchStep::Type).
     * 6. "CLOSE" button (only when LaunchManager::isComplete() is true).
     *
     * @par Log entry colour coding
     * | Type  | Prefix | Colour              |
     * |-------|--------|---------------------|
     * | Info  | `  `   | `Theme::textSub`    |
     * | Ok    | `✓ `   | Green `{74,222,128}` |
     * | Wait  | `○ `   | Yellow `{251,191,36}`|
     * | Done  | `★ `   | Purple `{167,139,250}`|
     *
     * @param state Read-only application state (progress and paused used by
     *              BottomBar, not directly by this draw call).
     */
    void draw(const AppState &state) const;
};

} // namespace zappy