# Broadcast Protocol

> Chaque broadcast commence par un ID spécifique + chiffrement basé sur le nom de la team.

---

## Phase Start

| Code | Signature       | Description         |
|------|-----------------|---------------------|
| `1`  | `[n lvl]`       | Niveau d'un agent   |
| `2`  | `[n]`           | Naissance           |
| `3`  | `[n x y m]`     | Réponse parent      |

### Détails des champs

**`1 [n lvl]` — Niveau**
- `n` : Identifiant de l'agent
- `lvl` : Niveau de l'agent

**`2 [n]` — Né**
- `n` : Identifiant de l'agent

**`3 [n x y m]` — Réponse parent**
- `n` : Identifiant de l'agent
- `x/y` : Position de l'œuf
- `m` : ID du nouveau-né

---

## Phase Endgame

| Code | Signature         | Description          |
|------|-------------------|----------------------|
| `1`  | `[n]`             | Qui est vivant ?     |
| `2`  | `[n t]`           | En vie               |
| `3`  | `[n x y t]`       | Mort                 |
| `4`  | `[n t]`           | Né                   |
| `5`  | `[n x y r q]`     | Demande de ressource |
| `6`  | `[n t x y m]`     | Réponse positive     |
| `7`  | `[n xm ym]`       | Ennemi détecté       |
| `8`  | `[n lvl]`         | Rituel prêt          |

### Détails des champs

**`1 [n]` — Qui est vivant ?**
- `n` : Identifiant de l'agent

**`2 [n t]` — En vie**
- `n` : Identifiant de l'agent
- `t` : Type de l'agent

**`3 [n x y t]` — Mort**
> À envoyer avant la mort volontaire (ou non) d'un agent.
- `n` : Identifiant de l'agent
- `x/y` : Position
- `t` : Type de l'agent

**`4 [n t]` — Né**
- `n` : Identifiant de l'agent
- `t` : Type de l'agent

**`5 [n x y r q]` — Je veux ça**
- `n` : Identifiant de l'agent
- `x/y` : Position
- `r` : Ressource demandée
- `q` : Quantité de ressource

**`6 [n t x y m]` — Réponse positive**
- `n` : Identifiant de l'agent
- `t` : Type de l'agent (détermine la nature du message)
- `x/y` : Position
- `m` : Destinataire

**`7 [n xm ym]` — Ennemi détecté**
- `xm/ym` : Position de l'ennemi

**`8 [n lvl]` — Rituel prêt**
- `lvl` : Niveau ciblé
