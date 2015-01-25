#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>
#include <sys/stat.h>
#include <X11/Xlib.h>
#include <jansson.h>
#include "atelier.h"
#include "resource.h"

ConfigRes config;

static void GetInstalledDirectory(char* buffer, int buffer_length) {
    char path[PATH_MAX+1] = {};
    readlink("/proc/self/exe", path, sizeof(path) - 1);
    strncpy(buffer, dirname(path), buffer_length);
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
    char filepath[PATH_MAX+1] = {};
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

//json_tから指定keyのString値を取り出す。無ければ空白を返す
static inline const char* GetStringValueIfExist(json_t *object, const char* key) {
    json_t *value = json_object_get(object, key);
    if (value == NULL) {
        return "";
    } else {
        return json_string_value(value);
    }
}

void LoadConfig() {
    char filepath[PATH_MAX+1] = {};
    {
        char *home = getenv("HOME");
        if (home == NULL) {
            GetInstalledDirectory(filepath, sizeof(filepath));
            sprintf(filepath, "%s/atelierrc.default", filepath);
        } else {
            sprintf(filepath, "%s/.atelierrc", home);
            if (stat(filepath, &(struct stat){}) != 0) {
                GetInstalledDirectory(filepath, sizeof(filepath));
                sprintf(filepath, "%s/atelierrc.default", filepath);
            }
        }
        printf("LoadConfig: %s\n", filepath);
    }
    json_t *root = json_load_file(filepath, 0, &(json_error_t){});
    if (json_is_object(root)) {
        strncpy(config.launcher_path, GetStringValueIfExist(root, "launcher"), sizeof(config.launcher_path));
    }
}
