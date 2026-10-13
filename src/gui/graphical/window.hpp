#pragma once
#include <cstdint>
#include <raylib.h>

/**
 * @file window.hpp
 * @brief Thin RAII wrapper around the raylib window and input API.
 *
 * Window abstracts raylib's global state so that the rest of the codebase
 * does not call raylib functions directly for basic window and input queries.
 * The window is opened in the constructor and closed in the destructor.
 */

/**
 * @brief RAII wrapper for a raylib window.
 *
 * Owns the raylib window lifetime. Config::VSYNC_ENABLED and Config::RESIZABLE
 * are read once during construction to set the appropriate window flags.
 */
class Window {
  public:
    /**
     * @brief Opens a raylib window with the given dimensions and title.
     *
     * Applies Config::VSYNC_ENABLED and Config::RESIZABLE flags before calling
     * InitWindow(), then sets the target FPS to Config::TARGET_FPS.
     *
     * @param width  Initial window width in pixels.
     * @param height Initial window height in pixels.
     * @param title  UTF-8 string shown in the title bar.
     */
    Window(uint16_t width, uint16_t height, const char *title);

    /**
     * @brief Closes the raylib window if it is still open.
     */
    ~Window();

    /**
     * @brief Returns false when the user has requested the window to close.
     * @return `true` while the window should remain open.
     */
    bool isOpen() const;

    /**
     * @brief Begins a new raylib drawing frame and clears the background.
     *
     * Must be paired with a subsequent call to display().
     *
     * @param r Red   component of the clear colour [0-255].
     * @param g Green component of the clear colour [0-255].
     * @param b Blue  component of the clear colour [0-255].
     * @param a Alpha component of the clear colour [0-255].
     */
    void clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

    /**
     * @brief Ends the current drawing frame and presents it to the screen.
     *
     * Must be called after clear() and all draw calls for the frame.
     */
    void display();

    /**
     * @brief Returns true if the given key was pressed this frame.
     * @param key raylib key constant (e.g. KEY_ESCAPE).
     */
    bool isKeyPressed(int key) const;

    /**
     * @brief Returns true while the given key is held down.
     * @param key raylib key constant.
     */
    bool isKeyDown(int key) const;

    /**
     * @brief Returns true if the given key was released this frame.
     * @param key raylib key constant.
     */
    bool isKeyReleased(int key) const;

    /**
     * @brief Returns true if the given mouse button was pressed this frame.
     * @param button raylib mouse button constant (e.g. MOUSE_BUTTON_RIGHT).
     */
    bool isMouseButtonPressed(int button) const;

    /**
     * @brief Returns true while the given mouse button is held down.
     * @param button raylib mouse button constant.
     */
    bool isMouseButtonDown(int button) const;

    /**
     * @brief Returns the current mouse cursor position in screen space.
     * @return raylib Vector2 with {x, y} in pixels.
     */
    Vector2 getMousePosition() const;

    /**
     * @brief Returns the current window width, updated on resize.
     * @return Width in pixels.
     */
    uint16_t getWidth() const;

    /**
     * @brief Returns the current window height, updated on resize.
     * @return Height in pixels.
     */
    uint16_t getHeight() const;

    /**
     * @brief Returns the time elapsed since the previous frame.
     * @return Delta time in seconds.
     */
    float getDeltaTime() const;

    /**
     * @brief Returns the measured frames per second.
     * @return Current FPS as reported by raylib.
     */
    int getFPS() const;

};
