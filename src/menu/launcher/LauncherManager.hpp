/**
 * @file LauncherManager.hpp
 * @brief Process launch and log management.
 *
 * @details LaunchManager starts the Zappy server, the configured AI clients,
 * and the selected GUI frontend while exposing status log entries to the modal UI.
 *
 * The manager is driven by LaunchManager::update() which must be called
 * every frame with the elapsed frame time in seconds.
 *
 * @par Typical usage
 * @code
 * // Trigger from SettingsPanel on user confirmation:
 * LaunchManager::instance().startSequence(cfg.server.port);
 *
 * // Each frame in LaunchModal::update():
 * LaunchManager::instance().update(GetFrameTime());
 *
 * // Render log in LaunchModal::draw():
 * for (auto &entry : LaunchManager::instance().log()) {
 *     if (entry.visible) { ... }
 * }
 * @endcode
 *
 * @ingroup Launcher
 */

#pragma once
#include "../config/ConfigManager.hpp"
#include <functional>
#include <string>
#include <vector>

namespace zappy {

/**
 * @brief A single timed event in the launch sequence script.
 * @ingroup Launcher
 */
struct LaunchStep {
    float delayMs;       ///< Milliseconds after sequence start before the entry becomes visible.
    std::string message; ///< Log message text.

    /**
     * @brief Visual type controlling the prefix icon and colour in LaunchModal.
     */
    enum class Type {
        Info, ///< Neutral informational message (grey, no icon prefix).
        Ok,   ///< Success confirmation (green, ✓ prefix).
        Wait, ///< Pending / in-progress step (yellow, ○ prefix).
        Done, ///< Final completion event (purple, ★ prefix); sets isComplete to true.
        Error ///< Failure event (red, ! prefix); sets isComplete to true.
    } type;
};

/**
 * @brief A rendered log entry derived from a LaunchStep.
 *
 * Entries start invisible and become visible once the sequence clock
 * reaches the corresponding LaunchStep::delayMs threshold.
 *
 * @ingroup Launcher
 */
struct LogEntry {
    std::string message;   ///< Text to display.
    LaunchStep::Type type; ///< Display style / icon.
    bool visible = false;  ///< True once the delay threshold has been passed.
};

/**
 * @brief Singleton that manages process startup and launch status.
 *
 * Call startSequence() to begin, update() each frame, and read log() for the
 * current display state.
 *
 * @ingroup Launcher
 */
class LaunchManager {
  public:
    /**
     * @brief Access the singleton instance.
     * @return Reference to the unique LaunchManager object.
     */
    static LaunchManager &instance();

    /**
     * @brief Reset state and begin launching the configured processes.
     *
     * Pre-populates the log with all steps (all invisible) and resets the
     * elapsed clock.  The sequence advances as update() is called.
     *
     * @param serverPort TCP port passed to the server step messages
     *                   (e.g. "Server listening on :4242").
     */
    void startSequence(int serverPort);

    /**
     * @brief Advance the sequence clock and reveal log entries on schedule.
     *
     * Must be called once per frame while the sequence is active.
     * Has no effect if the sequence is not active or is already complete.
     *
     * @param dt Elapsed time since the last frame, in seconds.
     */
    void update(float dt);

    /**
     * @brief Check whether a launch sequence is currently in progress.
     * @return true after startSequence() and before reset().
     */
    bool isRunning() const;

    /**
     * @brief Check whether the final "Done" step has been revealed.
     * @return true once the last LaunchStep::Type::Done entry becomes visible.
     */
    bool isComplete() const;

    /**
     * @brief Read-only access to the current log entries.
     *
     * The vector always contains all steps; iterate and check
     * LogEntry::visible to determine what to render.
     *
     * @return Const reference to the LogEntry vector.
     */
    const std::vector<LogEntry> &log() const;

    /**
     * @brief Clear all state and stop the sequence.
     *
     * Called when the user closes the LaunchModal.
     */
    void reset();

  private:
    LaunchManager() = default;
    ~LaunchManager() = default;
    LaunchManager(const LaunchManager &) = delete;
    LaunchManager &operator=(const LaunchManager &) = delete;

    /**
     * @brief Build the hard-coded step script for the given server port.
     * @param serverPort Port number embedded into step messages.
     */
    enum class Phase {
        Idle,
        StartServer,
        WaitServer,
        StartGui,
        StartAi,
        Done,
        Failed,
    };

    void addLog(const std::string &message, LaunchStep::Type type);
    void fail(const std::string &message);
    void killStartedProcesses();

    std::vector<LogEntry> m_log;     ///< Display state for each step.
    float m_elapsed = 0.0f;          ///< Milliseconds since startSequence().
    float m_phaseElapsed = 0.0f;     ///< Milliseconds since the current phase started.
    LaunchConfig m_config;           ///< Snapshot of config at launch click time.
    std::vector<int> m_childPids;    ///< PIDs spawned during this launch attempt.
    Phase m_phase = Phase::Idle;     ///< Current launch state.
    bool m_active = false;           ///< True between startSequence() and reset().
    bool m_done = false;             ///< True once Type::Done step is visible.
};

} // namespace zappy
