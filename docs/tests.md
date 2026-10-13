# Tests — Zappy

## Pourquoi Catch2 ?

Catch2 est un framework de tests unitaires C++ mature et largement adopté dans l'industrie. On l'a choisi pour les raisons suivantes :

- **Zéro friction d'installation** : disponible directement via les gestionnaires de paquets (`pacman -S catch2` sur Arch, `apt install libcatch2-dev` sur Ubuntu). Pas de submodule git, pas de header à télécharger.
- **Pas besoin d'écrire un `main()`** : Catch2 fournit son propre point d'entrée via `-lCatch2Main`. On écrit uniquement les tests.
- **Syntaxe expressive** : les macros `REQUIRE`, `REQUIRE_FALSE`, `REQUIRE_THROWS_AS` et `REQUIRE_NOTHROW` rendent les assertions lisibles sans documentation.
- **Exécution randomisée** : Catch2 mélange l'ordre des tests à chaque run (avec un seed affiché), ce qui aide à détecter les dépendances cachées entre tests.
- **Intégration CI simple** : un code de retour non-zéro en cas d'échec suffit pour intégrer avec n'importe quel pipeline CI.

## Comment lancer les tests

```bash
make test
```

Cette commande :
1. Compile les 8 fichiers de tests Catch2 (`tests/test_*.cpp`) et les lie avec les sources du projet (map + server, sans le `main`) pour produire le binaire `zappy_tests`
2. Compile les tests du protocole GUI (`tests/gui_protocol_tests.cpp`) pour produire le binaire `test_gui_protocol`
3. Exécute les deux binaires séquentiellement

Le workflow CI (`.github/workflows/ci-mirror.yml`) lance `make test` automatiquement après le build.

## Types de tests

Le projet utilise deux types de tests :

### Tests unitaires (Catch2)

90 test cases couvrant les composants serveur et map. Chaque fichier teste une classe ou un module isolé.

### Tests d'intégration protocole (custom)

8 scénarios de bout en bout qui simulent des échanges complets du protocole GUI (réception de messages serveur, mise à jour du monde, gestion des animations). Ces tests utilisent un framework minimal fait maison (`gui_protocol_tests.cpp`) car ils testent des interactions entre plusieurs composants GUI sans dépendre de raylib.

## Couverture des tests

### 1. Vector2 — `tests/test_vector.cpp` (22 tests)

Teste la struct template `Vector2<T>` utilisée pour toutes les positions et déplacements sur la map.

| Test | Ce qu'il vérifie |
|------|-----------------|
| Default constructor | `Vector2(0,0)` initialise à zéro |
| Parameterized constructor | Les valeurs x/y sont correctement assignées |
| Unary negation | `-Vector2(3,-5)` donne `(-3,5)` |
| Addition / Subtraction | Opérations de base entre vecteurs |
| += / -= operators | Opérations en place |
| Scalar multiplication (vec * scalar) | `(3,4) * 2 = (6,8)` |
| Scalar multiplication (scalar * vec) | Commutativité : `3 * (3,4) = (9,12)` |
| *= / /= operators | Multiplication et division en place |
| Modulo wrapping (positif) | `(12,15) % (10,10) = (2,5)` |
| Modulo wrapping (négatif) | `(-1,-3) % (10,10) = (9,7)` |
| Modulo wrapping (boundary) | `(10,10) % (10,10) = (0,0)` |
| Modulo wrapping (zéro) | `(0,0) % (10,10) = (0,0)` |
| Modulo wrapping (grand négatif) | `(-25,-37) % (10,10) = (5,3)` |
| Equality / Inequality | Opérateurs `==` et `!=` |
| Float operations | Fonctionne aussi avec `Vector2<float>` |
| Multiply by zero | `v * 0 = (0,0)` |

**Pourquoi ?** L'opérateur `%` implémente la formule `((x % m) + m) % m` pour gérer les coordonnées négatives. C'est la base du wrapping toroïdal de la map : si ce calcul est faux, les drones se téléportent au mauvais endroit ou provoquent des accès hors-limites.

### 2. Tile — `tests/test_tile.cpp` (8 tests)

Teste les cases de la map et leur gestion des ressources.

