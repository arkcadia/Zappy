/**
 * @file AppState.hpp
 * @brief Application-wide state container and navigation enumerations.
 *
 * @details AppState is a plain aggregate that acts as a single source of truth
 * for the launcher GUI.  It is passed by reference from the top-level
 * Application into every screen and overlay, allowing each component to both
 * read the current state and trigger navigation or overlay transitions without
 * holding direct pointers to sibling components.
 *
 * @note Thread safety: AppState is only ever accessed from the main render
 * thread; no synchronisation is required.
 *
 * @ingroup Core
 */

#pragma once

namespace zappy {

/**
 * @brief Identifies the top-level page currently displayed in the content area.
 *
 * Pages are mutually exclusive.  Navigating to a page does **not** implicitly
 * close any active overlay – that must be done explicitly via
 * AppState::closeOverlay().
 */
enum class Page {
    Home,   ///< Landing / hero screen shown on startup.
    Launch, ///< Alias for the Settings overlay accessed via the sidebar item.
    Logs,   ///< Runtime log viewer (future expansion).
    Config  ///< Detailed configuration panel (maps to Settings overlay).
};

/**
 * @brief Identifies the modal overlay currently displayed above the page.
 *
 * At most one overlay is active at a time.  `None` means no overlay is shown.
 */
enum class Overlay {
    None,       ///< No overlay is active; the underlying page is fully interactive.
    Settings,   ///< Launch-configuration panel (SettingsPanel).
    LaunchModal ///< Animated process-launch log modal (LaunchModal).
};

/**
 * @brief Central, mutable state bag passed to every screen and overlay each frame.
 *
 * @details Screens call the helper methods (navigate(), openOverlay(), etc.)
 * to mutate state.  Renderers only read through const references.
 *
 * **Lifecycle of a typical launch:**
 * 1. User clicks *Launch All* → @ref openOverlay(Overlay::Settings)
 * 2. User confirms → Settings calls LaunchManager::startSequence() then
 *    @ref openOverlay(Overlay::LaunchModal)
 * 3. LaunchModal ticks @ref progress until 1.0
 * 4. User closes the modal → @ref closeOverlay()
 *
 * @ingroup Core
 */
struct AppState {
    Page currentPage = Page::Home;          ///< Page rendered in the content area.
    Overlay currentOverlay = Overlay::None; ///< Currently active modal overlay, or None.
    bool newsCardVisible = true;            ///< Whether the news card is shown on HomeScreen.
    bool paused = false;                    ///< Whether the simulation progress is paused.
    float progress = 0.0f;                  ///< Simulation progress in [0, 1].
    bool progressRunning = false;           ///< True while the progress bar is advancing.
    bool running = true;                    ///< Set to false to request application exit.

    /**
     * @brief Navigate to the given page without touching the active overlay.
     * @param page Target page to display.
     */
    void navigate(Page page) { currentPage = page; }

    /**
     * @brief Show the given overlay on top of the current page.
     * @param o Overlay to activate.
     */
    void openOverlay(Overlay o) { currentOverlay = o; }

    /**
     * @brief Dismiss the active overlay and return to bare-page rendering.
     */
    void closeOverlay() { currentOverlay = Overlay::None; }

    /**
     * @brief Check whether a specific overlay is currently active.
     * @param o Overlay to test.
     * @return true if @p o is the currently active overlay.
     */
    bool isOverlay(Overlay o) const { return currentOverlay == o; }
};

} // namespace zappy