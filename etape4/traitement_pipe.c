#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

typedef struct {
    int largeur;
    int hauteur;
    int max_val;
    unsigned char *pixels;
} Image;

typedef struct {
    int numero_image;
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

void convertir_gris(Image *img) {
    int nb_pixels = img->largeur * img->hauteur;
    for (int i = 0; i < nb_pixels; i++) {
        unsigned char r = img->pixels[i*3];
        unsigned char g = img->pixels[i*3+1];
        unsigned char b = img->pixels[i*3+2];
        unsigned char gris = (unsigned char)(0.299*r + 0.587*g + 0.114*b);
        img->pixels[i*3] = gris;
        img->pixels[i*3+1] = gris;
        img->pixels[i*3+2] = gris;
    }
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
    char entree[50], sortie[50];
    sprintf(entree, "image%d.ppm", tache->numero_image);
    sprintf(sortie, "gris%d.ppm", tache->numero_image);

    Image *img = lire_image(entree);
    if (img) {
        convertir_gris(img);
        sauvegarder_image(img, sortie);
        liberer_image(img);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    int nb_processus = 2;
    int nb_threads = 3;
    if (argc == 3) {
        nb_processus = atoi(argv[1]);
        nb_threads = atoi(argv[2]);
    }

    printf("Configuration : %d processus, %d threads chacun\n",
           nb_processus, nb_threads);

    /* Créer le pipe AVANT le fork */
    int fd[2];
    if (pipe(fd) == -1) { perror("Erreur pipe"); return 1; }

    for (int p = 0; p < nb_processus; p++) {
        pid_t pid = fork();

        if (pid == 0) {
            /* ===== ENFANT ===== */
            close(fd[0]);   /* l'enfant écrit seulement */

            pthread_t threads[100];
            TacheThread taches[100];

            for (int t = 0; t < nb_threads; t++) {
                taches[t].numero_image = p * nb_threads + t + 1;
                pthread_create(&threads[t], NULL, tache_thread, &taches[t]);
            }
            for (int t = 0; t < nb_threads; t++) {
                pthread_join(threads[t], NULL);
            }

            /* Envoyer au parent le nombre d'images traitées */
            int images_traitees = nb_threads;
            write(fd[1], &images_traitees, sizeof(int));
            printf("  Enfant %d a traite %d images (envoye au parent)\n",
                   p+1, images_traitees);

            close(fd[1]);
            exit(0);
        }
    }

    /* ===== PARENT ===== */
    close(fd[1]);   /* le parent lit seulement */

    int total = 0;
    for (int p = 0; p < nb_processus; p++) {
        int recu;
        read(fd[0], &recu, sizeof(int));   /* lit ce qu'un enfant a envoyé */
        total += recu;
    }
    close(fd[0]);

    for (int p = 0; p < nb_processus; p++) {
        wait(NULL);
    }

    printf("Parent : TOTAL = %d images traitees\n", total);
    return 0;
}
