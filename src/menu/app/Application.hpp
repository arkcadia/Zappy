/**
 * @file Application.hpp
 * @brief Top-level application controller for the Zappy Launcher.
 *
 * @details Application owns every top-level screen and overlay, orchestrates
 * the main loop, and delegates per-frame update/render calls to the active
 * components based on the current AppState.
 *
 * **Ownership hierarchy:**
 * ```
 * Application
 * ├── AppState          (state bag)
 * ├── TitleBar          (fixed top chrome)
 * ├── Sidebar           (fixed left navigation)
 * ├── BottomBar         (fixed bottom status / progress)
 * ├── HomeScreen        (page: Home)
 * ├── SettingsPanel     (overlay: Settings)
 * └── LaunchModal       (overlay: LaunchModal)
 * ```
 *
 * @ingroup Core
 */

#pragma once
#include "../screen/BottomBar.hpp"
#include "../screen/Homescreen.hpp"
#include "../screen/LaunchModal.hpp"
#include "../screen/SettingsPanel.hpp"
#include "../screen/Sidebar.hpp"
#include "../screen/TitleBar.hpp"
#include "AppState.hpp"
#include <memory>

namespace zappy {

/**
 * @brief Root application object – creates the window, runs the main loop,
 *        and tears everything down on destruction.
 *
 * @par Construction
 * The constructor initialises the raylib window (1100 × 700, resizable),
 * loads `zappy_config.json` via ConfigManager, and synchronises
 * SettingsPanel fields from the loaded config.
 *
 * @par Main loop
 * Application::run() spins until either the OS closes the window or
 * AppState::running is set to `false` (e.g. by the TitleBar close button).
 * Each iteration calls update() then render().
 *
 * @par Destruction
 * The destructor persists the current config back to `zappy_config.json`
 * and shuts down the Renderer singleton.
 *
 * @ingroup Core
 */
class Application {
  public:
    /**
     * @brief Construct the application, opening the raylib window.
     *
     * Calls Renderer::init(), ConfigManager::load(), and
     * SettingsPanel::syncFromConfig().
     */
    Application();

    /**
     * @brief Destroy the application, persisting config and closing the window.
     *
     * Calls ConfigManager::save() then Renderer::shutdown().
     */
    ~Application();

    /**
     * @brief Enter the main loop.
     *
     * Blocks until `WindowShouldClose()` is true **or** AppState::running is
     * `false`.  Each iteration: update() → render().
     */
    void run();

  private:
    /**
     * @brief Process one frame of input and state transitions.
     *
     * Dispatches update() calls to the chrome bars and the active
     * page / overlay according to AppState::currentOverlay.
     *
     * Overlay priority:
     * 1. LaunchModal (if active)
     * 2. Settings panel (if active); triggers LaunchManager::startSequence()
     *    when the Settings overlay itself requests a launch.
     * 3. HomeScreen (default)
     */
    void update();

    /**
     * @brief Submit one frame of draw calls to raylib.
     *
     * All drawing happens between BeginDrawing() / EndDrawing().
     * Draw order (painter's algorithm, back-to-front):
     * 1. Background clear to near-black `{5, 5, 8, 255}`
     * 2. TitleBar
     * 3. Sidebar
     * 4. HomeScreen
     * 5. SettingsPanel (if overlay active)
     * 6. LaunchModal (if overlay active)
     * 7. BottomBar (always on top)
     */
    void render();

    AppState m_state;              ///< Shared application state passed to all components.
    Sidebar m_sidebar;             ///< Left-hand navigation sidebar.
    TitleBar m_titleBar;           ///< Top chrome bar with window controls.
    BottomBar m_bottomBar;         ///< Bottom status / progress bar.
    HomeScreen m_homeScreen;       ///< Default landing page.
    SettingsPanel m_settingsPanel; ///< Launch-configuration overlay.
    LaunchModal m_launchModal;     ///< Animated launch-sequence log overlay.
};

} // namespace zappy