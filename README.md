# Zappy

**A networked strategy simulation where autonomous agents survive, cooperate, and evolve in a shared world.**

Zappy brings together a TCP game server, a team of autonomous C++ agents, and live 2D and 3D visualizations. On the planet Trantor, agents explore a world that wraps at its edges, gather food and minerals, and coordinate rituals to reach level 8. Every decision has a cost: collecting resources keeps an agent alive, while progressing requires teammates to meet in the right place with the right supplies.

The project combines systems programming, agent coordination, and real-time graphics in a single C++23 codebase.

## Highlights

- **Shared simulation:** a server manages the map, resources, teams, player lifecycles, and queued actions through non-blocking TCP sockets and `poll`.
- **Autonomous teamwork:** agents use role-based state machines, leader election, directional broadcasts, and a shared rally point to organize exploration and progression.
- **Two ways to watch:** a 2D tactical viewer and a 3D viewer built with raylib display the same server-driven world.
- **Graphical launcher:** configure the world, teams, agent counts, and viewer, then start the simulation from one interface. Settings persist in JSON.
- **Observable behavior:** protocol events expose movement, inventories, resource collection, eggs, broadcasts, rituals, and deaths; launch scripts collect process logs.

## How it works

Trantor is a toroidal tile map: crossing one edge brings an agent back on the opposite side. Food sustains life, and six minerals provide the ingredients for elevation rituals: linemate, deraumere, sibur, mendiane, phiras, and thystame. Resources are replenished periodically.

Agents start at level 1 with limited vision. Higher levels expand their field of view, but further progression requires more resources and more participants of the same level on the same tile. Agents can also lay eggs to open new connection slots and eject other players from a tile.

Each AI process controls one player. Agents communicate through server broadcasts, using the direction of incoming messages to navigate toward teammates. Their strategy develops in two phases:

1. **Explore and establish a team.** Pioneers gather food, perform the first elevation, create eggs, exchange heartbeats, and elect a leader.
2. **Organize around a rally point.** The leader assigns roles and announces resource needs; workers gather and deliver supplies; ritualists assemble for coordinated elevation. Later arrivals can also become bodyguards or disruption agents.

| Role | Responsibility |
| --- | --- |
| Pioneer | Explore, survive, reach level 2, and prepare the rally |
| Leader | Assign roles, announce missing resources, and signal ritual readiness |
| Worker | Gather food and minerals, then deliver them to the rally point |
| Ritualist | Assemble with teammates and perform group incantations |
| Bodyguard | Patrol positions around the rally and eject nearby players |
| Chaos | Seek other players, eject them, and replay intercepted broadcasts |

The AI also supports a solo-team mode (`-s`) that disables bodyguard and chaos assignments for new arrivals, concentrating those agents on gathering.

## Architecture

| Component | Executable | Implementation |
| --- | --- | --- |
| Simulation server | `zappy_server` | C++23, POSIX sockets, `poll`, world and player models |
| Autonomous client | `zappy_ai` | C++23, TCP connection, interchangeable roles and state machines |
| 2D viewer | `zappy_gui` | raylib, tile inspection, camera controls, animated player events |
| 3D viewer | `zappy_gui3d` | raylib, 3D world rendering, camera navigation, HUD |
| Launcher | `zappy_launcher` | raylib interface, JSON configuration, process management |

The server is the source of truth. AI clients send newline-delimited commands and receive responses; graphical clients authenticate as `GRAPHIC` and consume world updates. Both viewers share the graphical protocol handler and client-side world model.

Broadcast messages carry a sender ID and sequence number to filter stale or duplicate messages. Multiple coordination messages can be packed into a single broadcast. This lets the agent layer coordinate roles and rituals using the game's own communication channel.

## Build

The current build targets **Linux** and uses:

