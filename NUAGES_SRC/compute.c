#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <gtk/gtk.h>

/*******************************************************
IL EST FORMELLEMENT INTERDIT DE CHANGER LE PROTOTYPE
DES FONCTIONS
*******************************************************/

#define SIZE (width * height * channels)

static int width = 0;
static int height = 0;
static int channels = 3;
static unsigned nb_clusters = 4;

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

int reassign_values(struct data* hist, guchar* centroids)
{
    int ret = 0;
    for (unsigned i = 0; i < 255; ++i)
    {
        unsigned old = hist[i].cluster;
        hist[i].cluster = 0;
        hist[i].dist = dist(i, centroids[0]);

        for (unsigned j = 1; j < nb_clusters; ++j)
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

void init_kmeans(struct data* hist, guchar* centroids, guchar* image)
{
    unsigned cluster_size = 255 / nb_clusters;
    for (unsigned i = 0; i < nb_clusters; ++i)
    {
        centroids[i] = i * cluster_size;
    }

    for (unsigned i = 0; i < 255; ++i)
    {
        hist[i].nb = 0;
    }

    reassign_values(hist, centroids);

    for (int i = 0; i < SIZE; i += channels)
    {
        guchar val = image[i];
        hist[val].nb++;
    }
}

guchar compute_centroid_n(unsigned n, unsigned* i, struct data* hist)
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

int recompute_centroids(struct data* hist, guchar* centroids)
{
    int ret = 0;
    unsigned i = 0;
    for (unsigned n = 0; n < nb_clusters; ++n)
    {
        guchar val = compute_centroid_n(n, &i, hist);
        ret = ret || centroids[n] != val;
        centroids[n] = val;
    }
    return ret;
}

void kmeans(guchar* src, guchar* dst)
{
    struct data hist[255];
    guchar* centroids = malloc(nb_clusters);
    init_kmeans(hist, centroids, src);

    while (1)
    {
        if (!recompute_centroids(hist, centroids))
            break;

        if (!reassign_values(hist, centroids))
            break;
    }

    unsigned cluster_size = 255 / nb_clusters;
    for (int i = 0; i < SIZE; i += channels)
    {
        guchar val = hist[src[i]].cluster * cluster_size;
        dst[i] = val;
        dst[i + 1] = val;
        dst[i + 2] = val;
    }

    free(centroids);
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
    width = NbCol;
    height = NbLine;

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

    kmeans(pucImaRes, pucImaRes);
}
