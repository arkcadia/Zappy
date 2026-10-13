# Documentation Technique — AI Agents (`zappy_ai`)

---

## Vue d'ensemble

Chaque processus `zappy_ai` contrôle un agent autonome. Au lancement il se connecte au serveur, effectue le handshake (WELCOME → team name → slots + map size), puis entre dans une boucle `run()` qui délègue chaque tick à un objet **rôle** interchangeable via `agent.setRole(...)`.

### Mode solo (`-s`)

Le flag `-s` active le **mode solo** : l'agent considère qu'il n'y a qu'une seule équipe sur le serveur. En mode solo, le `Leader` n'assigne jamais les rôles `Bodyguard` ni `Chaos` — tous les agents sont assignés `Worker` (ou `Ritualist` selon leur niveau). Cela concentre l'effort sur la collecte de ressources et l'incantation plutôt que sur la défense/perturbation.

Usage : `./zappy_ai -p 4242 -n pioneer -h localhost -s`

---

## Architecture des fichiers

```
src/agents/
├── main.cpp                   Point d'entrée (args: -p port -n team -h host [-s])
├── AgentConfig.hpp            Constantes de configuration (seuils, timings, slots)
├── core/
│   ├── Agent.{hpp,cpp}        État de l'agent : position, niveau, inventaire, connexion, solo flag, broadcast queue
│   ├── Broadcast.{hpp,cpp}    Encodage/décodage des broadcasts (chiffrement par team name)
│   └── Connection.{hpp,cpp}   Socket TCP (connect, readLine, sendLine)
├── state/
│   ├── Inventory.{hpp,cpp}    Compteurs de ressources (food, linemate, etc.)
│   └── World.{hpp,cpp}        Modèle interne de la carte (construit depuis Look)
└── roles/
    ├── ARole.{hpp,cpp}        Classe abstraite : helpers partagés par tous les rôles
    ├── queue.{hpp,cpp}        Rôle initial : détection de phase (Phase 1 ou Phase 2)
    ├── pioneer/               Rôle Phase 1 : exploration, incantation solo, fork, transition
    ├── leader/                Rôle Phase 2 : leader centralisé, attribution de rôles, gestion incantation
    ├── ritualist/             Rôle Phase 2 : incantation de groupe, coordination au point (0,0)
    ├── worker/                Rôle Phase 2 : collecte de ressources, dispersion, livraison à (0,0)
    ├── bodyguard/             Rôle Phase 2 : défense de périmètre autour de (0,0) (désactivé en mode solo)
    └── chaos/                 Rôle Phase 2 : perturbation des ennemis (désactivé en mode solo)
```

---

## Cycle de vie d'un agent

```
Spawn → Queue → [Pioneer | Rôle Phase 2]
```

### 1. Queue (détection de phase)

Tout agent spawne en rôle `Queue`. Il broadcast `code-5` et écoute les réponses :

| Signal reçu | Interprétation | Transition |
|---|---|---|
| `code-7` (role assignment) | Un leader Phase 2 est actif | Adopte le rôle assigné |
| `code-0` ou `code-1` (heartbeat Pioneer) | Phase 1 active, pas de Phase 2 | → `Pioneer` |
| `code-6` (beacon de ralliement) | Phase 2 active, pas encore de rôle | Continue de probe, re-envoie `code-5` |
| Silence après `QUERY_PROBES` (4) | Personne n'écoute (cold start) | → `Pioneer` |
| Timeout `QUEUE_TIMEOUT` (80) | Phase 2 active mais aucun rôle reçu | → `Worker` (défaut) |

### 2. Pioneer (Phase 1)

Machine à états : `ANNOUNCE → COLLECT → [INCANTATE | FORK | SETTLE]`

**ANNOUNCE** (`ANNOUNCE_TICKS` = 6 ticks)
- Broadcast `code-0 <id> <level>` (annonce initiale)
- Élection du leader : plus petit ID parmi les agents connus