- `clang++` with C++23 support and a standard library that provides `<format>`;
- GNU Make;
- raylib headers and library;
- OpenGL and X11 development libraries;
- the header-only `nlohmann/json` library for the launcher;
- Bash for the launch scripts; Python 3 for the optional log report.

With those dependencies installed:

```bash
git clone https://github.com/arkcadia/Zappy.git
cd Zappy
make
```

This builds all five executables. Run them from the repository root so that fonts, textures, and models in `assets/` can be found.

For development, `make debug` enables debug logging and symbols; `make verbose` adds more detailed logging. `make clean` removes build objects, and `make fclean` also removes executables and logs.

## Run a simulation

### Graphical launcher

```bash
./zappy_launcher
```

Configure server dimensions, teams, connection slots, frequency, agent counts, and the 2D or 3D frontend in the launcher. Configuration is saved to `zappy_config.json`, and launched processes write logs under `logs/launcher/`.

### Scripted demo

Start a 2D simulation with six initial agents and maintain a target population of twenty as connection slots become available:

```bash
bash run.sh -x 20 -y 20 -n explorers -c 6 -a 20 -f 10 -s
```

Add `--3d` to use the 3D viewer. Press `Ctrl+C` to stop the launched processes. Logs are saved under `logs/`; when Python 3 is available, the script produces an agent activity report on shutdown. Use `make debug` or `make verbose` before launching to capture detailed agent activity.

For a fixed number of clients, `launch_all.sh` offers a simpler launch sequence:

```bash
bash launch_all.sh --teams "blue orange" --ai-team blue -c 6 -a 6 -f 10
```

This creates both server teams and launches six agents for `blue`. Add `--gui zappy_gui3d` to switch viewers. Its logs are stored in `logs/launch_all/`.

### Start components separately

Run the server first:

```bash
./zappy_server -p 4242 -x 20 -y 20 -n blue orange -c 6 -f 10
```

Then open a viewer in another terminal:

```bash
./zappy_gui -p 4242 -h localhost
# Or: ./zappy_gui3d -p 4242 -h localhost
```

Start an agent in another terminal; repeat for additional players and teams:

```bash
./zappy_ai -p 4242 -n blue -h localhost
```

The server's `-c` argument sets the initial slots per team. `Fork` creates eggs that allow additional clients to join. The `-f` argument controls simulation frequency; lower values make the action easier to observe.

## Viewer controls

| Action | 2D viewer | 3D viewer |
| --- | --- | --- |
| Move the camera | Drag with right or middle mouse button | Arrow keys |
| Zoom | Mouse wheel | `Z` / `X` |
| Adjust camera height | — | `W` / `S` |
| Rotate the view | — | Drag with left mouse button |
| Move tile selection | Arrow keys or `W` / `A` / `S` / `D` after selecting a tile | — |
| Reset camera | `R` | `R` |
| Exit | `Esc` | `Esc` |

## Validation

```bash
make tests
```

The current Makefile runs eight graphical protocol scenarios without requiring raylib. They cover map initialization, players and teams, positions, inventories and resource events, broadcasts and deaths, incantations, eggs, and malformed messages.

Additional Catch2 test sources cover the server and world components in `tests/test_*.cpp`; these are not wired into the current `make tests` target.

## Explore the code

```text
src/
├── server/   TCP server, command scheduling, AI and GUI protocols
├── map/      World, tiles, teams, drones, and elevation rules
├── agents/   Agent core, local state, broadcasts, and strategy roles
├── gui/      2D rendering, shared networking, and graphical world model
├── gui3d/    3D rendering, camera, and HUD
└── menu/     Launcher interface, configuration, and process startup
tests/       Protocol scenarios and Catch2 unit test sources
assets/      Fonts, textures, and 3D models
tools/       Agent log analysis
docs/        Agent architecture notes and generated documentation
specs/       Design notes and communication protocol descriptions
```

Zappy connects three engineering challenges: maintaining a consistent shared simulation, coordinating autonomous agents with limited information, and translating network events into an inspectable visual world.
