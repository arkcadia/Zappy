# Transition Phase 1 → Phase 2 — Cartographie du Flux Actuel

---

## 📌 Introduction & Objectif du Flow

Ce document décrit le flux de transition entre la **Phase 1** (bootstrapping multi-agent) et la **Phase 2** (jeu en rôles spécialisés) du système d'IA Zappy.

L'objectif de cette transition est de passer d'une équipe d'agents `Pioneer` homogènes — qui collectent des ressources, se nivellent et se reproduisent — à une équipe hétérogène où chaque agent reçoit un rôle spécialisé (`Ritualist`, `Worker`, et optionnellement `Bodyguard`/`Chaos`) et exécute une stratégie coordonnée de montée en niveau jusqu'au niveau 8. En **mode solo** (`-s`), les rôles `Bodyguard` et `Chaos` sont désactivés car il n'y a pas d'ennemi.

Le flux implique une **élection de Leader** (plus petit ID parmi les agents vivants), une **phase de rallye** en `(0,0)`, et une **attribution de rôles** diffusée via broadcast.

---

## 🔄 Cartographie Séquentielle du Flux Actuel

### Étape 0 — Démarrage & Queue (tout nouvel agent)

Chaque agent spawne avec le rôle `Queue`, qui sert de détecteur de phase :

1. L'agent envoie un broadcast `code-5` (signal de présence/requête de phase).
2. Il attend jusqu'à `QUERY_PROBES` (4) cycles `Inventory`.
3. Selon la réponse reçue :
   - `code-7` → Phase 2 déjà active → adoption du rôle assigné.
   - `code-0` ou `code-1` → Phase 1 active → transition vers `Pioneer`.
   - `code-8` → Phase 2, fallback `Worker`.
   - Silence après timeout → Phase 1, cold start → transition vers `Pioneer`.

---

### Étape 1 — ANNOUNCE (Pioneer, durée : `ANNOUNCE_TICKS` = 6 ticks)

- Chaque `Pioneer` émet un broadcast `code-0 <id> <level>` (annonce initiale).
- Chaque agent écoute les annonces des pairs et met à jour sa table `_agentLevels`.
- **Élection du Leader** : l'agent ayant le plus petit ID connu prend automatiquement le rôle de Leader (`_isLeader = true`). L'élection est réévaluée à chaque tick.
- Après `ANNOUNCE_TICKS` ticks → transition vers `COLLECT`.

---

### Étape 2 — COLLECT (Pioneer, boucle principale)

Tous les agents, Leader inclus, exécutent la même boucle :

- **Heartbeat** : toutes les `HB_TICKS` (20) ticks, chaque agent envoie `code-1 <id> <level>` pour signaler sa présence et son niveau.
- **Survie** : si `food <= LOW_FOOD` (3), l'agent chasse de la nourriture prioritairement.
- **Incantation opportuniste** : si la tuile courante réunit les conditions → switch vers `INCANTATE`.
- **Fork** : si `level >= 2` et `food >= MIN_FOOD` (2) et que le nombre d'agents connus est insuffisant → switch vers `FORK` (pond un œuf).
- **Exploration** : déplacement aléatoire en avant avec virages occasionnels.

**Vérification de seuil (Leader uniquement)** — exécutée à chaque tick dans `_processBroadcasts` :

```
total (agents "frais" connus) >= PHASE2_TOTAL (15)
  ET
atLvl2 (agents niveau >= 2) >= PHASE2_LVL2 (6)
```

Dès que ces deux conditions sont réunies → le Leader déclenche la **Phase SETTLE**.

---

### Étape 3 — Déclenchement de SETTLE (Leader)

Le Leader :
1. Se positionne virtuellement en `(0,0)` (`agent.setPos(0, 0)`) et se marque `_arrivedAtRally = true`.
2. Fixe `_settleTarget = PHASE2_TOTAL - 1 = 14` (nombre de followers attendus).
3. Émet un broadcast `code-6` (balise de rallye).
4. Passe à l'état `SETTLE`.

Les followers reçoivent `code-6` dans `_processBroadcasts` et passent immédiatement à `SETTLE`.

---

### Étape 4 — SETTLE : Navigation des Followers vers `(0,0)`

Chaque follower exécute `_settleNavigate` :

