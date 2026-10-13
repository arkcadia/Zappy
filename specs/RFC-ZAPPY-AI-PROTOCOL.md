# RFC-ZAPPY-001 — Zappy AI-Server Communication Protocol

**Network Working Group:** Zappy Team  
**Category:** Informational  
**Date:** June 2026

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Conventions and Terminology](#2-conventions-and-terminology)
3. [Connection Model](#3-connection-model)
4. [Time Model](#4-time-model)
5. [Command Queue](#5-command-queue)
6. [Resources](#6-resources)
7. [Command Reference](#7-command-reference)
   - 7.1 [Forward](#71-forward)
   - 7.2 [Right](#72-right)
   - 7.3 [Left](#73-left)
   - 7.4 [Look](#74-look)
   - 7.5 [Inventory](#75-inventory)
   - 7.6 [Broadcast](#76-broadcast)
   - 7.7 [Connect_nbr](#77-connect_nbr)
   - 7.8 [Fork](#78-fork)
   - 7.9 [Eject](#79-eject)
   - 7.10 [Take](#710-take)
   - 7.11 [Set](#711-set)
   - 7.12 [Incantation](#712-incantation)
8. [Server-Initiated Messages](#8-server-initiated-messages)
   - 8.1 [dead](#81-dead)
   - 8.2 [message k, text](#82-message-k-text)
9. [Error Handling](#9-error-handling)
10. [Examples](#10-examples)
11. [Security Considerations](#11-security-considerations)

---

## 1. Introduction

The Zappy game server hosts a shared world map populated by players belonging to competing teams. Each AI client controls one player unit. Players collect resources, level up through incantation rituals, and compete for map dominance.

The protocol is strictly request/response: the client sends a command line and the server eventually replies with one response line. The server may also emit unsolicited notifications (see [Section 8](#8-server-initiated-messages)).

---

## 2. Conventions and Terminology

The key words **MUST**, **MUST NOT**, **REQUIRED**, **SHALL**, **SHOULD**, **MAY** are to be interpreted as described in RFC 2119.

| Term | Definition |
|------|------------|
| Client | The AI program controlling one player unit |
| Server | The Zappy game server |
| `f` | Server frequency (ticks per second), set at startup via `-f`. All time limits are multiples of `(1/f)` seconds |
| tile | One cell of the world grid |
| object/resource | An item that can be placed on or picked up from a tile |
| `ok` | Server acknowledgement: command succeeded |
| `ko` | Server rejection: command could not be executed |

---

## 3. Connection Model

**Transport:** TCP, server port configured at startup.

### Handshake Sequence

```
S -> C: "WELCOME\n"
C -> S: "<team-name>\n"
S -> C: "<unused-slots>\n"         # Remaining connection slots for the team
S -> C: "<map-width> <map-height>\n"  # Dimensions of the world map
```

After the handshake the client enters normal operation and **MAY** begin sending commands immediately.

---

## 4. Time Model

Every command has a cost expressed as a number of time units `T`, where one time unit equals `(1/f)` seconds. The server executes a command after `T` time units have elapsed since the command was dequeued. The response is sent at that moment.

### Command Costs

| Cost | Commands |
|------|----------|
| `7/f` | Most movement and interaction commands |
| `1/f` | Inventory |
| `42/f` | Fork |
| `300/f` | Incantation |

> A player that runs out of food while a command is pending will receive `dead` before the command's response (see [Section 8.1](#81-dead)).

---

## 5. Command Queue

The server maintains a FIFO queue of up to **10 pending commands** per client. Commands beyond the limit **MUST** be rejected by the server without response. Clients **SHOULD** track their own queue depth to avoid silent loss.

---

## 6. Resources

| Resource | Description |
|----------|-------------|
| `food` | Sustains the player — one unit = 126 time units of life |
| `linemate` | Ritual stone, level 1 ingredient |
| `deraumere` | Ritual stone, level 2 ingredient |
| `sibur` | Ritual stone, level 3 ingredient |
| `mendiane` | Ritual stone, level 4 ingredient |
| `phiras` | Ritual stone, level 5 ingredient |
| `thystame` | Ritual stone, level 6 ingredient |

Resources are listed in the fixed order above in `Look` responses and `Inventory` responses.

---

## 7. Command Reference

> Unless noted otherwise, all commands are terminated with `\n` and all responses are terminated with `\n`.

---

### 7.1. Forward

| | |
|-|---|
| **Action** | Move the player one tile in the direction it faces |
| **Syntax** | `Forward\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` |

The player advances one tile forward. World boundaries wrap around (toroidal map): moving off an edge places the player on the opposite side.

---

### 7.2. Right

| | |
|-|---|
| **Action** | Rotate the player 90 degrees clockwise |
| **Syntax** | `Right\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` |

---

### 7.3. Left

| | |
|-|---|
| **Action** | Rotate the player 90 degrees counter-clockwise |
| **Syntax** | `Left\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` |

---

### 7.4. Look

| | |
|-|---|
| **Action** | Observe all visible tiles in the player's field of view |
| **Syntax** | `Look\n` |
| **Time cost** | `7/f` |
| **Response** | `[ <tile-0>, <tile-1>, ..., <tile-N> ]\n` |

The response is a bracketed, comma-separated list of tile contents. Tile 0 is the player's current tile. Tiles are ordered left-to-right and front-to-back within each row at increasing distance.

Level `L` provides vision over `(2L+1)` tiles per row across `L+1` rows, for a total of `(L+1)²` tiles. Each tile is a space-separated list of objects; an empty tile is an empty string between commas.

**Example response (level 1 player):**
```
[ player food, linemate, , sibur ]
```

---

### 7.5. Inventory

| | |
|-|---|
| **Action** | Query the player's current resource inventory |
| **Syntax** | `Inventory\n` |
| **Time cost** | `1/f` |
| **Response** | `[ l <n>, d <n>, s <n>, m <n>, p <n>, t <n>, f <n> ]\n` |

`<n>` is a non-negative integer count for each resource.

---

### 7.6. Broadcast

| | |
|-|---|
| **Action** | Send a text message to all connected players |
| **Syntax** | `Broadcast <text>\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` |

`<text>` is an arbitrary UTF-8 string with no embedded newlines. All other players receive the message as an unsolicited notification (see [Section 8.2](#82-message-k-text)).

---

### 7.7. Connect_nbr

| | |
|-|---|
| **Action** | Query the number of unused team connection slots |
| **Syntax** | `Connect_nbr\n` |
| **Time cost** | `0` (immediate) |
| **Response** | `<value>\n` |

`<value>` is a non-negative integer. A value of `0` means the team is full; no new members can join until a current member forks.

---

### 7.8. Fork

| | |
|-|---|
| **Action** | Lay an egg on the current tile, creating one extra team connection slot |
| **Syntax** | `Fork\n` |
| **Time cost** | `42/f` |
| **Response** | `ok\n` |

After the response, a new client **MAY** connect using the team name and inherit the slot created by this Fork.

---

### 7.9. Eject

| | |
|-|---|
| **Action** | Push all other players on the current tile one tile in the direction the ejecting player faces |
| **Syntax** | `Eject\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` (at least one player ejected) / `ko\n` (no other players on tile) |

Ejected players receive an unsolicited `eject K\n` notification, where `K` is the direction from which the push originated (1–8, same encoding as Broadcast direction).

---

### 7.10. Take

| | |
|-|---|
| **Action** | Pick up one unit of a named resource from the current tile |
| **Syntax** | `Take <object>\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` (object picked up) / `ko\n` (object not on tile) |

`<object>` must be one of the resource names listed in [Section 6](#6-resources).

---

### 7.11. Set

| | |
|-|---|
| **Action** | Drop one unit of a named resource from inventory onto the current tile |
| **Syntax** | `Set <object>\n` |
| **Time cost** | `7/f` |
| **Response** | `ok\n` (object placed) / `ko\n` (object not in inventory) |

`<object>` must be one of the resource names listed in [Section 6](#6-resources).

---

### 7.12. Incantation

| | |
|-|---|
| **Action** | Begin a level-up ritual on the current tile |
| **Syntax** | `Incantation\n` |
| **Time cost** | `300/f` |

**Responses:**
- On initiation (immediate): `Elevation underway\n`
- On success (after `300/f`): `Current level: <k>\n` where `<k>` is the new level (2–8)
- On failure (wrong players/resources at resolution): `ko\n`

**Requirements per target level:**

| Level | Players | linemate | deraumere | sibur | mendiane | phiras | thystame |
|-------|---------|----------|-----------|-------|----------|--------|----------|
| 2     | 1       | 1        | 0         | 0     | 0        | 0      | 0        |
| 3     | 2       | 1        | 1         | 1     | 0        | 0      | 0        |
| 4     | 2       | 2        | 0         | 1     | 0        | 2      | 0        |
| 5     | 4       | 1        | 1         | 2     | 0        | 1      | 0        |
| 6     | 4       | 1        | 2         | 1     | 3        | 0      | 0        |
| 7     | 6       | 1        | 2         | 3     | 0        | 1      | 0        |
| 8     | 6       | 2        | 2         | 2     | 2        | 2      | 1        |

All participating players must be on the same tile and send `Incantation`. All required stones must be on that tile. All participants advance to the new level on success; stones are consumed from the tile.

---

## 8. Server-Initiated Messages

The server **MAY** send the following messages at any time, interleaved with command responses. Clients **MUST** handle them asynchronously.

### 8.1. dead

Sent when the player's food count reaches zero.

```
S -> C: "dead\n"
```

The connection is closed by the server immediately after. The client **SHOULD NOT** send further commands after receiving `dead`.

### 8.2. message k, text

Sent when another player broadcasts a message.

```
S -> C: "message <k>, <text>\n"
```

| Field | Description |
|-------|-------------|
| `<k>` | Integer 1–8: direction of the sender relative to the receiver (1 = north, then clockwise). `0` means sender and receiver are on the same tile |
| `<text>` | The original broadcast text |

---

## 9. Error Handling

| Situation | Behaviour |
|-----------|-----------|
| Unknown command | Server responds `ko\n` |
| Malformed arguments | Server responds `ko\n` |
| Queue full (> 10 commands) | Server **MAY** silently drop the command |
| Player dead mid-queue | `dead\n` is sent; pending responses are discarded and the connection is closed |

---

## 10. Examples

### Movement sequence
```
C -> S: Forward
S -> C: ok

C -> S: Right
S -> C: ok

C -> S: Look
S -> C: [ player food, linemate, , ]
```

### Pick up and drop
```
C -> S: Take linemate
S -> C: ok

C -> S: Set linemate
S -> C: ok
```

### Failed take
```
C -> S: Take thystame
S -> C: ko
```

### Incantation
```
C -> S: Incantation
S -> C: Elevation underway
... (300/f seconds pass) ...
S -> C: Current level: 2
```

### Broadcast received mid-command
```
C -> S: Forward
S -> C: message 3, 1 [42 2]    ← unsolicited, from teammate
S -> C: ok                      ← response to Forward
```

### Death during pending command
```
C -> S: Incantation
S -> C: Elevation underway
... (food exhausted before ritual completes) ...
S -> C: dead
[connection closed]
```

---

## 11. Security Considerations

The protocol transmits no credentials beyond the team name. Broadcast messages from other teams are visible to all players; clients **SHOULD** implement an application-level encryption or encoding scheme (e.g., XOR with a team-derived key) to prevent information leakage to enemy AI clients monitoring broadcasts.

See [broadcast_protocol.md](broadcast_protocol.md) for the team-internal broadcast encoding convention used by this implementation.

---

*End of RFC-ZAPPY-001*
