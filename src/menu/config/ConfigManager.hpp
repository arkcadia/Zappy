/**
 * @file ConfigManager.hpp
 * @brief Persistent launch-configuration management (JSON serialisation).
 *
 * @details ConfigManager is a singleton that owns the current LaunchConfig and
 * handles loading / saving it as a JSON file on disk (via nlohmann/json).
 * It also provides helpers to build the CLI command strings for the Zappy
 * processes.
 *
 * @par JSON schema (zappy_config.json)
 * @code{.json}
 * {
 *   "username": "PLAYER_1",
 *   "server": { "port": 4242, "width": 20, "height": 20,
 *               "freq": 100, "clients": 10, "teams": ["Team1","Team2"] },
 *   "ai":     { "host": "localhost", "port": 4242,
 *               "team": "Team1", "count": 1,
 *               "teams": [{"name": "Team1", "count": 1}] },
 *   "gui":    { "host": "localhost", "port": 4242, "frontend": "2d" }
 * }
 * @endcode
 *
 * @ingroup Config
 */

#pragma once
#include <string>
#include <vector>

namespace zappy {

/**
 * @brief Configuration for the `zappy_server` process.
 * @ingroup Config
 */
struct ServerConfig {
    int port = 4242;                                     ///< TCP port (`-p`).
    int width = 20;                                      ///< Map width in tiles (`-x`).
    int height = 20;                                     ///< Map height in tiles (`-y`).
    int freq = 100;                                      ///< Time unit frequency in Hz (`-f`).
    int clients = 10;                                    ///< Maximum AI clients per team (`-c`).
    std::vector<std::string> teams = {"Team1", "Team2"}; ///< Team names (`-n`).
};

/**
 * @brief Per-team AI launch count.
 * @ingroup Config
 */
struct AiTeamConfig {
    std::string team = "Team1"; ///< Team passed to `zappy_ai -n`.
    int count = 0;              ///< Number of AI clients to launch for this team.
};

/**
 * @brief Configuration for the `zappy_ai` client process.
 * @ingroup Config
 */
struct AiConfig {
    std::string host = "localhost";                   ///< Server hostname (`-h`).
    int port = 4242;                                  ///< Server port (`-p`).
    std::string team = "Team1";                       ///< Legacy single team (`-n`).
    int count = 1;                                    ///< Legacy single-team client count.
    std::vector<AiTeamConfig> teamCounts = {{"Team1", 1}}; ///< AI clients per server team.
};

/**
 * @brief Configuration for the graphical client process.
 * @ingroup Config
 */
struct GuiConfig {
    std::string host = "localhost"; ///< Server hostname (`-h`).
    int port = 4242;                ///< Server port (`-p`).
    std::string frontend = "2d";     ///< Frontend renderer: `2d` or `3d`.
};

/**
 * @brief Aggregate holding all per-process configuration plus the username.
 * @ingroup Config
 */
struct LaunchConfig {
    ServerConfig server;               ///< Server process config.
    AiConfig ai;                       ///< AI client config.
    GuiConfig gui;                     ///< GUI client config.
    std::string username = "PLAYER_1"; ///< Display name shown in the sidebar.
};

/**
 * @brief Singleton that loads, persists, and provides access to LaunchConfig.
 *
 * @par Accessing the config
 * @code
 * auto &cfg = zappy::ConfigManager::instance().config();
 * cfg.server.port = 1234;
 * zappy::ConfigManager::instance().save("zappy_config.json");
 * @endcode
 *
 * @ingroup Config
 */
class ConfigManager {
  public:
    /**
     * @brief Access the singleton instance.
     * @return Reference to the unique ConfigManager object.
     */
    static ConfigManager &instance();

    /**
     * @brief Load configuration from a JSON file.
     *
     * Missing keys are silently ignored; defaults from LaunchConfig are kept.
     * Any JSON parse exception is swallowed – the config remains at defaults.
     *
     * @param path File-system path to the JSON config file.
     */
    void load(const std::string &path);

    /**
     * @brief Persist the current configuration to a JSON file.
     *
     * If @p path is empty, falls back to the path supplied in the last
     * load() call.
     *
     * @param path Destination file-system path.
     */
    void save(const std::string &path) const;

    /**
     * @brief Mutable access to the current configuration.
     * @return Reference to the internal LaunchConfig.
     */
    LaunchConfig &config();

    /**
     * @brief Read-only access to the current configuration.
     * @return Const reference to the internal LaunchConfig.
     */
    const LaunchConfig &config() const;

    /**
     * @brief Build the full command line for `zappy_server`.
     *
     * Format: `./zappy_server -p <port> -x <w> -y <h> -f <freq> -c <clients> -n <teams...>`
     *
     * @return Shell command string.
     */
    std::string buildServerCmd() const;

    /**
     * @brief Build the full command line for `zappy_ai`.
     *
     * Format: `./zappy_ai -h <host> -p <port> -n <team>`
     *
     * @return Shell command string.
     */
    std::string buildAiCmd() const;

    /**
     * @brief Build the full command line for the selected GUI frontend.
     *
     * Format: `./zappy_gui -h <host> -p <port>` or
     * `./zappy_gui3d -h <host> -p <port>`
     *
     * @return Shell command string.
     */
    std::string buildGuiCmd() const;

    /**
     * @brief Resolve the configured GUI frontend to an executable name.
     *
     * Accepts `2d`, `zappy_gui`, `3d`, or `zappy_gui3d`; unknown values fall
     * back to the 2D GUI.
     *
     * @return `./zappy_gui` or `./zappy_gui3d`.
     */
    std::string guiBinary() const;

  private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    LaunchConfig m_config; ///< In-memory copy of the configuration.
    std::string m_path;    ///< Path used for the last load(); save() fallback.
};

} // namespace zappy
