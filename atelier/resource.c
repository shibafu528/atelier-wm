#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <X11/Xlib.h>
#include "atelier.h"
#include "resource.h"

static void GetInstalledDirectory(char* buffer, int buffer_length) {
    char path[1024] = {};
    char* base;
    readlink("/proc/self/exe", path, sizeof(path) - 1);
    base = dirname(path);
    strncpy(buffer, base, buffer_length);
}

static inline void NewBitmapRes(BitmapRes **res) {
    *res = (BitmapRes*)malloc(sizeof(BitmapRes));
    memset(*res, 0, sizeof(BitmapRes));
}

void FreeBitmapRes(BitmapRes *res) {
    XFreePixmap(disp, res->pixmap);
    free(res);
}

//インストールディレクトリにあるBitmapを読み出す。使用後はFreeBitmapResで解放
int ReadStaticBitmap(Drawable d, char *filename, BitmapRes **res_return) {
    char filepath[2048] = {};
    //ファイルパスを生成
    GetInstalledDirectory(filepath, sizeof(filepath));
    sprintf(filepath, "%s/%s", filepath, filename);
    printf("ReadStaticBitmap: %s\n", filepath);
    //読み出しを行う
    NewBitmapRes(res_return);
    return XReadBitmapFile(disp, d, filepath,
                           &(*res_return)->width, &(*res_return)->height,
                           &(*res_return)->pixmap,
                           &(*res_return)->x_hot, &(*res_return)->y_hot);
}
