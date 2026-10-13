#include "LauncherManager.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace zappy {

static constexpr float SERVER_READY_TIMEOUT_MS = 5000.0f;
static constexpr float AI_START_DELAY_MS = 300.0f;

static std::string errnoMessage(const std::string &prefix)
{
    return prefix + ": " + std::strerror(errno);
}

static bool ensureLogDir(std::string &error)
{
    if (mkdir("logs", 0755) != 0 && errno != EEXIST) {
        error = errnoMessage("cannot create logs");
        return false;
    }
    if (mkdir("logs/launcher", 0755) != 0 && errno != EEXIST) {
        error = errnoMessage("cannot create logs/launcher");
        return false;
    }
    return true;
}

static bool isExecutable(const std::string &path)
{
    return access(path.c_str(), X_OK) == 0;
}

static std::string joinCommand(const std::vector<std::string> &args)
{
    std::ostringstream oss;

    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
            oss << ' ';
        oss << args[i];
    }
    return oss.str();
}

static bool spawnProcess(const std::vector<std::string> &args, const std::string &logPath,
                         int &pidOut, std::string &error)
{
    if (args.empty()) {
        error = "empty command";
        return false;
    }

    pid_t pid = fork();
    if (pid < 0) {
        error = errnoMessage("fork failed");
        return false;
    }

    if (pid == 0) {
        setsid();

        int fd = open(logPath.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd >= 0) {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        std::vector<char *> argv;
        argv.reserve(args.size() + 1);
        for (const std::string &arg : args)
            argv.push_back(const_cast<char *>(arg.c_str()));
        argv.push_back(nullptr);

        execv(args[0].c_str(), argv.data());
        _exit(127);
    }

    pidOut = static_cast<int>(pid);
    return true;
}

static bool processExited(int pid)
{
    int status = 0;
    pid_t result = waitpid(static_cast<pid_t>(pid), &status, WNOHANG);

    return result == static_cast<pid_t>(pid);
}

static bool canConnectToPort(int port)
{
    addrinfo hints{};
    addrinfo *res = nullptr;
    std::string portString = std::to_string(port);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo("127.0.0.1", portString.c_str(), &hints, &res) != 0)
        return false;

    bool connected = false;
    for (addrinfo *it = res; it != nullptr; it = it->ai_next) {
        int fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0)
            continue;
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0)
            connected = true;
        close(fd);
        if (connected)
            break;
    }
    freeaddrinfo(res);
    return connected;
}

static std::vector<std::string> buildServerArgs(const LaunchConfig &config)
{
    std::vector<std::string> args = {
        "./zappy_server",
        "-p",
        std::to_string(config.server.port),
        "-x",
        std::to_string(config.server.width),
        "-y",
        std::to_string(config.server.height),
        "-f",
        std::to_string(config.server.freq),
        "-c",
        std::to_string(config.server.clients),
        "-n",
    };

    for (const std::string &team : config.server.teams)
        args.push_back(team);
    return args;
}

static std::vector<std::string> buildAiArgs(const LaunchConfig &config, const std::string &team)
{
    return {
        "./zappy_ai",
        "-h",
        config.ai.host,
        "-p",
        std::to_string(config.ai.port),
        "-n",
        team,
    };
}

struct AiLaunchGroup {
    std::string team;
    int count = 0;
};

static std::vector<AiLaunchGroup> buildAiLaunchGroups(const LaunchConfig &config)
{
    std::vector<AiLaunchGroup> groups;

    for (const AiTeamConfig &entry : config.ai.teamCounts) {
        if (!entry.team.empty() && entry.count > 0)
            groups.push_back({entry.team, entry.count});
    }
    if (groups.empty() && config.ai.teamCounts.empty() && !config.ai.team.empty() &&
        config.ai.count > 0) {
        groups.push_back({config.ai.team, config.ai.count});
    }
    return groups;
}

static std::string logSafeTeamName(std::string team)
{
    for (char &c : team) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!std::isalnum(uc) && c != '_' && c != '-')
            c = '_';
    }
    return team.empty() ? "team" : team;
}

static std::vector<std::string> buildGuiArgs(const LaunchConfig &config,
                                             const std::string &guiBinary)
{
    return {
        guiBinary,
        "-h",
        config.gui.host,
        "-p",
        std::to_string(config.gui.port),
    };
}

LaunchManager &LaunchManager::instance()
{
    static LaunchManager inst;
    return inst;
}

void LaunchManager::addLog(const std::string &message, LaunchStep::Type type)
{
    m_log.push_back({message, type, true});
}

void LaunchManager::fail(const std::string &message)
{
    addLog(message, LaunchStep::Type::Error);
    killStartedProcesses();
    m_phase = Phase::Failed;
    m_done = true;
}

void LaunchManager::killStartedProcesses()
{
    for (int pid : m_childPids) {
        if (pid > 0)
            kill(static_cast<pid_t>(pid), SIGTERM);
    }
}