| Test | Ce qu'il vérifie |
|------|-----------------|
| Default constructor | Toutes les ressources initialisées à 0 |
| operator[] access | Accès et modification par type de ressource (`FOOD`, `THYSTAME`, etc.) |
| operator+= | Ajout de ressources |
| operator+= accumulates | Les ajouts s'accumulent correctement |
| operator-= success | Soustraction quand les ressources sont suffisantes |
| operator-= failure | Refus quand une ressource est insuffisante — rien n'est modifié (all-or-nothing) |
| operator-= equal amounts | Soustraction de la totalité des ressources (le code utilise `<`, donc ça réussit) |
| operator-= zero subtraction | Soustraire zéro fonctionne |

**Pourquoi ?** L'opérateur `-=` a une sémantique transactionnelle : il vérifie d'abord que TOUTES les ressources suffisent avant d'en retirer. Si cette logique est cassée, un joueur pourrait consommer des ressources inexistantes.

### 3. World — `tests/test_world.cpp` (9 tests)

Teste la map 2D toroïdale, le wrapping des coordonnées et la gestion des équipes.

| Test | Ce qu'il vérifie |
|------|-----------------|
| Constructor size | La taille de la map correspond aux paramètres |
| operator[] positive wrapping | `(12,13)` wrappe à `(2,3)` sur une map 10x10 |
| operator[] negative wrapping | `(-1,-1)` wrappe à `(9,9)` |
| operator[] large negative | Wrapping avec de grandes valeurs négatives |
| operator[] zero | `(0,0)` et `(10,10)` pointent sur la même tile |
| createTeam / getTeam | Création et recherche d'équipes |
| getTeamNames | Récupération de tous les noms d'équipes |
| tick | Le compteur de ticks s'incrémente |
| spawnResources | Des ressources existent sur la map après construction |

**Pourquoi ?** Le wrapping des coordonnées est utilisé à chaque accès à une tile. Un bug ici causerait des segfaults ou des accès hors-limites mémoire.

### 4. Drone — `tests/test_drone.cpp` (16 tests)

Teste les entités joueurs : déplacement, rotation, vision et incantation.

| Test | Ce qu'il vérifie |
|------|-----------------|
| Direction ++ cycles | Cyclage 1 → 2 → ... → 8 → 1 |
| Direction ++ wraps | Wrap de 8 (WEST) à 1 (NORTH_WEST) |
| Direction ++ on NONE | NONE reste NONE |
| Direction -- cycles | Cyclage inverse |
| Direction -- wraps | Wrap de 1 à 8 |
| Direction -- on NONE | NONE reste NONE |
| Direction full cycle ++ | 8 incréments reviennent au départ |
| Direction full cycle -- | 8 décréments reviennent au départ |
| Unique IDs | Chaque drone a un ID unique |
| Move wrapping | La position reste dans les bornes après 50 déplacements |
| RotateR then RotateL | Tourner à droite puis à gauche revient à la direction initiale |
| RotateR x4 = 360° | 4 rotations droites font un tour complet |
| Initial level | Le niveau initial est 1 |
| startIncantation fails | L'incantation échoue sans ressources sur la tile |
| look tile count | `look()` retourne le bon nombre de tiles au level 1 (4 tiles) |
| look tiles not null | Aucune tile retournée n'est null |

**Pourquoi ?** La mécanique de direction avec le cyclage 1-8 et les rotations par pas de 2 est la source de bugs la plus fréquente. Le cône de vision (`look`) dépend du niveau et de la direction — c'est l'une des fonctions les plus critiques du jeu.

### 5. Team — `tests/test_team.cpp` (5 tests)

Teste la gestion des slots de connexion et le spawn des drones via les oeufs.

| Test | Ce qu'il vérifie |
|------|-----------------|
| Initial slots | Le nombre de slots correspond au constructeur |
| createDrone decrements slots | Chaque spawn réduit les slots |
| createDrone returns nullptr | Retour null quand il n'y a plus d'oeufs |
| createDrone unique IDs | Les drones créés ont des IDs uniques |
| add_egg increases spawns | Ajouter un oeuf permet de spawner un nouveau drone |

**Pourquoi ?** La gestion des slots et des oeufs conditionne la connexion des joueurs IA. Un bug ici empêcherait des joueurs de rejoindre la partie ou permettrait de dépasser la limite de connexions.

### 6. AI Protocol — `tests/test_ai_protocol.cpp` (10 tests)

Teste les fonctions utilitaires du protocole de communication serveur ↔ agents IA.

