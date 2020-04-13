#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <dirent.h>
#include <err.h>
#include "compute.h"

int process_image(const char* name)
{
    GdkPixbuf* pix_buf = gdk_pixbuf_new_from_file(name, NULL);

    if (!pix_buf)
    {
        return 0;
    }

    printf("Processing file: %s ...\n", name);

    int nb_col = gdk_pixbuf_get_width(pix_buf);
    int nb_line = gdk_pixbuf_get_height(pix_buf);
    guchar* pixels = gdk_pixbuf_get_pixels(pix_buf);

    ComputeImage(pixels, nb_col, nb_line, NULL);

    return 1;
}

int main(int argc, char** argv)
{
    gtk_init(&argc, &argv);

    DIR* d = opendir(".");

    if (!d)
    {
        errx(1, "Could not open in current directory");
    }

    for (struct dirent* entry = readdir(d); entry; entry = readdir(d))
    {
        if (!process_image(entry->d_name))
        {
            continue;
        }
    }

    closedir(d);
}
