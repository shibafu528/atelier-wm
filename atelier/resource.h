#ifndef RESOURCE_H
#define RESOURCE_H

#include <X11/Xlib.h>

typedef struct {
    Pixmap pixmap;
    unsigned int width, height;
    int x_hot, y_hot;
} BitmapRes;

void FreeBitmapRes(BitmapRes *res);

int ReadStaticBitmap(Drawable d, char *filename, BitmapRes *res_return);

#endif
