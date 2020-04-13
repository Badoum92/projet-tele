#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>

/*******************************************************
IL EST FORMELLEMENT INTERDIT DE CHANGER LE PROTOTYPE
DES FONCTIONS
*******************************************************/

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

struct data
{
    unsigned nb; // number of pixels with a given value
    unsigned cluster; // 0 <= cluster < nb_clusters
    unsigned dist; // distance to centroids[cluster]
};

unsigned dist(int x, int y)
{
    return ABS(x - y);
}

int reassign_values(struct image* img, struct data* hist, guchar* centroids)
{
    int ret = 0;
    for (unsigned i = 0; i < 255; ++i)
    {
        unsigned old = hist[i].cluster;
        hist[i].cluster = 0;
        hist[i].dist = dist(i, centroids[0]);

        for (unsigned j = 1; j < NB_CLUSTERS; ++j)
        {
            unsigned d = dist(i, centroids[j]);
            if (d < hist[i].dist)
            {
                hist[i].cluster = j;
                hist[i].dist = d;
            }
        }

        ret = ret || hist[i].cluster != old;
    }
    return ret;
}

void init_kmeans(struct image* img, struct data* hist, guchar* centroids,
                 guchar* image)
{
    unsigned cluster_size = 255 / NB_CLUSTERS;
    for (unsigned i = 0; i < NB_CLUSTERS; ++i)
    {
        centroids[i] = i * cluster_size;
    }

    for (unsigned i = 0; i < 255; ++i)
    {
        hist[i].nb = 0;
    }

    reassign_values(img, hist, centroids);

    for (int i = 0; i < img->length; i += img->channels)
    {
        guchar val = image[i];
        hist[val].nb++;
    }
}

guchar compute_centroid_n(struct image* img, unsigned n, unsigned* i,
                          struct data* hist)
{
    size_t nb_points = 0;
    size_t sum = 0;

    unsigned cur = *i;
    for (; cur < 255 && hist[cur].cluster == n; cur++)
    {
        nb_points += hist[cur].nb;
        sum += hist[cur].nb * cur;
    }
    *i = cur;

    return nb_points == 0 ? 0 : sum / nb_points;
}

int recompute_centroids(struct image* img, struct data* hist, guchar* centroids)
{
    int ret = 0;
    unsigned i = 0;
    for (unsigned n = 0; n < NB_CLUSTERS; ++n)
    {
        guchar val = compute_centroid_n(img, n, &i, hist);
        ret = ret || centroids[n] != val;
        centroids[n] = val;
    }
    return ret;
}

void kmeans(struct image* img)
{
    struct data hist[255];
    guchar centroids[NB_CLUSTERS];
    init_kmeans(img, hist, centroids, img->pixels);

    while (1)
    {
        if (!recompute_centroids(img, hist, centroids))
            break;

        if (!reassign_values(img, hist, centroids))
            break;
    }

    unsigned cluster_size = 255 / NB_CLUSTERS;
    for (int i = 0; i < img->length; i += img->channels)
    {
        guchar val = hist[img->pixels[i]].cluster * cluster_size;
        img->pixels[i] = val;
        img->pixels[i + 1] = val;
        img->pixels[i + 2] = val;
    }
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

void ComputeImage(guchar* pucImaOrig, int NbLine, int NbCol, guchar* pucImaRes)
{
    int iNbPixelsTotal, iNumPix;
    int iNumChannel, iNbChannels = 3; /* on travaille sur des images couleurs*/
    guchar ucMeanPix;

    printf("Segmentation de l'image.... A vous!\n");

    iNbPixelsTotal = NbCol * NbLine;
    for (iNumPix = 0; iNumPix < iNbPixelsTotal * iNbChannels;
         iNumPix = iNumPix + iNbChannels)
    {
        /*moyenne sur les composantes RVB */
        ucMeanPix = (unsigned char)((*(pucImaOrig + iNumPix)
                                     + *(pucImaOrig + iNumPix + 1)
                                     + *(pucImaOrig + iNumPix + 2))
                                    / 3);
        /* sauvegarde du resultat */
        for (iNumChannel = 0; iNumChannel < iNbChannels; iNumChannel++)
            *(pucImaRes + iNumPix + iNumChannel) = ucMeanPix;
    }

    // clang-format off
    struct image img = {
        .width = NbCol,
        .height = NbLine,
        .channels = 3,
        .length = NbCol * NbLine * 3,
        .pixels = pucImaRes
    };
    // clang-format on

    struct timespec begin = now();
    kmeans(&img);
    struct timespec end = now();
    double total = elapsed_time(&begin, &end);

    printf("======================\n");
    printf("Execution time: %.4lf ms\n", total * 1000);
    printf("======================\n");
}
