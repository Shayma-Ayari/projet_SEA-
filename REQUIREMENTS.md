# Spécifications techniques — Projet SEA

## Traitement intensif, parallèle & benchmarking

**Cours :** Système d'Exploitation Avancé (3ALINFO7)
**Domaine d'application :** Traitement parallèle d'images (filtre de flou)
**Réalisé par :** Shayma Ayari & Ines Jerbi

---

## 1. Description du projet

Ce projet implémente une application de **traitement d'images en parallèle**. Elle applique un filtre de flou sur un ensemble d'images en répartissant le travail sur plusieurs processus et plusieurs threads, puis mesure les performances obtenues (benchmarking).

Le projet illustre les concepts fondamentaux des systèmes d'exploitation :
- la création et la gestion de processus (multiprocessus)
- la programmation multithread
- la communication interprocessus (IPC)
- la synchronisation par sémaphores

---

## 2. Fonctionnalités

| Fonctionnalité | Description |
|---|---|
| Multiprocessus | Création de N processus configurables via fork() |
| Multithreads | Chaque processus lance M threads configurables via pthread |
| Pipe | Communication enfant vers parent |
| Mémoire partagée | Compteur global partagé entre processus (shmget) |
| Message queue | Échange de messages typés (msgget) |
| Sémaphores | Protection des ressources partagées |
| Philosophes dîneurs | Prévention du deadlock |
| Barbier endormi | Producteur/consommateur |
| Benchmarking | Mesure du temps selon le parallélisme |

---

## 3. Prérequis

- Système Linux (Ubuntu recommandé)
- Compilateur GCC et bibliothèque pthread

```bash
sudo apt update
sudo apt install build-essential
```

---

## 4. Structure du projet
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
for i in $(seq 1 12); do cp grande.ppm images/image$i.ppm; done
```

### 6.2 Application principale

```bash
./traitement_complet [nb_processus] [nb_threads]
./traitement_complet 2 3
```

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

1. Le processus parent crée N processus enfants via fork().
2. Chaque enfant crée M threads via pthread_create().
3. Chaque thread lit une image, applique le flou, sauvegarde le résultat.
4. Un compteur global en mémoire partagée, protégé par sémaphore, comptabilise les images.
5. Les processus communiquent via pipe et message queue.

---

## 8. Résultats du benchmarking

Tests sur machine virtuelle à 2 cœurs.

| Configuration | Images | Temps total | Temps par image |
|---|---|---|---|
| 1 processus | 1 | ~2.76 s | 2.76 s |
| 2 processus | 2 | ~3.44 s | 1.72 s |
| 4 processus | 4 | ~7.12 s | 1.78 s |

Observations :
- Le passage de 1 à 2 processus réduit le temps par image (~1.6x plus rapide).
- Au-delà de 2 processus, le gain stagne (limite des 2 cœurs).
- Le speedup dépend aussi des I/O disque et de la mémoire.

---

## 9. Filtre appliqué

Filtre de flou (moyenne des pixels voisins). Plus le rayon est grand, plus le traitement est intensif.
