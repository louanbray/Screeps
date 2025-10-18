# Compétition d'IA — Screeps Light

L'objectif de ce travail est de construire une stratégie automatisée (souvent appelée *intelligence artificielle* dans le domaine des jeux) pour un jeu compétitif de type **collecte de ressources**.

---

## 1. Présentation du problème

Le jeu se joue sur une **carte carrée** et les matchs classiques se feront entre **4 joueurs**.  
Chaque case de la carte peut être :
- un **mur**,
- un **dépôt**,
- ou quelques **cases spéciales** (qu’on pourra ignorer dans un premier temps).

Les murs ne peuvent pas être franchis : il faut les contourner.

Les unités, appelées **minions**, disposent de trois caractéristiques à paramétrer à leur création :  
- leur **vie** ;  
- leur **attaque** ;  
- leur **charge maximale**.

À chaque tour de jeu, chaque joueur doit indiquer une nouvelle position pour chaque minion dans le voisinage de celui-ci (haut, bas, gauche, droite) ou éventuellement **rester sur place**.

Pendant les 100 premiers tours de jeu, les joueurs sont isolés les uns des autres à l'aide d'un mur en croix situé au centre de la map.

---

### Combats et transferts

- Un déplacement demandé en direction d’un **minion ennemi** entraîne un **combat** :  
  chaque protagoniste perd un nombre de points de vie égal à l’attaque de son adversaire.  
  Si la vie d’un minion tombe à 0, il **meurt**.  
  Si l’attaquant tue son adversaire, il **prend sa place**, sinon il **reste immobile**.

- Un déplacement demandé en direction d’un **minion allié** entraîne un **transfert de charge**  
  (dans la limite de la capacité du receveur), **sans déplacement** effectif.

---

## 2. Fonctionnement de la simulation

Votre programme est exécuté automatiquement à chaque tour de jeu :  

1. Il **lit** l’état du jeu dans un fichier `mapData.txt` ;  
2. Il **écrit** ses ordres dans `answer.txt` ;  
3. Puis il **s’arrête**.

---

### 2.1. Les entrées

Le fichier `mapData.txt` contient plusieurs types de lignes :

- `n` : la largeur de la carte (carrée)  
- les `n` lignes suivantes décrivent les cases, chacune contenant `n` **triplets** séparés par des espaces :  
  ```
  T,N,D
  ```
  où :
  - `T` est le type de case :
    - `R` : ressource  
    - `W` : mur  
    - (d’autres types spéciaux sont décrits dans `special.md`)
  - `N` : nombre de ressources sur la case (entre 0 et 10)  
  - `D` : données supplémentaires (pour les cases spéciales)

- `M` : nombre total de minions sur la carte  
- les `M` lignes suivantes décrivent chaque minion sous la forme :
  ```
  P,X,Y,CAR,HP,SIZE,ATK
  ```
  où :
  - `P` : numéro du joueur propriétaire  
  - `X, Y` : coordonnées du minion  
  - `CAR` : charge actuelle  
  - `HP` : points de vie  
  - `SIZE` : capacité maximale  
  - `ATK` : points d’attaque  

- `ID RSC` : votre numéro de joueur et votre nombre actuel de ressources  
- `SPX SPY` : coordonnées de votre dépôt  
- `TOUR TOURMAX` : tour actuel et tour maximum  
- Des lignes supplémentaires peuvent indiquer des événements :
  ```
  EID,TURN
  ```
  (voir `randomEvents.md`)

---

### 2.2. La sortie

Vous devez écrire dans `answer.txt` une liste d’ordres, généralement **un par minion**, sous la forme :
```
X Y XDEST YDEST
```

Vous pouvez aussi **créer un nouveau minion** (un seul par tour) sur le dépôt via une ligne :
```
CREATE HP SIZE ATK
```

avec :
- `HP` ≥ 1 (sinon le minion meurt immédiatement)  
  → les points de vie effectifs sont `2 × HP`  
- `SIZE` : charge maximale du minion `2 × SIZE`  
- `ATK` : points d’attaque (non multipliés)
- `2 × HP` + `2 × SIZE` + `ATK` ≤ 25 (la somme des caractéristiques ne doit pas dépasser 25) 

**Coût total :**
```
HP + SIZE + ATK
```

---


## 3. Exécuter une simulation

Pour générer une carte :

```bash
python3 generate_map_advanced.py map.txt 10 5 3 3 5 7
```
> Ici, `10` est le demi-côté de la carte.

La disposition des murs varie selon la carte.  
Elle est paramétrable dans le script de génération (mais pas via les paramètres en ligne de commande).

---

Pour lancer une simulation :

```bash
python3 run_simulation.py map.txt 400 0.3 ./bot1 ./bot2 ./bot3 ./bot4
```

où :
- `400` = nombre de tours de la simulation  
- `0.3` = temps entre deux tours (en secondes)  
- `bot1` à `bot4` = programmes des joueurs (dans le même dossier)  

Un même programme peut contrôler plusieurs joueurs.

**Touches utiles pendant la simulation :**
- `+` / `-` : accélérer / ralentir  
- `Espace` : pause / reprise  

---

## 4. Tournoi

Un **tournoi** sera organisé entre les différents programmes.  
Les scores seront mis à jour selon un **classement de type ELO**.

Le **score d’un joueur** est le **nombre total de ressources rapatriées** à son dépôt,  
y compris celles **dépensées par la suite**.