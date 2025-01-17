#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <dirent.h>
#include <err.h>
#include "compute.h"

void process_image(const char* name)
{
    GdkPixbuf* pix_buf = gdk_pixbuf_new_from_file(name, NULL);

    if (!pix_buf)
    {
        return;
    }

    printf("============================================\n");
    printf("Processing file: %s ...\n", name);

    int width = gdk_pixbuf_get_width(pix_buf);
    int height = gdk_pixbuf_get_height(pix_buf);
    guchar* pixels = gdk_pixbuf_get_pixels(pix_buf);

    ComputeImage(pixels, height, width, NULL);

    gdk_pixbuf_unref(pix_buf);
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
        process_image(entry->d_name);
    }

    closedir(d);
}
