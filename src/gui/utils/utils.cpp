#include "utils.hpp"
#include "../graphical/config.hpp"

namespace Utils {

uint16_t calculateTileSize(uint16_t width, uint16_t height) {
    float usableW = static_cast<float>(Config::WINDOW_WIDTH) * 0.90f;
    float usableH = static_cast<float>(Config::WINDOW_HEIGHT) * 0.85f;

    float scaleX = usableW / static_cast<float>(width);
    float scaleY = usableH / static_cast<float>(height);
    float scale = (scaleX < scaleY) ? scaleX : scaleY;

    int size = static_cast<int>(scale);
    if (size < 8)
        size = 8;
    if (size > 120)
        size = 120;
    return static_cast<uint16_t>(size);
}

} // namespace Utils
