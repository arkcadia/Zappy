#include "SettingsPanel.hpp"
#include "../app/Renderer.hpp"
#include "../config/ConfigManager.hpp"
#include "Sidebar.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <raylib.h>
#include <rlgl.h>
#include <sstream>

static void fillR(Rectangle r, float rad, Color c) {
    float mx = std::max(r.width, r.height);
    DrawRectangleRounded(r, mx > 0 ? std::min(rad / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}
static void strokeR(Rectangle r, float rad, float t, Color c) {
    (void)t;
    float mx = std::max(r.width, r.height);
    DrawRectangleRoundedLines(r, mx > 0 ? std::min(rad / mx * 2.0f, 1.0f) : 0.0f, 8, c);
}

static std::string numStr(int v) { return std::to_string(v); }

static bool is3dFrontend(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value == "3d" || value == "zappy_gui3d" || value == "./zappy_gui3d";
}

static std::string normalizeFrontend(const std::string &value) {
    return is3dFrontend(value) ? "3d" : "2d";
}

static int parseNonNegativeInt(const std::string &value, int fallback) {
    try {
        return value.empty() ? fallback : std::max(0, std::stoi(value));
    } catch (...) {
        return fallback;
    }
}

static int aiCountFromConfig(const zappy::AiConfig &config, const std::string &team,
                             int fallback) {
    for (const zappy::AiTeamConfig &entry : config.teamCounts) {
        if (entry.team == team)
            return std::max(0, entry.count);
    }
    if (config.team == team)
        return std::max(0, config.count);
    return fallback;
}

static int aiGridRows(size_t teamCount) {
    return std::max(1, static_cast<int>((teamCount + 2) / 3));
}

static float settingsContentHeight(size_t teamCount, size_t previewLines) {
    float aiRows = static_cast<float>(aiGridRows(teamCount));
    float previewExtra = std::max(0.0f, (static_cast<float>(previewLines) - 3.0f) * 20.0f);

    return 680.0f + aiRows * 72.0f + previewExtra;
}

namespace zappy {

SettingsPanel::SettingsPanel() {
    auto setupNum = [](ui::TextInput &f, const std::string &lbl, const std::string &flag) {
        f.label = lbl;
        f.flag = flag;
        f.numeric = true;
        f.fontSize = 13.0f;
    };
    auto setupStr = [](ui::TextInput &f, const std::string &lbl, const std::string &flag) {
        f.label = lbl;
        f.flag = flag;
        f.numeric = false;
        f.fontSize = 13.0f;
    };

    setupNum(m_serverPort, "Port", "-p");
    setupNum(m_serverWidth, "Map width", "-x");
    setupNum(m_serverHeight, "Map height", "-y");
    setupNum(m_serverFreq, "Frequency", "-f");
    setupNum(m_serverClients, "Max clients", "-c");
    setupStr(m_serverTeams, "Teams", "-n");
    setupStr(m_aiHost, "Host", "-h");
    setupNum(m_aiPort, "Port", "-p");
    setupStr(m_guiHost, "Host", "-h");
    setupNum(m_guiPort, "Port", "-p");
    setupStr(m_guiFrontend, "Frontend", "2d/3d");

    m_gui2dBtn.label = "2D";
    m_gui3dBtn.label = "3D";

    m_launchBtn.label = "LAUNCH";
    m_launchBtn.style = ui::ButtonStyle{};

    m_cancelBtn.label = "CANCEL";
    m_cancelBtn.style = ui::Button::ghostStyle();

    syncFromConfig();
}

void SettingsPanel::syncFromConfig() {
    auto &cfg = ConfigManager::instance().config();
    m_serverPort.value = numStr(cfg.server.port);
    m_serverWidth.value = numStr(cfg.server.width);
    m_serverHeight.value = numStr(cfg.server.height);
    m_serverFreq.value = numStr(cfg.server.freq);
    m_serverClients.value = numStr(cfg.server.clients);
    std::string teams;
    for (size_t i = 0; i < cfg.server.teams.size(); ++i) {
        if (i > 0)
            teams += " ";
        teams += cfg.server.teams[i];
    }
    m_serverTeams.value = teams;
    m_aiHost.value = cfg.ai.host;
    m_aiPort.value = m_serverPort.value;
    m_aiRows.clear();
    ensureAiRowsForTeams(parseServerTeams());
    m_guiHost.value = cfg.gui.host;
    m_guiPort.value = m_serverPort.value;
    m_guiFrontend.value = normalizeFrontend(cfg.gui.frontend);
}

void SettingsPanel::syncToConfig() {
    auto &cfg = ConfigManager::instance().config();
    auto toInt = [](const std::string &s, int def) -> int {
        try {
            return s.empty() ? def : std::stoi(s);
        } catch (...) {
            return def;
        }
    };
    cfg.server.port = toInt(m_serverPort.value, 4242);
    cfg.server.width = toInt(m_serverWidth.value, 20);
    cfg.server.height = toInt(m_serverHeight.value, 20);
    cfg.server.freq = toInt(m_serverFreq.value, 100);
    cfg.server.clients = toInt(m_serverClients.value, 10);

    cfg.server.teams = parseServerTeams();
    ensureAiRowsForTeams(cfg.server.teams);

    cfg.ai.host = m_aiHost.value.empty() ? "localhost" : m_aiHost.value;
    cfg.ai.port = cfg.server.port;

    cfg.ai.teamCounts.clear();
    int maxAiForTeam = 0;
    const AiTeamRow *legacyRow = nullptr;
    const AiTeamRow *firstRow = nullptr;
    for (const AiTeamRow &row : m_aiRows) {
        int count = parseNonNegativeInt(row.count.value, 0);
        cfg.ai.teamCounts.push_back({row.team, count});
        maxAiForTeam = std::max(maxAiForTeam, count);
        if (firstRow == nullptr)
            firstRow = &row;
        if (legacyRow == nullptr && count > 0)
            legacyRow = &row;
    }
    if (legacyRow == nullptr)
        legacyRow = firstRow;
    if (legacyRow != nullptr) {
        cfg.ai.team = legacyRow->team;
        cfg.ai.count = parseNonNegativeInt(legacyRow->count.value, 0);
    }
    if (cfg.server.clients < maxAiForTeam) {
        cfg.server.clients = maxAiForTeam;
        m_serverClients.value = numStr(cfg.server.clients);
    }
    cfg.gui.host = m_guiHost.value.empty() ? "localhost" : m_guiHost.value;
    cfg.gui.port = cfg.server.port;
    cfg.gui.frontend = normalizeFrontend(m_guiFrontend.value);
}

std::vector<std::string> SettingsPanel::parseServerTeams() const {
    std::vector<std::string> teams;
    std::istringstream ss(m_serverTeams.value);
    std::string team;

    while (ss >> team)
        teams.push_back(team);
    if (teams.empty())
        teams.push_back("Team1");
    return teams;
}

bool SettingsPanel::ensureAiRowsForTeams(const std::vector<std::string> &teams) {
    bool same = teams.size() == m_aiRows.size();

    if (same) {
        for (size_t i = 0; i < teams.size(); ++i) {
            if (m_aiRows[i].team != teams[i]) {
                same = false;
                break;
            }
        }
    }
    if (same)
        return false;

    bool focusedAiRow = false;
    for (const AiTeamRow &row : m_aiRows) {
        if (m_focusedInput == &row.count) {
            focusedAiRow = true;
            break;
        }
    }

    auto existingCount = [&](const std::string &team) -> int {
        for (const AiTeamRow &row : m_aiRows) {
            if (row.team == team)
                return parseNonNegativeInt(row.count.value, 0);
        }
        return aiCountFromConfig(ConfigManager::instance().config().ai, team, 0);
    };

    std::vector<AiTeamRow> rows;
    rows.reserve(teams.size());
    for (const std::string &team : teams) {
        AiTeamRow row;

        row.team = team;
        row.count.label = team;
        row.count.flag = "-n";
        row.count.numeric = true;
        row.count.fontSize = 13.0f;
        row.count.value = numStr(existingCount(team));
        row.minusButton.label = "-";
        row.minusButton.style = ui::Button::ghostStyle();
        row.plusButton.label = "+";
        row.plusButton.style = ui::Button::ghostStyle();
        rows.push_back(row);
    }
    m_aiRows = rows;
    if (focusedAiRow)
        m_focusedInput = nullptr;
    return true;
}

void SettingsPanel::layoutFields(float x, float y, float w) {
    float col3W = (w - 24.0f) / 3.0f;
    float inputH = 36.0f;
    float fieldH = 54.0f;
    float gy = y + 72.0f;

    gy += 22.0f;
    m_serverPort.bounds = {x, gy + 18.0f, col3W, inputH};
    m_serverWidth.bounds = {x + col3W + 12.0f, gy + 18.0f, col3W, inputH};
    m_serverHeight.bounds = {x + (col3W + 12.0f) * 2, gy + 18.0f, col3W, inputH};
    gy += fieldH + 18.0f;
    m_serverFreq.bounds = {x, gy + 18.0f, col3W, inputH};
    m_serverClients.bounds = {x + col3W + 12.0f, gy + 18.0f, col3W, inputH};
    m_serverTeams.bounds = {x + (col3W + 12.0f) * 2, gy + 18.0f, col3W, inputH};
    gy += fieldH + 18.0f + 44.0f;

    gy += 22.0f;
    m_aiHost.bounds = {x, gy + 18.0f, col3W, inputH};
    m_aiPort.bounds = {};
    gy += fieldH + 18.0f;

    float stepperW = 34.0f;
    float stepperGap = 7.0f;
    for (size_t i = 0; i < m_aiRows.size(); ++i) {
        int gridX = static_cast<int>(i % 3);
        int gridY = static_cast<int>(i / 3);
        Rectangle area = {x + gridX * (col3W + 12.0f), gy + gridY * (fieldH + 18.0f) + 18.0f,
                          col3W, inputH};

        m_aiRows[i].minusButton.bounds = {area.x, area.y, stepperW, area.height};
        m_aiRows[i].plusButton.bounds = {area.x + area.width - stepperW, area.y, stepperW,
                                         area.height};
        m_aiRows[i].count.bounds = {area.x + stepperW + stepperGap, area.y,
                                    area.width - (stepperW + stepperGap) * 2.0f, area.height};
    }
    gy += static_cast<float>(aiGridRows(m_aiRows.size())) * (fieldH + 18.0f) + 44.0f;

    gy += 22.0f;
    m_guiHost.bounds = {x, gy + 18.0f, col3W, inputH};
    m_guiFrontend.bounds = {x + col3W + 12.0f, gy + 18.0f, col3W, inputH};
    m_guiPort.bounds = {};
}

void SettingsPanel::update(AppState &state) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float titleH = 36.0f;
    float bottomH = 72.0f;
    float panelX = Sidebar::WIDTH;
    float panelY = titleH;
    float panelW = sw - panelX;
    float panelH = sh - titleH - bottomH;

    m_scroll.viewport = {panelX, panelY, panelW, panelH};
    ensureAiRowsForTeams(parseServerTeams());
    size_t previewLines = 2;
    for (const AiTeamRow &row : m_aiRows) {
        if (parseNonNegativeInt(row.count.value, 0) > 0)
            ++previewLines;
    }
    if (previewLines == 2)
        ++previewLines;
    m_scroll.contentHeight = settingsContentHeight(m_aiRows.size(), previewLines);
    m_scroll.update();

    float innerX = panelX + 25.0f;
    float innerW = panelW - 64.0f;
    float scrolledY = panelY - m_scroll.getOffset();

    auto refreshLayout = [&]() {
        layoutFields(innerX, scrolledY, innerW);
        float frontendGap = 8.0f;
        float frontendW = (m_guiFrontend.bounds.width - frontendGap) * 0.5f;
        m_gui2dBtn.bounds = {m_guiFrontend.bounds.x, m_guiFrontend.bounds.y, frontendW,
                             m_guiFrontend.bounds.height};
        m_gui3dBtn.bounds = {m_guiFrontend.bounds.x + frontendW + frontendGap,
                             m_guiFrontend.bounds.y, frontendW, m_guiFrontend.bounds.height};
    };
    refreshLayout();

    auto refreshFrontendButtons = [this]() {
        bool use3d = is3dFrontend(m_guiFrontend.value);
        m_gui2dBtn.style = use3d ? ui::Button::ghostStyle() : ui::ButtonStyle{};
        m_gui3dBtn.style = use3d ? ui::ButtonStyle{} : ui::Button::ghostStyle();
    };
    refreshFrontendButtons();

    float footerY = panelY + panelH - 52.0f;
    float footerRight = panelX + panelW - 36.0f;
    m_launchBtn.bounds = {footerRight - 124.0f, footerY, 124.0f, 36.0f};
    m_cancelBtn.bounds = {m_launchBtn.bounds.x - 108.0f, footerY, 96.0f, 36.0f};

    handleFocusClick();

    std::vector<ui::TextInput *> inputs = {&m_serverPort, &m_serverWidth,   &m_serverHeight,
                                           &m_serverFreq, &m_serverClients, &m_serverTeams,
                                           &m_aiHost,     &m_guiHost};
    for (AiTeamRow &row : m_aiRows)
        inputs.push_back(&row.count);
    for (auto *inp : inputs) {
        if (inp->focused)
            inp->update();
    }

    m_aiPort.value = m_serverPort.value;
    m_guiPort.value = m_serverPort.value;

    if (ensureAiRowsForTeams(parseServerTeams()))
        refreshLayout();

    if (m_gui2dBtn.update()) {
        m_guiFrontend.value = "2d";
        ConfigManager::instance().config().gui.frontend = "2d";
    }
    if (m_gui3dBtn.update()) {
        m_guiFrontend.value = "3d";
        ConfigManager::instance().config().gui.frontend = "3d";
    }
    refreshFrontendButtons();

    for (AiTeamRow &row : m_aiRows) {
        if (row.minusButton.update()) {
            int count = parseNonNegativeInt(row.count.value, 0);
            row.count.value = numStr(std::max(0, count - 1));
        }
        if (row.plusButton.update()) {
            int count = parseNonNegativeInt(row.count.value, 0);
            row.count.value = numStr(count + 1);
        }
    }

    syncToConfig();

    auto clearFocus = [&inputs, this]() {
        m_focusedInput = nullptr;
        inputs = {&m_serverPort, &m_serverWidth,   &m_serverHeight,
                  &m_serverFreq, &m_serverClients, &m_serverTeams,
                  &m_aiHost,     &m_guiHost};
        for (AiTeamRow &row : m_aiRows)
            inputs.push_back(&row.count);
        for (auto *inp : inputs)
            inp->focused = false;
    };

    if (m_launchBtn.update()) {
        clearFocus();
        syncToConfig();
        ConfigManager::instance().save("zappy_config.json");
        state.openOverlay(Overlay::LaunchModal);
        return;
    }

    if (m_cancelBtn.update()) {
        clearFocus();
        state.closeOverlay();
        state.navigate(Page::Home);
    }

    float closeX = sw - 28.0f - 16.0f;
    float closeY = panelY + 14.0f;
    if (CheckCollisionPointRec(GetMousePosition(), {closeX, closeY, 28.0f, 28.0f}) &&
        IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        clearFocus();
        state.closeOverlay();
        state.navigate(Page::Home);
    }
}

void SettingsPanel::handleFocusClick() {
    if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        return;

    std::vector<ui::TextInput *> inputs = {&m_serverPort, &m_serverWidth,   &m_serverHeight,
                                           &m_serverFreq, &m_serverClients, &m_serverTeams,
                                           &m_aiHost,     &m_guiHost};
    for (AiTeamRow &row : m_aiRows)
        inputs.push_back(&row.count);

    Vector2 mp = GetMousePosition();
    m_focusedInput = nullptr;
    for (auto *inp : inputs) {
        if (CheckCollisionPointRec(mp, inp->bounds)) {
            inp->focused = true;
            m_focusedInput = inp;
        } else {
            inp->focused = false;
        }
    }
}

void SettingsPanel::draw(const AppState &) const {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float titleH = 36.0f;
    float bottomH = 72.0f;
    float panelX = Sidebar::WIDTH;
    float panelY = titleH;
    float panelW = sw - panelX;
    float panelH = sh - titleH - bottomH;
    float innerX = panelX + 32.0f;
    float innerW = panelW - 64.0f;

    DrawRectangle((int)panelX, (int)panelY, (int)panelW, (int)panelH, {8, 6, 16, 248});

    m_scroll.beginClip();
    float scrolledY = panelY - m_scroll.getOffset();
    drawSettingsContent(innerX, scrolledY, innerW, 0);
    m_scroll.endClip();

    auto &R = Renderer::instance();
    auto &th = R.theme();

    DrawRectangle((int)panelX, (int)(panelY + panelH - 64.0f), (int)panelW, 64, {8, 6, 16, 248});
    DrawLineEx({panelX, panelY + panelH - 64.0f}, {panelX + panelW, panelY + panelH - 64.0f}, 1.0f,
               th.borderCol);

    m_cancelBtn.draw();
    m_launchBtn.draw();
    float closeX = sw - 28.0f - 16.0f;
    float closeY = panelY + 14.0f;
    Rectangle closeBtn = {closeX, closeY, 28.0f, 28.0f};
    Vector2 mp = GetMousePosition();
    bool closeHov = CheckCollisionPointRec(mp, closeBtn);
    fillR(closeBtn, 6.0f, closeHov ? Color{239, 68, 68, 38} : Color{255, 255, 255, 13});
    strokeR(closeBtn, 6.0f, 1.0f, closeHov ? Color{239, 68, 68, 102} : th.borderCol);
    DrawTextEx(R.fontBody(), "x", {closeX + 8.0f, closeY + 7.0f}, 13.0f, 0.0f,
               closeHov ? Color{248, 113, 113, 255} : th.textSub);
}

void SettingsPanel::drawSettingsContent(float x, float y, float w, float scrollY) const {
    (void)scrollY;
    auto &R = Renderer::instance();
    auto &th = R.theme();
    float fieldH = 54.0f;

    float headY = y + 28.0f;
    DrawTextEx(R.fontMono(), "LAUNCH CONFIGURATION", {x, headY}, 15.0f, 1.5f, th.textPrimary);
    DrawLineEx({x, headY + 26.0f}, {x + w, headY + 26.0f}, 1.0f, th.borderCol);

    float gy = headY + 44.0f;

    auto drawField = [&](const ui::TextInput &inp) {
        ui::drawLabel(inp.label, inp.flag, inp.bounds.x, inp.bounds.y - 18.0f);
        inp.draw();
    };

    ui::drawSectionTitle("H", "SERVER — zappy_server", x, gy, w);
    gy += 22.0f;
    drawField(m_serverPort);
    drawField(m_serverWidth);
    drawField(m_serverHeight);
    gy += fieldH + 18.0f;
    drawField(m_serverFreq);
    drawField(m_serverClients);
    drawField(m_serverTeams);
    gy += fieldH + 18.0f;
    gy += 44.0f;

    ui::drawSectionTitle("@", "AI CLIENT — zappy_ai", x, gy, w);
    gy += 22.0f;
    drawField(m_aiHost);
    gy += fieldH + 18.0f;
    for (const AiTeamRow &row : m_aiRows) {
        ui::drawLabel(row.team, "-n", row.minusButton.bounds.x, row.count.bounds.y - 18.0f);
        row.minusButton.draw();
        row.count.draw();
        row.plusButton.draw();
    }
    gy += static_cast<float>(aiGridRows(m_aiRows.size())) * (fieldH + 18.0f);
    gy += 44.0f;

    ui::drawSectionTitle("O", "GUI — frontend", x, gy, w);
    gy += 22.0f;
    drawField(m_guiHost);
    ui::drawLabel("Frontend", "2d/3d", m_guiFrontend.bounds.x, m_guiFrontend.bounds.y - 18.0f);
    m_gui2dBtn.draw();
    m_gui3dBtn.draw();
    gy += fieldH + 18.0f;
    gy += 44.0f;

    auto &cm = ConfigManager::instance();
    const auto &cfg = cm.config();
    std::vector<std::string> cmds = {cm.buildServerCmd()};
    for (const AiTeamRow &row : m_aiRows) {
        int count = parseNonNegativeInt(row.count.value, 0);
        if (count <= 0)
            continue;
        std::ostringstream oss;
        oss << "./zappy_ai -h " << cfg.ai.host << " -p " << cfg.ai.port << " -n " << row.team;
        if (count > 1)
            oss << "  x" << count;
        cmds.push_back(oss.str());
    }
    if (cmds.size() == 1)
        cmds.push_back("zappy_ai disabled");
    cmds.push_back(cm.buildGuiCmd());
    ui::drawCmdPreview(cmds, x, gy, w, std::max(82.0f, 22.0f * cmds.size() + 18.0f));
}

void SettingsPanel::drawInputField(ui::TextInput &input, float x, float y, float w) const {
    input.bounds = {x, y, w, 36.0f};
    input.draw();
}

void SettingsPanel::updateInputPositions(float, float, float) {}

} // namespace zappy
