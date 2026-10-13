#pragma once
#include <cstdint>
#include <memory>
#include <raylib.h>
#include <string>
#include <vector>

/**
 * @file graphical.hpp
 * @brief Main graphical client class and related data structures.
 *
 * Graphical owns the complete rendering pipeline, input handling, camera
 * management, network message processing, and world state management for
 * the Zappy GUI client.
 *
 * The implementation is split across four translation units:
 * - graphical.cpp          — lifecycle, server protocol, statistics
 * - graphical_renderer.cpp — all draw calls
 * - graphical_camera.cpp   — camera math and mouse/scroll input
 * - graphical_input.cpp    — keyboard input and tile selection
 */

class Window;
class World;
class ResourceRenderer;

namespace Network {
class NetworkClient;
}

/**
 * @brief Stores the 2-D pan/zoom camera state.
 *
 * Offset values are in screen pixels and represent the screen position of tile
 * (0, 0). The zoom factor scales the base tile size linearly.
 */
struct Camera2DState {
    float offsetX = 0.0f;     ///< Screen X of tile (0,0) in pixels.
    float offsetY = 0.0f;     ///< Screen Y of tile (0,0) in pixels.
    float zoom = 1.0f;        ///< Current zoom factor; 1.0 = Config::BASE_TILE_SIZE px per tile.
    bool dragging = false;    ///< True while a pan drag is in progress.
    float dragStartX = 0.0f;  ///< Mouse X at the start of the current drag.
    float dragStartY = 0.0f;  ///< Mouse Y at the start of the current drag.
    float dragOriginX = 0.0f; ///< Camera offsetX at the start of the current drag.
    float dragOriginY = 0.0f; ///< Camera offsetY at the start of the current drag.
};

/**
 * @brief A single timestamped line in the HUD event log.
 *
 * Alpha starts at 6.0 and fades at 0.18 per second; the entry is removed
 * from the log vector once alpha reaches zero.
 */
struct LogEntry {
    std::string text; ///< Message text displayed in the right panel.
    float alpha;      ///< Fade value in [0, 6]; drives render opacity.
};

enum class InteractionAnimationType {
    Spawn,
    Move,
    Broadcast,
    Pickup,
    Drop,
    Eject,
    Fork,
    EggLaid,
    EggHatch,
    EggDeath,
    IncantationStart,
    IncantationEnd,
    Death,
    LevelUp,
};

struct InteractionAnimation {
    InteractionAnimationType type = InteractionAnimationType::Spawn;
    float x = 0.0f;
    float y = 0.0f;
    float startedAt = 0.0f;
    float duration = 0.8f;
    int resourceIndex = -1;
    int playerId = -1;
    bool success = true;
    std::string teamName;
    std::string label;
};

/**
 * @brief Top-level graphical client.
 *
 * Graphical owns every subsystem needed to run the GUI: the raylib Window,
 * the World grid, the ResourceRenderer, camera state, and the event log.
 * It also holds the background Texture2D and the custom Font loaded from
 * Config::BG_IMAGE_PATH and Config::FONT_PATH respectively.
 *
 * ### Main loop
 * run() drives a fixed-structure loop:
 * 1. handleInput()  — keyboard, mouse, camera
 * 2. update(dt)     — time advance, log fade, network poll
 * 3. render()       — full frame draw
 *
 * ### Network protocol
 * handleServerMessage() reacts to the following Zappy server commands:
 * - `msz w h`        — initialises the world grid
 * - `bct x y r0..r6` — sets resource counts on one tile
 * - `sgt t`          — logs the tick frequency
 * - `seg`            — logs end-of-game
 */
class Graphical {
  public:
    /**
     * @brief Constructs the client, opens the window, and loads assets.
     *
     * Loads Config::BG_IMAGE_PATH as a texture (silently skipped if missing)
     * and Config::FONT_PATH as a 32pt font (falls back to raylib default if
     * missing). The world pointer is null until the server sends `msz`.
     *
     * @param networkClient Non-owning reference to the already-connected client.
     */
    explicit Graphical(Network::NetworkClient &networkClient);

    /**
     * @brief Unloads the background texture and font, then destroys all members.
     */
    ~Graphical();

    /**
     * @brief Enters the main loop and blocks until the window is closed.
     *
     * Returns only when the window close button is pressed or ESC is hit.
     */
    void run();

    /**
     * @brief Returns a mutable reference to the world grid.
     * @pre The server must have sent `msz` before this is called.
     */
    World &getWorld() { return *m_world; }

    /**
     * @brief Returns a const reference to the world grid.
     * @pre The server must have sent `msz` before this is called.
     */
    const World &getWorld() const { return *m_world; }

  private:
    Network::NetworkClient &m_networkClient; ///< Reference to the TCP client (not owned).
    std::unique_ptr<Window> m_window;        ///< Raylib window wrapper.
    std::unique_ptr<World> m_world;          ///< Game grid; null until msz is received.
    std::unique_ptr<ResourceRenderer> m_resourceRenderer; ///< Resource dot renderer.

    bool m_running;        ///< Loop guard; set to false by ESC or window close.
    bool m_mapReceived;    ///< True once the server has sent a valid `msz` command.
    uint16_t m_gridWidth;  ///< Grid width in tiles received from the server.
    uint16_t m_gridHeight; ///< Grid height in tiles received from the server.
    int m_tileSize;        ///< Current tile size in pixels at zoom 1.

