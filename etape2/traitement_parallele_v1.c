#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* fork() */
#include <sys/wait.h>   /* wait() */

typedef struct {
    int largeur;
    int hauteur;
    int max_val;
    unsigned char *pixels;
} Image;

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

/* Fonction qui traite UNE image complète */
void traiter_une_image(const char *entree, const char *sortie) {
    Image *img = lire_image(entree);
    if (!img) return;
    convertir_gris(img);
    sauvegarder_image(img, sortie);
    liberer_image(img);
}

int main() {
    /* Liste des images à traiter */
    const char *images[] = {"image1.ppm", "image2.ppm", "image3.ppm"};
    const char *sorties[] = {"gris1.ppm", "gris2.ppm", "gris3.ppm"};
    int nb_images = 3;

    printf("Le parent va creer %d enfants\n", nb_images);

    /* Pour chaque image, créer un enfant */
    for (int i = 0; i < nb_images; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            /* Code de l'ENFANT : il traite SON image puis s'arrête */
            printf("  Enfant %d traite %s\n", i+1, images[i]);
            traiter_une_image(images[i], sorties[i]);
            exit(0);   /* TRÈS IMPORTANT : l'enfant s'arrête ici */
        }
        /* Le parent continue la boucle pour créer le prochain enfant */
    }

    /* Le parent attend que TOUS les enfants finissent */
    for (int i = 0; i < nb_images; i++) {
        wait(NULL);
    }

    printf("Tous les enfants ont fini. Parent termine.\n");
    return 0;
}
int main(int argc, char *argv[]) {
    int nb_images = 3;
    if (argc == 2) {
        nb_images = atoi(argv[1]);
    }

    printf("Le parent va creer %d enfants\n", nb_images);

    for (int i = 0; i < nb_images; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            char entree[50], sortie[50];
            sprintf(entree, "image%d.ppm", i+1);
            sprintf(sortie, "gris%d.ppm", i+1);

            printf("  Enfant %d traite %s\n", i+1, entree);
            traiter_une_image(entree, sortie);
            exit(0);
        }
    }

    for (int i = 0; i < nb_images; i++) {
        wait(NULL);
    }

    printf("Tous les enfants ont fini. Parent termine.\n");
    return 0;
}
