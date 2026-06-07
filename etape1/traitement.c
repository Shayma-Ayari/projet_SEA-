#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int largeur;
    int hauteur;
    int max_val;
    unsigned char *pixels;
} Image;

Image* lire_image(const char *chemin) {
    FILE *f = fopen(chemin, "r");
    if (!f) {
        perror("Erreur ouverture fichier");
        return NULL;
    }

    Image *img = malloc(sizeof(Image));
    char format[3];

    fscanf(f, "%s", format);
    if (strcmp(format, "P3") != 0) {
        printf("Erreur : format non supporte (attendu P3)\n");
        fclose(f);
        free(img);
        return NULL;
    }

    fscanf(f, "%d %d", &img->largeur, &img->hauteur);
    fscanf(f, "%d", &img->max_val);

    int nb_valeurs = img->largeur * img->hauteur * 3;
    img->pixels = malloc(nb_valeurs * sizeof(unsigned char));

    for (int i = 0; i < nb_valeurs; i++) {
        int val;
        fscanf(f, "%d", &val);
        img->pixels[i] = (unsigned char)val;
    }

    fclose(f);
    printf("Image lue : %d x %d pixels\n", img->largeur, img->hauteur);
    return img;
}

void convertir_gris(Image *img) {
    int nb_pixels = img->largeur * img->hauteur;

    for (int i = 0; i < nb_pixels; i++) {
        unsigned char r = img->pixels[i * 3];
        unsigned char g = img->pixels[i * 3 + 1];
        unsigned char b = img->pixels[i * 3 + 2];

        unsigned char gris = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);

        img->pixels[i * 3]     = gris;
        img->pixels[i * 3 + 1] = gris;
        img->pixels[i * 3 + 2] = gris;
    }

    printf("Conversion en niveaux de gris terminee\n");
}

void sauvegarder_image(Image *img, const char *chemin) {
    FILE *f = fopen(chemin, "w");
    if (!f) {
        perror("Erreur sauvegarde fichier");
        return;
    }

    fprintf(f, "P3\n");
    fprintf(f, "%d %d\n", img->largeur, img->hauteur);
    fprintf(f, "%d\n", img->max_val);

    int nb_pixels = img->largeur * img->hauteur;
    for (int i = 0; i < nb_pixels; i++) {
        fprintf(f, "%d %d %d\n",
            img->pixels[i * 3],
            img->pixels[i * 3 + 1],
            img->pixels[i * 3 + 2]);
    }

    fclose(f);
    printf("Image sauvegardee : %s\n", chemin);
}

void liberer_image(Image *img) {
    free(img->pixels);
    free(img);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage : %s image_entree.ppm image_sortie.ppm\n", argv[0]);
        return 1;
    }

    printf("=== Traitement de %s ===\n", argv[1]);

    Image *img = lire_image(argv[1]);
    if (!img) return 1;

    convertir_gris(img);
    sauvegarder_image(img, argv[2]);
    liberer_image(img);

    printf("=== Traitement termine ===\n");
    return 0;
}