    Camera2DState m_camera;      ///< Pan/zoom camera state.
    std::vector<LogEntry> m_log; ///< Fading event log entries (max 24).
    std::vector<InteractionAnimation> m_interactionAnimations;
    float m_time;                ///< Elapsed time in seconds, updated each frame.

    int m_hoveredTileX;  ///< Grid X of the tile under the cursor, or -1 when none.
    int m_hoveredTileY;  ///< Grid Y of the tile under the cursor, or -1 when none.
    int m_selectedTileX; ///< Grid X of the keyboard-selected tile.
    int m_selectedTileY; ///< Grid Y of the keyboard-selected tile.
    bool m_hasSelection; ///< True once a tile has been selected.

    int m_mapX; ///< Screen X of the map viewport left edge.
    int m_mapY; ///< Screen Y of the map viewport top edge.
    int m_mapW; ///< Width  of the map viewport in pixels.
    int m_mapH; ///< Height of the map viewport in pixels.

    int m_totalFood;     ///< Aggregate food count across all tiles.
    int m_totalMinerals; ///< Aggregate mineral count (all non-food resources) across all tiles.
    int m_filledTiles;   ///< Number of tiles containing at least one resource.

    Texture2D m_bgTexture; ///< Background image texture; valid only when m_bgLoaded is true.
    bool m_bgLoaded;       ///< True when the background texture was loaded successfully.
    Font m_font;           ///< Custom display font; valid only when m_fontLoaded is true.
    bool m_fontLoaded;     ///< True when the custom font was loaded successfully.

    /** @name Core loop (graphical.cpp) */
    ///@{
    void handleInput();
    void update(float dt);
    void processNetworkMessages();
    void handleServerMessage(const std::string &msg);
    void initializeWorld(uint16_t w, uint16_t h);
    void addInteractionAnimation(InteractionAnimationType type, float x, float y,
                                 const std::string &teamName = "", int resourceIndex = -1,
                                 bool success = true, const std::string &label = "",
                                 int playerId = -1, float duration = 0.8f);
    void updateInteractionAnimations();
    void render();
    void updateMapStats();
    ///@}

    /** @name Layout (graphical_renderer.cpp) */
    ///@{
    /**
     * @brief Recomputes m_mapX/Y/W/H from the current window size.
     *
     * Called at the start of every render() to support window resizing.
     */
    void initLayout();
    ///@}

    /** @name Chrome draw calls (graphical_renderer.cpp) */
    ///@{
    void drawHUDBar();        ///< Draws the top status bar with chips and FPS counter.
    void drawLeftPanel();     ///< Draws the resource legend and tile inspector panel.
    void drawRightPanel();    ///< Draws the event log and map statistics panel.
    void drawControlsStrip(); ///< Draws the bottom key-binding hint strip.
    void drawWaitingScreen(); ///< Draws the "awaiting server" splash in the map area.
    ///@}

    /** @name Map viewport draw calls (graphical_renderer.cpp) */
    ///@{
    /**
     * @brief Draws the background image or fallback colour into a screen rectangle.
     * @param x Left edge in screen pixels.
     * @param y Top edge in screen pixels.
     * @param w Width in screen pixels.
     * @param h Height in screen pixels.
     */
    void drawBackground(int x, int y, int w, int h);

    /**
     * @brief Draws the full map viewport: background, grid, edges, and resources.
     *
     * Uses BeginScissorMode / EndScissorMode to clip to the map rectangle so
     * panel overlaps are never visible.
     */
    void drawMapViewport();

    /**
     * @brief Draws all visible tiles with fill, border, hover, and selection effects.
     *
     * Iterates only over tiles whose screen rectangle intersects the window;
     * off-screen tiles are skipped entirely.
     */
    void drawGrid();

    /**
     * @brief Delegates per-tile resource dot rendering to ResourceRenderer.
     */
    void drawResources();
    void drawInteractionAnimations();

    /**
     * @brief Draws animated edge markers and "WRAPS" labels at the grid boundary.
     *
     * The top/right edges glow red and the bottom/left edges glow blue to
     * reflect the background image's asymmetric lighting.
     */
    void drawToroidalEdges();
    ///@}

    /** @name Helpers */
    ///@{
    /**
     * @brief Appends a new entry to the event log, capping the list at 24 entries.
     * @param text Message string to display.
     */
    void pushLog(const std::string &text);

    /**
     * @brief Processes mouse wheel, RMB pan drag, and tile hover each frame.
     */
    void updateCamera();

    /**
     * @brief Moves the keyboard selection one tile in the pressed direction,
     *        wrapping toroidally at grid edges.
     */
    void handleArrowKeys();

    /**
     * @brief Resets camera offset so the grid is centred, and zoom to 1.0.
     */
    void resetCamera();

    /**
     * @brief Converts a world-space tile coordinate to screen pixels.
     * @param wx Tile column (may be fractional).
     * @param wy Tile row    (may be fractional).
     * @return Screen position in pixels.
     */
    Vector2 worldToScreen(float wx, float wy) const;

    /**
     * @brief Converts a screen pixel position to a world-space tile coordinate.
     * @param sx Screen X in pixels.
     * @param sy Screen Y in pixels.
     * @return Tile coordinate (fractional; floor to get the tile index).
     */
    Vector2 screenToWorld(float sx, float sy) const;
    ///@}
};
