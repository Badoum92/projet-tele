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
#define NB_CLUSTERS    8

struct image
{
    unsigned width;
    unsigned height;
    unsigned channels;
    unsigned size;
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
    unsigned cluster; // cluster of the vector (0 <= cluster < NB_CLUSTER)
    unsigned dist; // distance to centroids[cluster]
};

unsigned dist(int* a, int* b)
{
    int diff[5];
    for (unsigned i = 0; i < 5; i++)
    {
        diff[i] = a[i] - b[i];
    }
#define _2(X) X* X
    unsigned dist =
        _2(diff[0]) + _2(diff[1]) + _2(diff[2]) + _2(diff[3]) + _2(diff[4]);
#undef _2
    return dist;
}

// Re-compute the mass center of each pixels
bool reassign_values(struct image* img, struct vector* vectors,
                     int* mass_centers)
{
    bool ret = 0;
    for (unsigned i = 0; i < img->size; ++i)
    {
        struct vector* v = vectors + i;

        unsigned old_cluster = v->cluster;
        v->cluster = 0;

        v->dist = dist(v->components, mass_centers);
        for (unsigned j = 0; j < NB_CLUSTERS; ++j)
        {
            unsigned d = dist(v->components, mass_centers + (5 * j));
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

void insert_sort(int* components, int value)
{
    components[4] = value;
    for (int i = 3; i >= 0; i--)
    {
        if (components[i] > components[i + 1])
        {
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

            insert_sort(vectors[i].components,
                        img->pixels[img->channels * (y * img->width + x)]);

            // add left neighbour
            if (x > 0)
            {
                insert_sort(
                    vectors[i].components,
                    img->pixels[img->channels * (y * img->width + (x - 1))]);
            }

            // add up neighbour
            if (y > 0)
            {
                insert_sort(
                    vectors[i].components,
                    img->pixels[img->channels * ((y - 1) * img->width + x)]);
            }

            // add right neighbour
            if (x < img->width - 1)
            {
                insert_sort(
                    vectors[i].components,
                    img->pixels[img->channels * (y * img->width + (x + 1))]);
            }

            // add up neighbour
            if (y < img->height - 1)
            {
                insert_sort(
                    vectors[i].components,
                    img->pixels[img->channels * ((y + 1) * img->width + x)]);
            }
        }
    }

    // Initialize mass centers equally spaced on the homogeneity axis
    unsigned min = 0;
    unsigned step = (255 - min) / (NB_CLUSTERS);
    for (unsigned i_cluster = 0; i_cluster < NB_CLUSTERS; i_cluster++)
    {
        unsigned value = 255 - i_cluster * step;
        for (unsigned i = 0; i < 5; i++)
        {
            mass_centers[i_cluster * 5 + i] = value;
        }
    }

    reassign_values(img, vectors, mass_centers);
}

int unsigned_comp(const void* a, const void* b)
{
    return *(const int*)(a) - *(const int*)(b);
}

bool recompute_centroids(struct image* img, struct vector* vectors, int* mass_centers)
{
    int nb_points_per_cluster[NB_CLUSTERS] = {0};
    int new_mass_centers[NB_CLUSTERS * 5] = {0};

    // sum all vectors components in their mass center
    for (unsigned i_vector = 0; i_vector < img->size; i_vector++)
    {
        struct vector* v = vectors + i_vector;

        int *nb_points = nb_points_per_cluster + v->cluster;
        int *new_mass_center = new_mass_centers + 5 * v->cluster;

        for (unsigned i = 0; i < 5; i++)
        {
            new_mass_center[i] += v->components[i];
        }

        *nb_points = *nb_points + 1;
    }

    // Divide each mass_center by its nb of points to calculate the mean
    for (unsigned i_cluster = 0; i_cluster < NB_CLUSTERS; ++i_cluster)
    {
        int nb_points = nb_points_per_cluster[i_cluster];
        if (nb_points)
        {
            for (unsigned i = 0; i < 5; i++)
            {
                new_mass_centers[i_cluster * 5 + i] = new_mass_centers[i_cluster * 5 + i] / nb_points;
            }
        }
    }

    // CLOUDS_CLUSTER is always homogeneous
    int *clouds_mass_center = new_mass_centers + CLOUDS_CLUSTER * 5;
    qsort(clouds_mass_center, 5, sizeof(int), unsigned_comp);
    unsigned median = clouds_mass_center[2];
    for (unsigned i = 0; i < 5; i++) {
        clouds_mass_center[i] = median;
    }

    bool has_changed = memcmp(mass_centers, new_mass_centers, NB_CLUSTERS * 5 * sizeof(int)) != 0;

    if (has_changed)
    {
        memcpy(mass_centers, new_mass_centers, NB_CLUSTERS * 5 * sizeof(int));
    }

    return has_changed;
}

void print_centers(int* mass_centers)
{
    for (unsigned i_cluster = 0; i_cluster < NB_CLUSTERS; ++i_cluster)
    {
        printf("center %u: ", i_cluster);

        for (unsigned i = 0; i < 5; i++)
        {
            printf("%4d ", mass_centers[i_cluster * 5 + i]);
        }

        printf("\n");
    }
}

void kmeans(struct image* img)
{
    struct vector* vectors = calloc(img->size, sizeof(struct vector));
    int mass_centers[NB_CLUSTERS * 5];

    init_kmeans(img, vectors, mass_centers);

    while (1)
    {
        if (!recompute_centroids(img, vectors, mass_centers))
            break;

        if (!reassign_values(img, vectors, mass_centers))
            break;
    }

    unsigned pixels_per_cluster[NB_CLUSTERS] = {0};
    for (unsigned i = 0; i < img->size; i++)
    {
        struct vector* v = vectors + i;
        pixels_per_cluster[v->cluster]++;
    }

#if 0
    for (unsigned i = 0; i < NB_CLUSTERS; i++)
    {
        printf("Pixels in cluster %u: %u.\n", i, pixels_per_cluster[i]);
    }
#endif

    unsigned percent_cloud = ((float)*pixels_per_cluster / img->size) * 100;
    printf("%d%% cloud\n", percent_cloud);

    // Update the img to show every cluster
    unsigned cluster_size = 255 / NB_CLUSTERS;
    for (unsigned i = 0; i < img->size; i++)
    {
        if (vectors[i].cluster == CLOUDS_CLUSTER)
        {
            img->pixels[3 * i] = 255;
            img->pixels[3 * i + 1] = 0;
            img->pixels[3 * i + 2] = 0;
        }
        else
        {
            guchar val = 255 - vectors[i].cluster * cluster_size;
            img->pixels[3 * i] = val;
            img->pixels[3 * i + 1] = val;
            img->pixels[3 * i + 2] = val;
        }
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
        .size = width * height,
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
