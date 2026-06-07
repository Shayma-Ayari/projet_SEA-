#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define NB_IMAGES_TOTAL 40

typedef struct {
    int largeur;
    int hauteur;
    int max_val;
    unsigned char *pixels;
} Image;

typedef struct {
    int prochaine_image;
    int total_images;
    int images_traitees;
    double temps_calcul_total;
} DonneesPartagees;

typedef struct {
    DonneesPartagees *donnees;
    sem_t *verrou;
} TacheThread;

Image* lire_image(const char *chemin) {
    FILE *f = fopen(chemin, "r");
    if (!f) { perror("Erreur ouverture"); return NULL; }
    Image *img = malloc(sizeof(Image));
    char format[3];
    fscanf(f, "%s", format);
    if (strcmp(format, "P3") != 0) {
        printf("Format non supporte\n");
        fclose(f); free(img); return NULL;
    }
    fscanf(f, "%d %d", &img->largeur, &img->hauteur);
    fscanf(f, "%d", &img->max_val);
    int nb = img->largeur * img->hauteur * 3;
    img->pixels = malloc(nb * sizeof(unsigned char));
    for (int i = 0; i < nb; i++) {
        int val; fscanf(f, "%d", &val);
        img->pixels[i] = (unsigned char)val;
    }
    fclose(f);
    return img;
}

void appliquer_flou(Image *img) {
    int L = img->largeur;
    int H = img->hauteur;
    int nb = L * H * 3;
    unsigned char *copie = malloc(nb);
    for (int i = 0; i < nb; i++) copie[i] = img->pixels[i];

    int rayon = 10;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < L; x++) {
            int somme_r = 0, somme_g = 0, somme_b = 0, compte = 0;
            for (int dy = -rayon; dy <= rayon; dy++) {
                for (int dx = -rayon; dx <= rayon; dx++) {
                    int nx = x + dx, ny = y + dy;
                    if (nx >= 0 && nx < L && ny >= 0 && ny < H) {
                        int idx = (ny * L + nx) * 3;
                        somme_r += copie[idx];
                        somme_g += copie[idx + 1];
                        somme_b += copie[idx + 2];
                        compte++;
                    }
                }
            }
            int idx = (y * L + x) * 3;
            img->pixels[idx]     = somme_r / compte;
            img->pixels[idx + 1] = somme_g / compte;
            img->pixels[idx + 2] = somme_b / compte;
        }
    }
    free(copie);
}

void sauvegarder_image(Image *img, const char *chemin) {
    FILE *f = fopen(chemin, "w");
    if (!f) { perror("Erreur sauvegarde"); return; }
    fprintf(f, "P3\n%d %d\n%d\n", img->largeur, img->hauteur, img->max_val);
    int nb_pixels = img->largeur * img->hauteur;
    for (int i = 0; i < nb_pixels; i++) {
        fprintf(f, "%d %d %d\n",
            img->pixels[i*3], img->pixels[i*3+1], img->pixels[i*3+2]);
    }
    fclose(f);
}

void liberer_image(Image *img) {
    free(img->pixels);
    free(img);
}

void* tache_thread(void *arg) {
    TacheThread *tache = (TacheThread*)arg;

    while (1) {
        sem_wait(tache->verrou);
        int numero = tache->donnees->prochaine_image;
        if (numero >= tache->donnees->total_images) {
            sem_post(tache->verrou);
            break;
        }
        tache->donnees->prochaine_image++;
        sem_post(tache->verrou);

        char entree[80], sortie[80];
        sprintf(entree, "images/image%d.ppm", numero + 1);
        sprintf(sortie, "resultats/gris%d.ppm", numero + 1);

        Image *img = lire_image(entree);
        if (!img) continue;

        struct timespec t1, t2;
        clock_gettime(CLOCK_MONOTONIC, &t1);
        appliquer_flou(img);
        clock_gettime(CLOCK_MONOTONIC, &t2);
        double duree = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) / 1e9;

        sauvegarder_image(img, sortie);
        liberer_image(img);

        sem_wait(tache->verrou);
        tache->donnees->images_traitees++;
        tache->donnees->temps_calcul_total += duree;
        sem_post(tache->verrou);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int nb_processus = 2;
    int nb_threads = 1;
    if (argc == 3) {
        nb_processus = atoi(argv[1]);
        nb_threads = atoi(argv[2]);
    }

    int nb_workers = nb_processus * nb_threads;
    printf("Configuration : %d processus x %d threads = %d workers, %d images au total\n",
           nb_processus, nb_threads, nb_workers, NB_IMAGES_TOTAL);

    key_t cle = ftok(".", 'A');
    int shmid = shmget(cle, sizeof(DonneesPartagees), IPC_CREAT | 0666);
    if (shmid == -1) { perror("Erreur shmget"); return 1; }
    DonneesPartagees *donnees = (DonneesPartagees*)shmat(shmid, NULL, 0);
    donnees->prochaine_image = 0;
    donnees->total_images = NB_IMAGES_TOTAL;
    donnees->images_traitees = 0;
    donnees->temps_calcul_total = 0.0;

    sem_unlink("/sem_compteur");
    sem_t *verrou = sem_open("/sem_compteur", O_CREAT, 0666, 1);
    if (verrou == SEM_FAILED) { perror("Erreur sem_open"); return 1; }

    struct timespec debut, fin;
    clock_gettime(CLOCK_MONOTONIC, &debut);

    for (int p = 0; p < nb_processus; p++) {
        pid_t pid = fork();
        if (pid == 0) {
            pthread_t threads[100];
            TacheThread taches[100];
            for (int t = 0; t < nb_threads; t++) {
                taches[t].donnees = donnees;
                taches[t].verrou = verrou;
                pthread_create(&threads[t], NULL, tache_thread, &taches[t]);
            }
            for (int t = 0; t < nb_threads; t++) {
                pthread_join(threads[t], NULL);
            }
            shmdt(donnees);
            exit(0);
        }
    }

    for (int p = 0; p < nb_processus; p++) {
        wait(NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &fin);
    double temps_mur = (fin.tv_sec - debut.tv_sec) + (fin.tv_nsec - debut.tv_nsec) / 1e9;

    int n = donnees->images_traitees;
    double calcul_par_image = (n > 0) ? donnees->temps_calcul_total / n : 0;

    printf("Images traitees      : %d\n", n);
    printf("Temps mur (total)    : %.3f s\n", temps_mur);
    printf("Calcul cumule        : %.3f s\n", donnees->temps_calcul_total);
    printf(">>> Calcul par image : %.3f s\n", calcul_par_image);

    shmdt(donnees);
    shmctl(shmid, IPC_RMID, NULL);
    sem_close(verrou);
    sem_unlink("/sem_compteur");

    return 0;
}