1. Écoute les broadcasts `code-6` pour connaître la direction de la balise.
2. Avance pas à pas vers la source via `_stepTowardBeacon()`.
3. Après chaque pas `Forward`, envoie un `Inventory` pour actualiser la position (flush de balise périmée).
4. Quand la balise arrive en direction `0` (source = même tuile) → le follower émet `code-5` et se marque arrivé.

---

### Étape 5 — SETTLE : Attribution des Rôles (Leader, streaming)

Pour chaque `code-5` reçu d'un follower :

1. Le Leader note l'arrivant (`_settleArrivals`).
2. Calcule son rôle via `_pickRole(level)` :
   - `Ritualist` : `level >= 2` ET `ritualistSlotsLeft > 0` (6 slots max).
   - `Bodyguard` : `bodyguardSlotsLeft > 0` (4 slots max).
   - `Worker` : fallback.
3. Met en file `code-7 <n> <targetId> <roleType>` (broadcast d'attribution individuel).
4. Flush le buffer de broadcast immédiatement.

Le Leader continue d'émettre `code-6` (balise) toutes les `BEACON_FLUSH_PERIOD` (7) ticks pour guider les retardataires.

---

### Étape 6 — SETTLE : Finalisation & Signal de Départ

Le Leader finalise dès que l'une des deux conditions est atteinte :

| Condition | Valeur |
|---|---|
| Tous les slots assignés | `_settleArrivals.size() >= _settleTarget` (14) |
| Timeout | `_settleTicks >= SETTLE_TIMEOUT` (40 ticks) |

À la finalisation :
1. Le Leader s'assigne lui-même un rôle (`_pickRole`).
2. Émet `code-8` (signal catch-all de départ général).
3. Met `_transitionToPhase2 = true`.

---

### Étape 7 — Transition de Rôle (tous les agents)

Dans `Pioneer::tick()`, après chaque appel à `_doSettle()` :

```
if (_transitionToPhase2)
    agent.setRole( make_unique<Leader|Ritualist|Bodyguard|Worker|Chaos>() )
```

- Le Leader Pioneer s'auto-assigne le rôle `Leader` (Phase 2).
- Un follower qui a reçu `code-7` part **immédiatement** sans attendre `code-8`.
- Un follower sans `code-7` attend `code-8` (catch-all) ou son timeout `SETTLE_TIMEOUT * SETTLE_FOLLOWER_MUL` (120 ticks) → défaut `Worker`.
- En **mode solo** (`-s`), le Leader Phase 2 n'assigne jamais `Bodyguard` ni `Chaos` aux latecomers — tous reçoivent `Worker`.

---

## 🚦 États et Conditions de Déclenchement

### Déclencheur de la Phase de Rassemblement

Le Leader, dans l'état `COLLECT`, évalue à chaque tick (via `_processBroadcasts`) :

```
total >= 15  (agents "frais" dans _agentLevels + soi-même)
    ET
atLvl2 >= 6  (d'entre eux ont level >= 2)
```

Un pair est "frais" si son dernier heartbeat date de moins de `STALE_TICKS` (100) ticks locaux. Un pair périmé est ignoré du comptage — le seuil peut ainsi ne jamais être atteint si les agents ne heartbeatent pas assez fréquemment.

### Condition de Validation du Leader

Le Leader considère la phase SETTLE terminée dès que :

- **Condition nominale** : `_settleArrivals.size() >= 14` (tous les followers attendus ont émis `code-5`).
- **Timeout de sécurité** : `_settleTicks >= 40` ticks écoulés depuis le déclenchement du SETTLE.

Le timeout garantit que le jeu démarre même si des agents sont morts en route ou n'ont pas reçu la balise.

### Condition de Relance (Phase 2)

Les agents en attente partent au premier des deux événements suivants :

| Signal | Source | Effet |
|---|---|---|
| `code-7` individuel | Leader | Départ immédiat avec rôle assigné |
| `code-8` catch-all | Leader | Départ immédiat, rôle = défaut (`Worker` si pas de `code-7` reçu) |
| Timeout follower | Local | 120 ticks max → départ forcé comme `Worker` |

---

## 📡 Protocoles de Communication (Broadcasts)

Tous les messages sont encodés avec le protocole d'équipe (clé basée sur le nom de la team) et préfixés `<code> <sender_id> [payload]`.

| Code | Émetteur | Contenu | Écouteurs | Impact |
|---|---|---|---|---|
| `code-0` | Tout `Pioneer` | `<id> <level>` | Tous `Pioneer` | Annonce initiale, mise à jour `_agentLevels`, élection du Leader |
| `code-1` | Tout `Pioneer` | `<id> <level>` | Tous `Pioneer` | Heartbeat périodique (tous les 20 ticks), maintien de la fraîcheur des pairs |
| `code-5` | Follower arrivé en `(0,0)` | `<id>` | Leader (Pioneer SETTLE ou Ritualist) | Signale l'arrivée → déclenche l'attribution d'un rôle via `code-7` |
| `code-6` | Leader | _(vide)_ | Tous followers en `SETTLE` | Balise de navigation ; la direction indique vers où marcher ; `dir=0` = on est arrivé |
| `code-7` | Leader | `<n> <id> <role> ...` | Tous `Pioneer` / `Queue` | Attribution de rôle individuelle ; le destinataire qui reconnaît son ID part en Phase 2 |
| `code-8` | Leader | _(vide)_ | Tous `Pioneer` / `Queue` | Signal catch-all de fin de SETTLE ; tout agent restant passe en Phase 2 (rôle `Worker` si pas de `code-7` reçu) |

---

## ⚠️ Points de Blocage & Goulots d'Étranglement Identifiés

### 1. Seuil de déclenchement rigide

Le seuil `PHASE2_TOTAL = 15` agents frais est vérifié en temps réel par le Leader, mais il repose uniquement sur les heartbeats (`code-1`) reçus. Un agent qui se trouvent dans une zone sans broadcast reach, ou qui n'a pas heartbeaté depuis `STALE_TICKS` (100) ticks, **n'est pas compté**. Le rallye peut être retardé indéfiniment si l'équipe atteint 14 agents stables mais que le 15ème est considéré périmé.

### 2. Synchronisation bloquante du Leader

Bien que l'attribution des rôles soit en **streaming** (un `code-7` dès chaque arrivée de `code-5`), **le Leader ne s'auto-assigne pas et n'émet pas `code-8` avant que tous les `_settleTarget` (14) followers aient confirmé leur présence** — ou que le timeout de 40 ticks soit écoulé.

```
Situation nominale :
Leader attend code-5 de 14 agents → émet code-8 → Phase 2 démarre

Situation dégradée (1 agent manquant) :
Leader attend 40 ticks → code-8 → Phase 2 démarre avec retard
```

Ce timeout de 40 ticks est un **goulot d'étranglement systématique** dès qu'un seul agent est mort, bloqué par la faim, ou hors de portée de broadcast.

### 3. Latence de navigation des followers

La navigation vers `(0,0)` repose sur les balises `code-6` émises toutes les `BEACON_FLUSH_PERIOD = 7` ticks, entrecoupées d'`Inventory` (synchrones, 1 tick server). La convergence est donc lente sur de grandes maps : chaque pas vers la balise coûte au minimum 2 ticks server (1 turn + 1 forward), plus 1 `Inventory` de flush. Un agent à distance Manhattan 10 de `(0,0)` peut mettre 30–60 ticks locaux pour arriver.

### 4. Timeout follower 3× plus long

Le timeout des followers (`SETTLE_TIMEOUT * SETTLE_FOLLOWER_MUL = 120 ticks`) est destiné à couvrir le temps de navigation, mais il signifie qu'un follower dont le `code-7` se perd en transit attend jusqu'à 120 ticks locaux avant de fallback en `Worker`, **même si `code-8` a déjà été émis par le Leader**. L'agent doit avoir reçu `code-8` pour partir tôt — si ce broadcast est également manqué, il attend son timeout complet.

### 5. Récapitulatif des délais dans le pire cas

| Étape | Durée nominale | Durée pire cas |
|---|---|---|
| ANNOUNCE | 6 ticks | 6 ticks |
| COLLECT (bootstrap équipe) | Variable (FORK + incantations) | Illimitée si seuil jamais atteint |
| Navigation SETTLE (follower) | ~10–30 ticks | ~60 ticks (grande map) |
| Attente Leader avant code-8 | 0–40 ticks | 40 ticks (timeout) |
| Attente follower sans code-7/8 | 0 ticks | 120 ticks (timeout follower) |
