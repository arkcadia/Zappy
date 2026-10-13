#include "Application.hpp"
#include "../config/ConfigManager.hpp"
#include "../launcher/LauncherManager.hpp"
#include "Renderer.hpp"
#include <raylib.h>

namespace zappy {

Application::Application() {
    auto &R = Renderer::instance();
    R.init(1100, 700, "Zappy Launcher");

    ConfigManager::instance().load("zappy_config.json");
    m_settingsPanel.syncFromConfig();
}

Application::~Application() {
    ConfigManager::instance().save("zappy_config.json");
    Renderer::instance().shutdown();
}

void Application::run() {
    while (!WindowShouldClose() && m_state.running) {
        update();
        render();
    }
}

void Application::update() {
    m_titleBar.update(m_state);
    m_sidebar.update(m_state);
    m_bottomBar.update(m_state);

    if (m_state.isOverlay(Overlay::LaunchModal)) {
        m_launchModal.update(m_state);
    } else if (m_state.isOverlay(Overlay::Settings)) {
        m_settingsPanel.update(m_state);
        if (m_state.isOverlay(Overlay::LaunchModal)) {
            auto &cfg = ConfigManager::instance().config();
            LaunchManager::instance().startSequence(cfg.server.port);
        }
    } else {
        m_homeScreen.update(m_state);
    }
}

void Application::render() {
    BeginDrawing();
    ClearBackground({5, 5, 8, 255});

    m_titleBar.draw();
    m_sidebar.draw(m_state);
    m_homeScreen.draw(m_state);

    if (m_state.isOverlay(Overlay::Settings)) {
        m_settingsPanel.draw(m_state);
    }

    if (m_state.isOverlay(Overlay::LaunchModal)) {
        m_launchModal.draw(m_state);
    }

    m_bottomBar.draw(m_state);

    EndDrawing();
}

} // namespace zappy