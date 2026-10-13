#pragma once
#include <cstdint>

/**
 * @file config.hpp
 * @brief Compile-time configuration constants for the Zappy GUI.
 *
 * All visual, layout, and asset path settings live here. Changing a value in
 * this file propagates automatically to every subsystem that includes it.
 */

/**
 * @namespace Config
 * @brief Global compile-time constants shared across all GUI subsystems.
 */
namespace Config {

/** @name Window settings */
///@{
constexpr uint16_t WINDOW_WIDTH = 1920;  ///< Default window width in pixels.
constexpr uint16_t WINDOW_HEIGHT = 1080; ///< Default window height in pixels.
constexpr uint16_t TARGET_FPS = 120;     ///< Vsync target frame rate.
constexpr const char *WINDOW_TITLE = "ZAPPY — MISSION CONTROL"; ///< Title bar string.
constexpr bool VSYNC_ENABLED = true; ///< Enable vertical synchronisation.
constexpr bool RESIZABLE = true;     ///< Allow window resizing at runtime.
///@}

/** @name Camera / zoom */
///@{
constexpr float MIN_ZOOM = 0.2f;   ///< Minimum camera zoom factor.
constexpr float MAX_ZOOM = 6.0f;   ///< Maximum camera zoom factor.
constexpr float ZOOM_STEP = 0.12f; ///< Zoom delta applied per mouse-wheel tick.
constexpr int BASE_TILE_SIZE = 52; ///< Default tile size in pixels at zoom 1.
///@}

/** @name Layout geometry */
///@{
constexpr int PANEL_W = 280; ///< Width of the left and right side panels.
constexpr int HUD_H = 48;    ///< Height of the top HUD bar.
constexpr int STRIP_H = 28;  ///< Height of the bottom controls strip.
///@}

/** @name Asset paths */
///@{
constexpr const char *BG_IMAGE_PATH = "assets/background.png";
constexpr const char *FONT_PATH = "assets/fonts/rostex/Rostex-Regular.ttf";
///@}

/**
 * @brief Simple RGBA colour value.
 * Mirrors raylib Color but usable before raylib headers are included.
 */
struct Color {
    uint8_t r; ///< Red channel   [0-255].
    uint8_t g; ///< Green channel [0-255].
    uint8_t b; ///< Blue channel  [0-255].
    uint8_t a; ///< Alpha channel [0-255].
};

/** @name Background colours */
///@{
constexpr Color BG_COLOR = {3, 4, 10, 255};       ///< Window clear colour.
constexpr Color BG_PANEL = {6, 7, 18, 240};       ///< Side panel background.
constexpr Color TILE_FILL = {8, 10, 22, 80};      ///< Even tile fill.
constexpr Color TILE_FILL_ALT = {10, 13, 28, 80}; ///< Odd tile fill.
///@}

/** @name Accent colours */
///@{
constexpr Color ACCENT_CYAN = {40, 180, 255, 255};   ///< Primary highlight.
constexpr Color ACCENT_RED = {255, 60, 40, 255};     ///< Selected tile / danger.
constexpr Color ACCENT_ORANGE = {255, 120, 20, 255}; ///< Warning / event log.
constexpr Color ACCENT_GREEN = {0, 255, 160, 255};   ///< Connected status.
constexpr Color ACCENT_AMBER = {255, 184, 0, 255};   ///< Statistics values.
///@}

/** @name Text colours */
///@{
constexpr Color TEXT_PRIMARY = {190, 215, 245, 255}; ///< Main readable text.
constexpr Color TEXT_DIM = {80, 120, 170, 255};      ///< De-emphasised text.
constexpr Color TEXT_GHOST = {40, 60, 100, 255};     ///< Placeholder / hint text.
///@}

/** @name Grid and border colours */
///@{
constexpr Color GRID_LINE = {60, 30, 80, 45};    ///< Default tile border.
constexpr Color GRID_HOVER = {255, 80, 40, 160}; ///< Hovered tile border.
constexpr Color HUD_BG = BG_PANEL;               ///< HUD bar background alias.
constexpr Color HUD_BORDER = {80, 40, 140, 70};  ///< Panel separator colour.
///@}

/** @name Resource colours */
///@{
constexpr Color RES_FOOD = {255, 210, 60, 255};     ///< Food.
constexpr Color RES_LINEMATE = {80, 200, 120, 255}; ///< Linemate.
constexpr Color RES_DERAUMERE = {255, 60, 60, 255}; ///< Deraumere.
constexpr Color RES_SIBUR = {60, 130, 255, 255};    ///< Sibur.
constexpr Color RES_MENDIANE = {200, 60, 255, 255}; ///< Mendiane.
constexpr Color RES_PHIRAS = {255, 200, 60, 255};   ///< Phiras.
constexpr Color RES_THYSTAME = {60, 220, 255, 255}; ///< Thystame.
///@}

} // namespace Config
