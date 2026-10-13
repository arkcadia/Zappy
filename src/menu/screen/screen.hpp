/**
 * @file screen.hpp
 * @brief Abstract base interface shared by all full-page screens.
 *
 * @details Every class that occupies the main content area of the launcher
 * window should derive from Screen and implement update() and draw().
 *
 * The interface is intentionally minimal: it enforces the
 * update → draw contract without dictating layout, ownership, or
 * rendering strategy.  Concrete screens own their own widgets and
 * compute their own geometry each frame based on the live window size.
 *
 * @par Inheritance diagram
 * ```
 * Screen (abstract)
 * └── HomeScreen
 * ```
 * Non-page overlays (SettingsPanel, LaunchModal) and chrome bars
 * (TitleBar, Sidebar, BottomBar) do **not** inherit from Screen because
 * they are not full-page replacements – they layer on top of or beside
 * the active page.
 *
 * @ingroup Screens
 */

#pragma once
#include "../app/AppState.hpp"

namespace zappy {

/**
 * @brief Pure-virtual interface for full-page content screens.
 *
 * Screens are stored by value inside Application and called each frame
 * in the order: update() (all chrome + active page/overlay), then
 * render() (all chrome + active page/overlay).
 *
 * @ingroup Screens
 */
class Screen {
  public:
    /// @brief Virtual destructor – required for correct polymorphic deletion.
    virtual ~Screen() = default;

    /**
     * @brief Process one frame of input and drive state transitions.
     *
     * Implementations should read raylib input functions (IsMouseButtonPressed,
     * GetMousePosition, etc.) and call AppState helpers to navigate or open
     * overlays.  No drawing may occur here.
     *
     * @param state Mutable application state.
     */
    virtual void update(AppState &state) = 0;

    /**
     * @brief Submit draw calls for the current frame.
     *
     * Must be called between raylib's BeginDrawing() / EndDrawing() pair
     * (guaranteed by Application::render()).  Implementations must not mutate
     * @p state.
     *
     * @param state Read-only application state.
     */
    virtual void draw(const AppState &state) = 0;
};

} // namespace zappy