| Test | Ce qu'il vérifie |
|------|-----------------|
| resource_name | Les 7 noms de ressources et le retour vide pour un index invalide |
| resource_index | Conversion nom → index pour les 7 ressources |
| resource_index invalid | Retour -1 pour des noms invalides (y compris la casse : `"Food"` ≠ `"food"`) |
| resource roundtrip | `resource_index(resource_name(i)) == i` pour tout i |
| ai_cmd_cost known | Coûts spéciaux : Inventory=1, Fork=42, Incantation=300 |
| ai_cmd_cost default | Les autres commandes (Forward, Right, Left, Look) coûtent 7 |
| build_inventory_response | Format de la réponse inventaire : `[food 10, linemate 0, ...]` |
| build_inventory_response zeros | Format avec un inventaire vide |
| build_look_response empty | Format avec des tiles vides (pas de drones = pas de "player") |
| build_look_response resources | Format avec des ressources sur les tiles |

**Pourquoi ?** Le protocole est l'interface entre le serveur et les IA. Si le format des réponses change ou si les fonctions de conversion ne reconnaissent pas un nom de ressource, les agents IA ne fonctionnent plus.

### 7. Client — `tests/test_client.cpp` (11 tests)

Teste le buffering réseau et le parsing des lignes du protocole TCP.

| Test | Ce qu'il vérifie |
|------|-----------------|
| next_line no newline | Retour `nullopt` quand le buffer ne contient pas de `\n` |
| next_line extracts lines | Extraction correcte de plusieurs lignes consécutives |
| next_line strips \r\n | Le `\r` est bien retiré (compatibilité Windows/telnet) |
| next_line empty line | Une ligne vide (`\n` seul) retourne une string vide |
| next_line leaves remainder | Les données partielles restent dans le buffer |
| send appends newline | `send()` ajoute `\n` automatiquement |
| send no double newline | Pas de double `\n` si déjà présent |
| send accumulates | Plusieurs envois s'accumulent dans le write buffer |
| has_pending_write | Détection de données en attente d'écriture |
| type checks | `is_ai()` et `is_gui()` fonctionnent selon le type |
| drain buffer | Plusieurs `next_line()` vident correctement le buffer |

**Pourquoi ?** Le parsing de lignes est la fondation de toute la communication réseau. Un bug dans `next_line()` (mauvaise gestion du `\r\n`, ligne partielle perdue) casserait l'ensemble du protocole serveur.

### 8. Server Args — `tests/test_server_args.cpp` (7 tests)

Teste le parsing des arguments de lancement du serveur.

| Test | Ce qu'il vérifie |
|------|-----------------|
| All required flags | Le parsing fonctionne avec tous les flags requis |
| With frequency | Le flag optionnel `-f` est reconnu |
| Missing required flag | Exception quand `-c` manque |
| Duplicate flag | Exception quand un flag est en double |
| Unknown flag | Exception pour un flag inconnu |
| Help flag | `-h` ne lance pas d'exception |
| Long flags | `--port`, `--width`, `--height`, `--names`, `--clients`, `--freq` fonctionnent |

**Pourquoi ?** Un mauvais parsing des arguments peut lancer le serveur avec des valeurs incorrectes (mauvais port, mauvaise taille de map) sans que l'utilisateur s'en rende compte.

### 9. GUI Protocol — `tests/gui_protocol_tests.cpp` (8 scénarios)

Tests d'intégration qui simulent des échanges complets du protocole graphique. Utilise un framework custom (pas Catch2) car ces tests vérifient les interactions entre `GuiProtocolHandler` et `World` sans dépendre de raylib.

| Scénario | Ce qu'il vérifie |
|----------|-----------------|
| Map initialization and tile sync | `msz` initialise le monde, `bct` met à jour les ressources des tiles |
| New player, team, duplicates | `pnw` crée un joueur, gère les doublons, associe la team |
| Position, orientation, unknown ppo | `ppo` met à jour la position, gère les joueurs inconnus |
| Level, inventory, resource events | `plv`/`pin`/`pgt`/`pdr` — gestion du niveau, inventaire, prise et dépôt de ressources |
| Broadcast, death, unknown ids | `pbc` (broadcast), `pdi` (mort), tolérance aux IDs inconnus |
| Incantation events | `pic`/`pie` — début/fin d'incantation, synchronisation des participants |
| Egg and parent relationship | `enw`/`ebo`/`edi` — ponte, éclosion et mort des oeufs |
| Malformed messages are safe | Les messages malformés sont rejetés sans corrompre l'état du jeu |

**Pourquoi ?** Le protocole GUI est l'interface entre le serveur et le client graphique. Ces tests vérifient que chaque commande du protocole met correctement à jour l'état du monde et que les messages invalides ne crashent pas le client.
