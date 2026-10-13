#pragma once
#include <cstdint>

/**
 * @file utils.hpp
 * @brief Miscellaneous utility functions shared across the GUI.
 */

/**
 * @namespace Utils
 * @brief Stateless helper functions used by multiple subsystems.
 */
namespace Utils {

/**
 * @brief Computes the largest tile size that fits the grid inside the usable
 *        window area without exceeding the clamping bounds.
 *
 * The usable area is defined as 90% of Config::WINDOW_WIDTH horizontally and
 * 85% of Config::WINDOW_HEIGHT vertically. The tile size is the floor of the
 * minimum of the two scale factors, then clamped to [8, 120].
 *
 * @param width  Number of tile columns in the grid.
 * @param height Number of tile rows in the grid.
 * @return Recommended tile side length in pixels, clamped to [8, 120].
 */
uint16_t calculateTileSize(uint16_t width, uint16_t height);

} // namespace Utils
