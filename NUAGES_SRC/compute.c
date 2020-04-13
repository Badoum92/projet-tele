#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>

/*******************************************************
IL EST FORMELLEMENT INTERDIT DE CHANGER LE PROTOTYPE
DES FONCTIONS
*******************************************************/

#define CLOUDS_CLUSTER 0
#define NB_CLUSTERS 9

struct image
{
    unsigned width;
    unsigned height;
    unsigned channels;
    unsigned length;
    guchar* pixels;
};

static struct timespec now()
{
    struct timespec tp;
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp;
}

static double elapsed_time(struct timespec* begin, struct timespec* end)
{
    double s = difftime(end->tv_sec, begin->tv_sec);
    long ns = end->tv_nsec - begin->tv_nsec;
    return s + ((double)ns) / 1.0e9;
}

struct vector
{
    int components[5]; // radiometry of a pixel and its 4-connexity neighbours
    unsigned cluster;       // cluster of the vector (0 <= cluster < NB_CLUSTER)
    unsigned dist;          // distance to centroids[cluster]
};

unsigned dist(int *a, int *b)
{
    int diff[5];
    for (unsigned i = 0; i < 5; i ++) {
        diff[i] = a[i] - b[i];
    }
#define _2(X) X * X
    unsigned dist = _2(diff[0]) + _2(diff[1]) + _2(diff[2]) + _2(diff[3]) + _2(diff[4]);
#undef _2
    return dist;
}

// Re-compute the mass center of each pixels
bool reassign_values(struct image* img, struct vector* vectors, int* mass_centers)
{
    bool ret = 0;
    for (unsigned i = 0; i < img->width * img->height; ++i)
    {
        struct vector *v = vectors + i;

        unsigned old_cluster = v->cluster;
        v->cluster = 0;

        v->dist = dist(v->components, mass_centers);
        for (unsigned j = 1; j < NB_CLUSTERS; ++j)
        {
            unsigned d = dist(v->components, mass_centers + (5*j));
            if (d < v->dist)
            {
                v->cluster = j;
                v->dist = d;
            }
        }

        ret = ret || v->cluster != old_cluster;
    }
    return ret;
}

void insert_sort(int *components, int value)
{
    components[4] = value;
    for (int i = 3; i > 0; i--) {
        if (components[i] > components[i + 1]) {
            break;
        }
        unsigned tmp = components[i];
        components[i] = components[i + 1];
        components[i + 1] = tmp;
    }
}

// Init the vectors and place the mass centers
void init_kmeans(struct image* img, struct vector* vectors, int* mass_centers)
{
    // Init vectors from neighbours
    for (unsigned y = 0; y < img->height; y++)
    {
        for (unsigned x = 0; x < img->width; x++)
        {
            // each vector is already 0 because of calloc

            unsigned i = y * img->width + x;

            // add left neighbour
            if (x > 0) {
                insert_sort(vectors[i].components, img->pixels[y * img->width + (x - 1)]);
            }

            // add up neighbour
            if (y > 0) {
                insert_sort(vectors[i].components, img->pixels[(y - 1) * img->width + x]);
            }

            // add right neighbour
            if (x < img->width - 1) {
                insert_sort(vectors[i].components, img->pixels[y * img->width + (x + 1)]);
            }

            // add up neighbour
            if (y < img->height - 1) {
                insert_sort(vectors[i].components, img->pixels[(y + 1) * img->width + x]);
            }
        }
    }

    // Initialize mass centers
    unsigned step = 255 / NB_CLUSTERS;
    for (unsigned i_cluster = 1; i_cluster < NB_CLUSTERS; i_cluster++)
    {
        unsigned value = 255 - i_cluster * step;
        for (unsigned i = 0; i < 5; i++) {
            mass_centers[i_cluster * 5 + i] = value;
        }
    }

    reassign_values(img, vectors, mass_centers);
}

void compute_centroid_n(int *mean, struct image* img, struct vector *vectors, unsigned i_cluster)
{
    unsigned nb_points = 0;

    // TODO: median for cluser 0

    for (unsigned i_vector = 0; i_vector < img->width * img->height; i_vector++)
    {
        struct vector *v = vectors + i_vector;

        if (v->cluster != i_cluster) {
            continue;
        }

        for (unsigned i = 0; i < 5; i++) {
            mean[i] += v->components[i];
        }

        nb_points++;
    }

    if (nb_points) {
        for (unsigned i = 0; i < 5; i++) {
            mean[i] = mean[i] / nb_points;
        }
    }
}

bool recompute_centroids(struct image* img, struct vector* vectors, int* mass_centers)
{
    bool ret = false;

    for (unsigned i_cluster = 0; i_cluster < NB_CLUSTERS; ++i_cluster)
    {
        int mean[5];
        compute_centroid_n(mean, img, vectors, i_cluster);
        ret = ret || memcmp(mass_centers + 5 * i_cluster, mean, 5 * sizeof(int)) != 0;

        memcpy(&mass_centers[i_cluster], mean, 5 * sizeof(int));
    }

    return ret;
}

void kmeans(struct image* img)
{
    struct vector *vectors = calloc(img->width * img->height, sizeof(struct vector));
    int mass_centers[NB_CLUSTERS * 5];

    init_kmeans(img, vectors, mass_centers);

    while (1)
    {
        if (!recompute_centroids(img, vectors, mass_centers))
            break;

        if (!reassign_values(img, vectors, mass_centers))
            break;
    }

    unsigned cluster_size = 255 / NB_CLUSTERS;
    for (int i = 0; i < img->width * img->height; i++)
    {
        guchar val = 255 - vectors[i].cluster * cluster_size;
        img->pixels[3*i] = val;
        img->pixels[3*i + 1] = val;
        img->pixels[3*i + 2] = val;
    }

    free(vectors);
}

/*---------------------------------------
  Proto:


  But:

  Entrees:
          --->le tableau des valeurs des pixels de l'image d'origine
          (les lignes sont mises les unes � la suite des autres)
      --->le nombre de lignes de l'image,
      --->le nombre de colonnes de l'image,
          --->le tableau des valeurs des pixels de l'image resultat
          (les lignes sont mises les unes � la suite des autres)


  Sortie:

  Rem:

  Voir aussi:

  ---------------------------------------*/

void ComputeImage(guchar* src, int height, int width, guchar* dst)
{
    // clang-format off
    struct image img = {
        .width = width,
        .height = height,
        .channels = 3,
        .length = width * height * 3,
        .pixels = dst ? dst : src
    };
    // clang-format on

    for (unsigned i = 0; i < img.length; i += img.channels)
    {
        unsigned char mean = (src[i] + src[i + 1] + src[i + 2]) / 3;
        img.pixels[i] = mean;
        img.pixels[i + 1] = mean;
        img.pixels[i + 2] = mean;
    }

    struct timespec begin = now();
    kmeans(&img);
    struct timespec end = now();
    double total = elapsed_time(&begin, &end);
    printf("Execution time: %.4lf ms\n", total * 1000);
}
