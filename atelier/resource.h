#ifndef RESOURCE_H
#define RESOURCE_H

#include <X11/Xlib.h>
#include <linux/limits.h>

typedef struct {
    Pixmap pixmap;
    unsigned int width, height;
    int x_hot, y_hot;
} BitmapRes;

typedef struct {
    char launcher_path[PATH_MAX+1];
} ConfigRes;

extern ConfigRes config;

void FreeBitmapRes(BitmapRes *res);

int ReadStaticBitmap(Drawable d, char *filename, BitmapRes **res_return);

void LoadConfig();

#endif
