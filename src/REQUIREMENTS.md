# Spécifications techniques — Projet SEA

## Traitement intensif, parallèle & benchmarking

**Cours :** Système d'Exploitation Avancé (3ALINFO7)
**Domaine d'application :** Traitement parallèle d'images (filtre de flou + génération de vignettes)
**Réalisé par :** Shayma Ayari & Ines Jerbi

---

## 1. Description du projet

Ce projet implémente une application de traitement d'images en parallèle. Elle applique un filtre de flou sur un ensemble d'images, puis génère une vignette de chaque image traitée en réutilisant un outil externe existant (ImageMagick). Le travail est réparti sur plusieurs processus et threads, et les performances sont mesurées.

Concepts illustrés :
- création et gestion de processus (multiprocessus)
- programmation multithread
- recouvrement de processus (exec) et réutilisation de logiciels existants
- communication interprocessus (IPC) : pipe, mémoire partagée, file de messages
- synchronisation par sémaphores

---

## 2. Critères couverts

| Critère | Implémentation |
|---|---|
| Multiprocessus configurable | fork() x N (N en argument) |
| Multithreads configurable | pthread x M (M en argument) |
| Recouvrement | fork() + execlp() lancant ImageMagick |
| Chargement solution existante | Reutilisation d'ImageMagick (convert) |
| IPC Pipe | Communication enfant vers parent |
| IPC Memoire partagee | Compteur global (shmget) |
| IPC File de messages | Bilan de chaque processus (msgget/msgsnd/msgrcv) |
| Semaphores | Protection du compteur (sem_open) |
| Philosophes dineurs | Prevention du deadlock |
| Barbier endormi | Producteur/consommateur |
| Benchmarking | Mesure du temps selon le parallelisme |

---

## 3. Prérequis

```bash
sudo apt update
sudo apt install build-essential imagemagick
```

---

## 4. Structure du projet

projet_SEA/
├── README.md
├── REQUIREMENTS.md
├── src/
│   ├── traitement_complet.c
│   ├── benchmark.c
│   ├── philosophes.c
│   ├── barbier.c
│   └── generer_image.c
└── .gitignore

---

## 5. Compilation

```bash
cd src
gcc -o traitement_complet traitement_complet.c -Wall -pthread
gcc -o benchmark benchmark.c -Wall -pthread
gcc -o philosophes philosophes.c -Wall -pthread
gcc -o barbier barbier.c -Wall -pthread
gcc -o generer_image generer_image.c -Wall
```

---

## 6. Utilisation

### 6.1 Préparer les images

```bash
./generer_image 800
mkdir -p images resultats
for i in $(seq 1 40); do cp grande.ppm images/image$i.ppm; done
```

### 6.2 Application principale

```bash
./traitement_complet [nb_processus] [nb_threads]
./traitement_complet 2 3
```

Applique le flou, genere les vignettes via ImageMagick (recouvrement), met a jour le compteur partage (protege par semaphore), et chaque processus envoie son bilan au parent via la file de messages.

### 6.3 Benchmark

```bash
./benchmark 1 1
./benchmark 2 1
./benchmark 4 1
```

### 6.4 Problèmes classiques

```bash
./philosophes
./barbier
```

---

## 7. Architecture technique

1. Le processus parent cree N processus enfants via fork().
2. Chaque enfant cree M threads via pthread_create().
3. Chaque thread lit une image, applique le flou, sauvegarde, puis fait un recouvrement (fork + execlp) pour lancer ImageMagick et generer une vignette.
4. Un compteur global en memoire partagee, protege par semaphore, comptabilise les images.
5. Chaque processus envoie son bilan au parent via une file de messages.

---

## 8. Résultats du benchmarking

Tests sur machine virtuelle a 2 coeurs, pour 40 images.

| Configuration | Images | Temps total | Speedup |
|---|---|---|---|
| 1 processus | 40 | ~100.8 s | 1.0x |
| 2 processus | 40 | ~68.4 s | 1.47x |
| 4 processus | 40 | ~61.1 s | 1.65x |

Observations :
- Le passage de 1 a 2 processus accelere nettement (2 coeurs en parallele).
- Au-dela de 2 processus, le gain ralentit (limite des 2 coeurs).
- Le speedup depend aussi des I/O disque.

Le benchmark utilise une file de travail partagee : les workers piochent dynamiquement la prochaine image, equilibrant la charge.

---

## 9. Filtre appliqué

Filtre de flou (moyenne des pixels voisins). Apres le flou, une vignette est generee par ImageMagick via recouvrement, combinant code maison et outil existant.
