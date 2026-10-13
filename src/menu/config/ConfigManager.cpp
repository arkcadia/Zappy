#include "ConfigManager.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

namespace zappy {

static std::string normalizedFrontend(std::string frontend) {
    std::transform(frontend.begin(), frontend.end(), frontend.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (frontend == "3d" || frontend == "zappy_gui3d" || frontend == "./zappy_gui3d")
        return "3d";
    return "2d";
}

static int clampAiCount(int count) {
    return std::max(0, count);
}

static void normalizeAiConfig(AiConfig &config) {
    std::vector<AiTeamConfig> normalized;

    for (const AiTeamConfig &entry : config.teamCounts) {
        if (entry.team.empty())
            continue;
        auto it = std::find_if(normalized.begin(), normalized.end(),
                               [&](const AiTeamConfig &existing) {
                                   return existing.team == entry.team;
                               });
        if (it == normalized.end()) {
            normalized.push_back({entry.team, clampAiCount(entry.count)});
        } else {
            it->count = clampAiCount(entry.count);
        }
    }

    if (normalized.empty()) {
        std::string team = config.team.empty() ? "Team1" : config.team;
        normalized.push_back({team, clampAiCount(config.count)});
    }

    config.teamCounts = normalized;

    const AiTeamConfig *legacy = &config.teamCounts.front();
    for (const AiTeamConfig &entry : config.teamCounts) {
        if (entry.count > 0) {
            legacy = &entry;
            break;
        }
    }
    config.team = legacy->team;
    config.count = clampAiCount(legacy->count);
}

ConfigManager &ConfigManager::instance() {
    static ConfigManager inst;
    return inst;
}

LaunchConfig &ConfigManager::config() { return m_config; }
const LaunchConfig &ConfigManager::config() const { return m_config; }

void ConfigManager::load(const std::string &path) {
    m_path = path;
    std::ifstream f(path);
    if (!f.is_open())
        return;

    try {
        json j;
        f >> j;

        auto &s = m_config.server;
        auto &a = m_config.ai;
        auto &g = m_config.gui;

        if (j.contains("server")) {
            auto &js = j["server"];
            if (js.contains("port"))
                s.port = js["port"].get<int>();
            if (js.contains("width"))
                s.width = js["width"].get<int>();
            if (js.contains("height"))
                s.height = js["height"].get<int>();
            if (js.contains("freq"))
                s.freq = js["freq"].get<int>();
            if (js.contains("clients"))
                s.clients = js["clients"].get<int>();
            if (js.contains("teams"))
                s.teams = js["teams"].get<std::vector<std::string>>();
        }
        if (j.contains("ai")) {
            auto &ja = j["ai"];
            if (ja.contains("host"))
                a.host = ja["host"].get<std::string>();
            if (ja.contains("port"))
                a.port = ja["port"].get<int>();
            if (ja.contains("team"))
                a.team = ja["team"].get<std::string>();
            if (ja.contains("count"))
                a.count = clampAiCount(ja["count"].get<int>());
            if (ja.contains("teams") && ja["teams"].is_array()) {
                a.teamCounts.clear();
                for (const auto &entry : ja["teams"]) {
                    std::string team;
                    int count = 0;

                    if (entry.is_object()) {
                        if (entry.contains("name"))
                            team = entry["name"].get<std::string>();
                        else if (entry.contains("team"))
                            team = entry["team"].get<std::string>();
                        if (entry.contains("count"))
                            count = entry["count"].get<int>();
                    } else if (entry.is_string()) {
                        team = entry.get<std::string>();
                    }
                    if (!team.empty())
                        a.teamCounts.push_back({team, clampAiCount(count)});
                }
            } else {
                a.teamCounts = {{a.team.empty() ? "Team1" : a.team, clampAiCount(a.count)}};
            }
            normalizeAiConfig(a);
        }
        if (j.contains("gui")) {
            auto &jg = j["gui"];
            if (jg.contains("host"))
                g.host = jg["host"].get<std::string>();
            if (jg.contains("port"))
                g.port = jg["port"].get<int>();
            if (jg.contains("frontend"))
                g.frontend = normalizedFrontend(jg["frontend"].get<std::string>());
        }
        if (j.contains("username")) {
            m_config.username = j["username"].get<std::string>();
        }
    } catch (...) {
    }
}

void ConfigManager::save(const std::string &path) const {
    const auto &s = m_config.server;
    const auto &a = m_config.ai;
    const auto &g = m_config.gui;

    json j;
    j["username"] = m_config.username;
    j["server"]["port"] = s.port;
    j["server"]["width"] = s.width;
    j["server"]["height"] = s.height;
    j["server"]["freq"] = s.freq;
    j["server"]["clients"] = s.clients;
    j["server"]["teams"] = s.teams;
    j["ai"]["host"] = a.host;
    j["ai"]["port"] = a.port;

    const AiTeamConfig *legacy = nullptr;
    for (const AiTeamConfig &entry : a.teamCounts) {
        if (entry.count > 0) {
            legacy = &entry;
            break;
        }
    }
    if (legacy == nullptr && !a.teamCounts.empty())
        legacy = &a.teamCounts.front();
    j["ai"]["team"] = legacy != nullptr ? legacy->team : a.team;
    j["ai"]["count"] = legacy != nullptr ? clampAiCount(legacy->count) : clampAiCount(a.count);

    j["ai"]["teams"] = json::array();
    for (const AiTeamConfig &entry : a.teamCounts) {
        json team;
        team["name"] = entry.team;
        team["count"] = clampAiCount(entry.count);
        j["ai"]["teams"].push_back(team);
    }
    j["gui"]["host"] = g.host;
    j["gui"]["port"] = g.port;
    j["gui"]["frontend"] = normalizedFrontend(g.frontend);

    std::ofstream f(path.empty() ? m_path : path);
    if (f.is_open())
        f << j.dump(2);
}

std::string ConfigManager::buildServerCmd() const {
    const auto &s = m_config.server;
    std::ostringstream oss;
    oss << "./zappy_server -p " << s.port << " -x " << s.width << " -y " << s.height << " -f "
        << s.freq << " -c " << s.clients << " -n";
    for (auto &t : s.teams)
        oss << " " << t;
    return oss.str();
}

std::string ConfigManager::buildAiCmd() const {
    const auto &a = m_config.ai;
    std::ostringstream oss;
    oss << "./zappy_ai -h " << a.host << " -p " << a.port << " -n " << a.team;
    return oss.str();
}

std::string ConfigManager::buildGuiCmd() const {
    const auto &g = m_config.gui;
    std::ostringstream oss;
    oss << guiBinary() << " -h " << g.host << " -p " << g.port;
    return oss.str();
}

std::string ConfigManager::guiBinary() const {
    return normalizedFrontend(m_config.gui.frontend) == "3d" ? "./zappy_gui3d" : "./zappy_gui";
}

} // namespace zappy
