#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NB_PHILOSOPHES 5

sem_t fourchettes[NB_PHILOSOPHES];   /* une fourchette = un sémaphore */

void penser(int id) {
    printf("Philosophe %d pense...\n", id);
    usleep(100000);   /* pause 0.1s */
}

void manger(int id) {
    printf("Philosophe %d MANGE\n", id);
    usleep(100000);
}

void* philosophe(void *arg) {
    int id = *(int*)arg;
    int gauche = id;                       /* fourchette de gauche */
    int droite = (id + 1) % NB_PHILOSOPHES; /* fourchette de droite */

    for (int repas = 0; repas < 3; repas++) {   /* chacun mange 3 fois */
        penser(id);

        /* ASTUCE anti-deadlock : le dernier philosophe prend dans l'ordre inverse */
        if (id == NB_PHILOSOPHES - 1) {
            sem_wait(&fourchettes[droite]);   /* droite d'abord */
            sem_wait(&fourchettes[gauche]);
        } else {
            sem_wait(&fourchettes[gauche]);   /* gauche d'abord */
            sem_wait(&fourchettes[droite]);
        }

        /* Il a les deux fourchettes : il mange */
        manger(id);

        /* Il repose les deux fourchettes */
        sem_post(&fourchettes[gauche]);
        sem_post(&fourchettes[droite]);
    }

    printf("Philosophe %d a fini de manger\n", id);
    return NULL;
}

int main() {
    pthread_t threads[NB_PHILOSOPHES];
    int ids[NB_PHILOSOPHES];

    /* Initialiser chaque fourchette à 1 (disponible) */
    for (int i = 0; i < NB_PHILOSOPHES; i++) {
        sem_init(&fourchettes[i], 0, 1);
    }

    /* Créer un thread par philosophe */
    for (int i = 0; i < NB_PHILOSOPHES; i++) {
        ids[i] = i;
        pthread_create(&threads[i], NULL, philosophe, &ids[i]);
    }

    /* Attendre que tous finissent */
    for (int i = 0; i < NB_PHILOSOPHES; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Détruire les fourchettes */
    for (int i = 0; i < NB_PHILOSOPHES; i++) {
        sem_destroy(&fourchettes[i]);
    }

    printf("Tous les philosophes ont termine. Aucun deadlock.\n");
    return 0;
}
