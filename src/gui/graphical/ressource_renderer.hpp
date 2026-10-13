#pragma once
#include "world.hpp"
#include <array>
#include <raylib.h>

/**
 * @file ressource_renderer.hpp
 * @brief Per-tile resource dot renderer.
 *
 * ResourceRenderer is responsible for the visual representation of every
 * resource type on the game grid. Each resource is drawn as an animated
 * glowing dot whose radius and opacity pulse over time.
 */

/**
 * @brief Holds the visual properties of a single resource type.
 */
struct ResourceVisual {
    ::Color color;      ///< Base RGBA colour used for the dot, glow, and ring.
    const char *label;  ///< Full name string (e.g. "FOOD").
    const char *symbol; ///< Single-character abbreviation (e.g. "F").
};

/**
 * @brief Renders resource dots onto individual tiles.
 *
 * Constructed once and reused every frame. The m_visuals array maps each
 * ResourceType index to its colour and label, initialised from Config constants
 * in the constructor.
 *
 * Rendering is split into two levels:
 * - drawResourcesOnTile() iterates over all resources on a tile and places
 *   each non-zero type in a grid layout inside the tile rectangle.
 * - drawResourceDot() draws the actual glow + fill + ring circles for one
 *   resource at a given screen position.
 */
class ResourceRenderer {
  public:
    /**
     * @brief Initialises the visual table from Config resource colour constants.
     */
    ResourceRenderer();

    /**
     * @brief Renders all non-zero resources present on a tile.
     *
     * Resources are arranged in a left-to-right, top-to-bottom dot grid that
     * fits inside the tile rectangle. Dots are skipped if they would extend
     * below the tile's bottom edge.
     *
     * @param px       Screen X of the tile's top-left corner.
     * @param py       Screen Y of the tile's top-left corner.
     * @param tileSize Tile side length in screen pixels (affected by zoom).
     * @param tile     Tile whose resources should be drawn.
     * @param time     Elapsed time in seconds, used to drive the pulse animation.
     * @param phase    Per-tile phase offset so adjacent tiles pulse out of sync.
     */
    void drawResourcesOnTile(int px, int py, int tileSize, const Tile &tile, float time,
                             float phase);

  private:
    std::array<ResourceVisual, static_cast<uint8_t>(ResourceType::Count)> m_visuals;
    ///< Visual properties indexed by ResourceType cast to uint8_t.

    /**
     * @brief Draws a single animated resource dot at the given position.
     *
     * Draws three concentric circles: a large semi-transparent glow, a filled
     * inner disk, and a bright ring. When amount > 1 and the dot is large
     * enough, the quantity is rendered as a centred text label.
     *
     * @param x      Screen X of the dot's top-left bounding square.
     * @param y      Screen Y of the dot's top-left bounding square.
     * @param size   Dot bounding square side length in pixels.
     * @param type   Resource type, used to look up colour and label.
     * @param amount Quantity on the tile; shown as text when > 1 and size >= 10.
     * @param pulse  Normalised animation value in [0, 1] that drives radius and alpha.
     */
    void drawResourceDot(int x, int y, int size, ResourceType type, uint16_t amount, float pulse);
};