void LaunchManager::startSequence(int)
{
    reset();
    m_config = ConfigManager::instance().config();
    m_active = true;
    m_done = false;
    m_phase = Phase::StartServer;
    m_elapsed = 0.0f;
    m_phaseElapsed = 0.0f;
    addLog("Initializing launch sequence...", LaunchStep::Type::Info);
}

void LaunchManager::update(float dt)
{
    if (!m_active || m_done)
        return;

    m_elapsed += dt * 1000.0f;
    m_phaseElapsed += dt * 1000.0f;

    std::string error;
    std::string guiBinary = ConfigManager::instance().guiBinary();

    if (m_phase == Phase::StartServer) {
        if (!ensureLogDir(error)) {
            fail(error);
            return;
        }
        if (!isExecutable("./zappy_server")) {
            fail("./zappy_server is missing or not executable");
            return;
        }
        if (!isExecutable("./zappy_ai")) {
            fail("./zappy_ai is missing or not executable");
            return;
        }
        if (!isExecutable(guiBinary)) {
            fail(guiBinary + " is missing or not executable");
            return;
        }
        addLog("./zappy_server -- found", LaunchStep::Type::Ok);
        addLog("./zappy_ai -- found", LaunchStep::Type::Ok);
        addLog(guiBinary + " -- found", LaunchStep::Type::Ok);

        int pid = -1;
        std::vector<std::string> args = buildServerArgs(m_config);
        addLog("Spawning server process...", LaunchStep::Type::Wait);
        if (!spawnProcess(args, "logs/launcher/server.log", pid, error)) {
            fail(error);
            return;
        }
        m_childPids.push_back(pid);
        addLog(joinCommand(args), LaunchStep::Type::Ok);
        m_phase = Phase::WaitServer;
        m_phaseElapsed = 0.0f;
        return;
    }

    if (m_phase == Phase::WaitServer) {
        if (!m_childPids.empty() && processExited(m_childPids.front())) {
            fail("server exited before accepting connections");
            return;
        }
        if (canConnectToPort(m_config.server.port)) {
            addLog("Server listening on :" + std::to_string(m_config.server.port),
                   LaunchStep::Type::Ok);
            m_phase = Phase::StartGui;
            m_phaseElapsed = 0.0f;
            return;
        }
        if (m_phaseElapsed >= SERVER_READY_TIMEOUT_MS) {
            fail("server did not become ready in time");
            return;
        }
        return;
    }

    if (m_phase == Phase::StartGui) {
        int pid = -1;
        std::vector<std::string> args = buildGuiArgs(m_config, guiBinary);
        addLog("Opening GUI window...", LaunchStep::Type::Wait);
        if (!spawnProcess(args, "logs/launcher/gui.log", pid, error)) {
            fail(error);
            return;
        }
        m_childPids.push_back(pid);
        addLog(joinCommand(args), LaunchStep::Type::Ok);
        m_phase = Phase::StartAi;
        m_phaseElapsed = 0.0f;
        return;
    }

    if (m_phase == Phase::StartAi) {
        if (m_phaseElapsed < AI_START_DELAY_MS)
            return;

        std::vector<AiLaunchGroup> groups = buildAiLaunchGroups(m_config);
        int total = 0;
        for (const AiLaunchGroup &group : groups)
            total += group.count;

        if (total <= 0) {
            addLog("No AI clients configured", LaunchStep::Type::Info);
            addLog("ALL PROCESSES RUNNING", LaunchStep::Type::Done);
            m_phase = Phase::Done;
            m_done = true;
            return;
        }

        addLog("Connecting " + std::to_string(total) + " AI client" +
                   (total > 1 ? "s..." : "..."),
               LaunchStep::Type::Wait);
        for (const AiLaunchGroup &group : groups) {
            std::vector<std::string> args = buildAiArgs(m_config, group.team);
            for (int i = 0; i < group.count; ++i) {
                int pid = -1;
                std::string logPath = "logs/launcher/ai_" + logSafeTeamName(group.team) + "_" +
                                      std::to_string(i + 1) + ".log";
                if (!spawnProcess(args, logPath, pid, error)) {
                    fail(error);
                    return;
                }
                m_childPids.push_back(pid);
            }
            std::string command = joinCommand(args);
            if (group.count > 1)
                command += "  x" + std::to_string(group.count);
            addLog(command, LaunchStep::Type::Ok);
        }
        addLog("ALL PROCESSES RUNNING", LaunchStep::Type::Done);
        m_phase = Phase::Done;
        m_done = true;
    }
}

bool LaunchManager::isRunning() const { return m_active; }
bool LaunchManager::isComplete() const { return m_done; }

const std::vector<LogEntry> &LaunchManager::log() const { return m_log; }

void LaunchManager::reset()
{
    m_log.clear();
    m_elapsed = 0.0f;
    m_phaseElapsed = 0.0f;
    m_childPids.clear();
    m_phase = Phase::Idle;
    m_active = false;
    m_done = false;
}

} // namespace zappy
