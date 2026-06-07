#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int taille = 800;   /* image 800x800 par défaut */
    if (argc == 2) taille = atoi(argv[1]);

    FILE *f = fopen("grande.ppm", "w");
    fprintf(f, "P3\n%d %d\n255\n", taille, taille);

    /* Générer des pixels colorés (un dégradé) */
    for (int y = 0; y < taille; y++) {
        for (int x = 0; x < taille; x++) {
            int r = (x * 255) / taille;
            int g = (y * 255) / taille;
            int b = 128;
            fprintf(f, "%d %d %d\n", r, g, b);
        }
    }

    fclose(f);
    printf("Image %dx%d generee : grande.ppm\n", taille, taille);
    return 0;
}
