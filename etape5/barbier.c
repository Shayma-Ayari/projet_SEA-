#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#define NB_CHAISES 3      /* chaises dans la salle d'attente */
#define NB_CLIENTS 8      /* nombre total de clients qui vont venir */

sem_t clients;        /* nombre de clients en attente */
sem_t barbier;        /* le barbier est-il prêt ? */
sem_t acces_salle;    /* verrou pour modifier le compteur de chaises */

int clients_en_attente = 0;

/* Le thread du BARBIER */
void* le_barbier(void *arg) {
    while (1) {
        sem_wait(&clients);       /* dort tant qu'aucun client (s'endort ici) */

        sem_wait(&acces_salle);   /* verrouille pour modifier le compteur */
        clients_en_attente--;     /* un client quitte la salle d'attente */
        printf("  Barbier : je reveille et je coupe (reste %d en attente)\n",
               clients_en_attente);
        sem_post(&acces_salle);   /* déverrouille */

        sem_post(&barbier);       /* signale : barbier prêt */

        usleep(200000);           /* coupe les cheveux (0.2s) */
        printf("  Barbier : coupe terminee\n");
    }
    return NULL;
}

/* Le thread d'un CLIENT */
void* le_client(void *arg) {
    int id = *(int*)arg;

    sem_wait(&acces_salle);   /* verrouille pour vérifier les chaises */

    if (clients_en_attente < NB_CHAISES) {
        clients_en_attente++;   /* s'assoit sur une chaise */
        printf("Client %d s'assoit (%d en attente)\n", id, clients_en_attente);
        sem_post(&acces_salle);     /* déverrouille */

        sem_post(&clients);   /* signale sa présence (réveille le barbier) */
        sem_wait(&barbier);   /* attend que le barbier soit prêt */

        printf("Client %d se fait couper les cheveux\n", id);
    } else {
        /* Salle pleine : le client repart */
        printf("Client %d : salle pleine, je repars\n", id);
        sem_post(&acces_salle);
    }
    return NULL;
}

int main() {
    pthread_t th_barbier;
    pthread_t th_clients[NB_CLIENTS];
    int ids[NB_CLIENTS];

    /* Initialiser les sémaphores */
    sem_init(&clients, 0, 0);      /* 0 client au début */
    sem_init(&barbier, 0, 0);      /* barbier pas encore prêt */
    sem_init(&acces_salle, 0, 1);  /* verrou libre (1) */

    /* Lancer le barbier */
    pthread_create(&th_barbier, NULL, le_barbier, NULL);

    /* Les clients arrivent un par un, avec un petit délai */
    for (int i = 0; i < NB_CLIENTS; i++) {
        ids[i] = i;
        pthread_create(&th_clients[i], NULL, le_client, &ids[i]);
        usleep(100000);   /* un nouveau client toutes les 0.1s */
    }

    /* Attendre que tous les clients aient été traités */
    for (int i = 0; i < NB_CLIENTS; i++) {
        pthread_join(th_clients[i], NULL);
    }

    printf("Tous les clients sont passes. Fin de journee.\n");
    /* Note : le barbier tourne en boucle infinie, on arrête le programme ici */
    exit(0);
}