**COLLECT** (boucle principale)
- Envoie un heartbeat `code-1` tous les `HB_TICKS` (20) ticks
- Ramasse la food sur la tuile courante
- Si `food <= MIN_FOOD + 1` (après le grace period de `FOOD_GRACE_TICKS` = 100) : chasse de la nourriture
- Si la tuile permet une incantation solo : → `INCANTATE`
- Si `level >= 2` et `food >= MIN_FOOD` et agents < `PHASE2_TOTAL` : → `FORK`
- Sinon : exploration aléatoire

**INCANTATE**
- Envoie `Incantation`, attend `Elevation underway` + `Current level: k`
- Retour à `COLLECT`

**FORK**
- Envoie un heartbeat puis `Fork`
- Retour à `COLLECT`

**Déclenchement SETTLE** (Leader uniquement, dans `_processBroadcasts`)
- Condition : `total agents frais >= PHASE2_TOTAL` (6) ET `agents level >= 2 >= PHASE2_LVL2` (6)
- Le leader broadcast `code-6` (beacon de ralliement) et passe à `SETTLE`
- Les followers reçoivent `code-6` dans `_processBroadcasts` et passent aussi à `SETTLE`

**SETTLE**
- **Leader** : émet `code-6` périodiquement. Pour chaque `code-5` reçu (arrivée d'un follower), assigne un rôle via `code-7`. Quand tous sont arrivés ou timeout (`SETTLE_TIMEOUT` = 80) : batch-assign les retardataires, s'auto-assigne, émet `code-8`, et transitionne.
- **Follower** : navigue vers la source du beacon `code-6` via `_stepTowardBeacon`. Quand `dir == 0` : émet `code-5` (arrivée). Attend `code-7` (rôle individuel) ou `code-8` (catch-all). Timeout : `SETTLE_TIMEOUT × SETTLE_FOLLOWER_MUL` (240).

**Transition Phase 2** (dans `Pioneer::tick`)
- `_transitionToPhase2 == true` → `agent.setRole(Ritualist|Bodyguard|Worker|Chaos)`
- Le leader Pioneer devient le leader Ritualist

### 3. Attribution des rôles

Deux mécanismes d'attribution coexistent :

#### Phase 1 → Phase 2 (`Pioneer::_pickRole`, dans SETTLE)

Ordre de priorité (appelé pour chaque agent au moment du SETTLE) :

| Condition | Rôle assigné | Slots max |
|---|---|---|
| `level >= 2` ET `ritualistSlotsLeft > 0` | `Ritualist` | `RITUALIST_SLOTS` (6) |
| Sinon | `Worker` | illimité |

#### Phase 2 (`Leader::_pickNewRole`, pour les latecomers)

Le Leader Phase 2 assigne les rôles aux agents qui arrivent après le SETTLE via un cycle modulo 5 :

| Slot (`_totalAssigned % 5`) | Rôle assigné |
|---|---|
| 3 | `Bodyguard` |
| 0 | `Chaos` |
| 1, 2, 4 | `Worker` |

**Mode solo (`-s`)** : `_pickNewRole` retourne toujours `Worker`, les rôles `Bodyguard` et `Chaos` ne sont jamais attribués.

---

## Rôles Phase 2

### Ritualist

Machine à états : `NAVIGATE → ARRIVED` (leader) ou `CALIBRATE → ARRIVED` (non-leader)

- **NAVIGATE** : le leader connaît sa position, utilise `_navigateTo(0, 0)`
- **CALIBRATE** : les non-leaders naviguent vers `(0,0)` via les beacons `code-6` (`_calibrate`)
- **ARRIVED** : boucle principale stationnaire à `(0,0)` :
  1. **Food** : `Inventory` puis `Take food` tant que `food < RITUALIST_LOW_FOOD` (5)
  2. **Incantation** : tous les `INCANT_CHECK_PERIOD` (5) ticks, vérifie via `Look` si les ressources sont présentes. Refuse si `food < INCANT_MIN_FOOD` (4) ou si la food sur la tuile est insuffisante. Sinon envoie `Incantation`.
  3. **Broadcast** (leader) : répond aux `code-5` (latecomers) avec `code-7` (Worker). Émet `code-6` beacon. Broadcast les besoins en ressources (`code-11`) quand l'incantation échoue par manque de pierres.
  4. **Broadcast** (non-leader) : drain la queue, émet `code-6`
  5. **Fork** (non-leader) : vérifie `Connect_nbr`, fork si `eggs < 5` et `food > MIN_FOOD + 1`
  6. **Flush** : envoie les broadcasts accumulés tous les `BEACON_FLUSH_PERIOD` (7) ticks

### Worker

Machine à états : `CALIBRATE → SCATTER → GATHER ↔ DELIVER`

- **CALIBRATE** : navigue vers `(0,0)` via `_calibrate`, puis passe à `SCATTER`
- **SCATTER** (dispersion initiale) :
  - Tourne un nombre aléatoire de fois (0–3 rotations à droite) pour partir dans une direction différente des autres Workers
  - Avance de 2–4 cases dans la direction choisie
  - Passe à `GATHER`
- **GATHER** :
  - `Inventory` → si `food >= WORKER_FOOD_CARRY` (8) : → `DELIVER`
  - `Look` → ramasse tout sur la tuile courante (sauf à `(0,0)`)
  - Se dirige vers la tuile de food la plus proche sans autres joueurs (évite le clustering). Fallback : n'importe quelle food.
  - Si aucune food visible mais des joueurs visibles : **fuit les joueurs** (tourne dans la direction opposée au joueur le plus proche)
  - Sinon : exploration aléatoire avec probabilité de changement de direction de 50% (au lieu de 33%)
- **DELIVER** :
  - `_navigateTo(0, 0)` → dépose toutes les ressources (`Set`) → `Fork` → `Forward` → retour à `GATHER`

### Bodyguard

Machine à états : `NAVIGATE → GUARD`

- **NAVIGATE** : calibre vers `(0,0)` puis navigue vers son poste (`_postCoords`) :
  - Poste 0 = `(1, 0)` face Est
  - Poste 1 = `(W-1, 0)` face Ouest
  - Poste 2 = `(0, 1)` face Sud
  - Poste 3 = `(0, H-1)` face Nord
- **GUARD** :
  - S'oriente face vers l'extérieur
  - `Look` → si `playerCount > 1` : `Eject`
  - Drain broadcasts

### Chaos

Machine à états : `CALIBRATE → RAMPAGE`

- **CALIBRATE** : navigue vers `(0,0)` via les beacons
- **RAMPAGE** :
  - Replay les broadcasts ennemis interceptés (confusion)
  - `Look` → si joueurs sur sa tuile : `Eject`
  - Sinon : chasse le joueur visible le plus proche (`_findNearestPlayer`)
  - Exploration aléatoire si aucun joueur visible

---

## Helpers partagés (ARole)

| Méthode | Description |
|---|---|
| `_sendAndRead(cmd)` | Envoie une commande serveur, retourne la réponse (gère `dead` et `message` en transit) |
| `_readNext()` | Lit la prochaine ligne non-broadcast du serveur |
| `_parseLook(raw)` | Parse la réponse `Look` en vecteur de tuiles |
| `_parseInventory(raw)` | Met à jour l'inventaire de l'agent |
| `_handleBroadcast(line)` | Parse `message k, text` et alimente la broadcast queue |
| `_navigateTo(x, y)` | Avance d'un pas vers `(x,y)` sur la carte toroïdale. Retourne `true` si arrivé. |
| `_calibrate()` | Navigue vers la source des beacons `code-6`. Lock `setPos(0,0)` à l'arrivée. |
| `_findNearestFood(tiles, lateral)` | Trouve la tuile de food la plus proche. `lateral` : `<0` = gauche, `0` = devant, `>0` = droite. |
| `_findNearestPlayer(tiles, lateral)` | Idem pour les joueurs. |
| `_surviveIfLow()` | Si `food <= LOW_FOOD` : prend le contrôle du tick pour chercher/ramasser de la food. Retourne `true` si le tick a été consommé. |
| `_stepTowardBeacon(dir, justTurned)` | Un pas vers un beacon (direction 1-8). Tourne puis avance au tick suivant. |
| `_moveForward / _turnLeft / _turnRight / _turnAround` | Primitives de mouvement |
| `_enqueueBroadcast(code, payload)` | Met un message en file d'attente |
| `_flushBroadcastBuf()` | Envoie tous les messages en une seule commande `Broadcast` (packing `code-10`) |

---

## Protocole Broadcast (Phase 1 — Pioneer)

Tous les messages sont encodés avec la clé d'équipe. Format : `<code> <sender_id> [payload]`

| Code | Payload | Émetteur | Rôle |
|---|---|---|---|
| `0` | `<level>` | Tout Pioneer | Annonce initiale + élection leader |
| `1` | `<level>` | Tout Pioneer | Heartbeat périodique (tous les 20 ticks) |
| `5` | *(vide)* | Follower arrivé au rally | Signal d'arrivée → déclenche attribution de rôle |
| `6` | *(vide)* | Leader | Beacon de ralliement (direction permet la navigation) |
| `7` | `<n> <id₁> <role₁> ...` | Leader | Attribution de rôle individuelle ou batch |
| `8` | *(vide)* | Leader | Signal catch-all de fin de SETTLE |

## Protocole Broadcast (Phase 2 — Ritualist)

| Code | Payload | Émetteur | Rôle |
|---|---|---|---|
| `5` | *(vide)* | Queue (latecomer) | Demande de rôle |
| `6` | *(vide)* | Ritualist (leader + non-leader) | Beacon continu pour calibration des nouveaux |
| `7` | `<n> <id₁> <role₁> ...` | Ritualist leader | Attribution de rôle aux latecomers |
| `11` | `<lin> <der> <sib> <men> <phi> <thy>` | Ritualist leader | Besoins en ressources pour la prochaine incantation |

---

## AgentConfig — Constantes

| Constante | Valeur | Description |
|---|---|---|
| `MIN_FOOD` | 2 | Stock minimal pour fork / agir |
| `FOOD_GRACE_TICKS` | 100 | Ticks post-spawn avant d'activer la survie (buffer serveur caché) |
| `PHASE2_TOTAL` | 6 | Agents nécessaires pour déclencher SETTLE |
| `PHASE2_LVL2` | 6 | Dont au moins N au niveau ≥ 2 |
| `STALE_TICKS` | 100 | Durée de validité d'un heartbeat |
| `HB_TICKS` | 20 | Période d'envoi des heartbeats |
| `ANNOUNCE_TICKS` | 6 | Durée de la phase ANNOUNCE |
| `SETTLE_TIMEOUT` | 80 | Ticks max avant finalisation du leader |
| `SETTLE_FOLLOWER_MUL` | 3 | Multiplicateur timeout follower (80 × 3 = 240) |
| `RITUALIST_SLOTS` | 6 | Nombre max de Ritualists |
| `BODYGUARD_SLOTS` | 4 | Nombre max de Bodyguards |
| `BEACON_FLUSH_PERIOD` | 7 | Période de flush des beacons |
| `RITUALIST_LOW_FOOD` | 5 | Seuil de food du Ritualist pour Take food |
| `INCANT_CHECK_PERIOD` | 5 | Période de vérification d'incantation (ticks ARRIVED) |
| `INCANT_MIN_FOOD` | 4 | Food minimum pour lancer une incantation (300 ticks ≈ 2.4 food) |
| `WORKER_FOOD_CARRY` | 4 | Food minimum avant de livrer à (0,0) |
| `QUERY_PROBES` | 4 | Sondages avant conclusion en Queue |
| `QUEUE_TIMEOUT` | 80 | Timeout total en Queue |

---

## Incantation (côté serveur)

Le serveur gère l'incantation en deux phases :

1. **Initiation** : le drone envoie `Incantation`. Le serveur vérifie via `canIncantate()` : assez de joueurs du même niveau sur la tuile (`players_required` : 1/2/2/4/4/6/6) + assez de ressources. Si OK → `Elevation underway` immédiat + le client est marqué `incanting = true`. Sinon → `ko`.

2. **Résolution** (300 ticks plus tard) : le serveur re-vérifie les conditions en ne comptant que les drones marqués `incanting` sur la même tuile au même niveau. Si OK → consomme les ressources 1×, monte en niveau tous les drones incantant éligibles, envoie `Current level: k` à chacun. Sinon → `ko`.

Les notifications GUI (`pic`, `pie`, `plv`) sont envoyées à chaque étape.
