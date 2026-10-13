#include "window.hpp"
#include "config.hpp"
#include <raylib.h>

Window::Window(uint16_t width, uint16_t height, const char *title) {
    SetConfigFlags(0);
    if (Config::VSYNC_ENABLED)
        SetConfigFlags(FLAG_VSYNC_HINT);
    if (Config::RESIZABLE)
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);

    InitWindow(width, height, title);
    SetTargetFPS(Config::TARGET_FPS);
}

Window::~Window() {
    if (IsWindowReady())
        CloseWindow();
}

bool Window::isOpen() const { return !WindowShouldClose(); }

void Window::clear(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    BeginDrawing();
    ClearBackground({r, g, b, a});
}

void Window::display() { EndDrawing(); }

bool Window::isKeyPressed(int key) const { return IsKeyPressed(key); }

bool Window::isKeyDown(int key) const { return IsKeyDown(key); }

bool Window::isKeyReleased(int key) const { return IsKeyReleased(key); }

bool Window::isMouseButtonPressed(int button) const { return IsMouseButtonPressed(button); }

bool Window::isMouseButtonDown(int button) const { return IsMouseButtonDown(button); }

Vector2 Window::getMousePosition() const { return GetMousePosition(); }

uint16_t Window::getWidth() const { return static_cast<uint16_t>(GetScreenWidth()); }

uint16_t Window::getHeight() const { return static_cast<uint16_t>(GetScreenHeight()); }

float Window::getDeltaTime() const { return GetFrameTime(); }

int Window::getFPS() const { return GetFPS(); }
