#ifndef WINDOW_H
#define WINDOW_H

#include <X11/Xlib.h>
#include <stdlib.h>
#include "atelier.h"

#define foreach_wl(wl) for(WindowList *iter = (wl); iter != NULL; iter = iter->next)

typedef struct _WindowList WindowList;
struct _WindowList {
    Window frame;
    Window window;
    WindowList *prev;
    WindowList *next;
};

static inline int IsFrame(WindowList *wl, Window w) {
    return wl != NULL && wl->frame == w;
}

static inline int IsClient(WindowList *wl, Window w) {
    return wl != NULL && wl->window == w;
}

static inline WindowList* GetFirstWindow(WindowList *wl) {
    while (wl->prev) {
        wl = wl->prev;
    }
    return wl;
}

static inline WindowList* GetLastWindow(WindowList *wl) {
    while (wl->next) {
        wl = wl->next;
    }
    return wl;
}

static inline WindowList* GetPrevWindow(WindowList *wl) {
    return wl->prev? wl->prev : GetLastWindow(wl);
}

static inline WindowList* GetNextWindow(WindowList *wl) {
    return wl->next? wl->next : GetFirstWindow(wl);
}

static inline int GetWindowNum(WindowList *wl) {
    int i = 0;
    foreach_wl(wl) {
        i++;
    }
    return i;
}

static inline int GetSwitchableWindows(WindowList *wl, WindowList ***wl_return) {
    int size = GetWindowNum(wl);
    *wl_return = (WindowList**)calloc(size, sizeof(WindowList*));
    int i = 0;
    foreach_wl(wl) {
        XWindowAttributes attr;
        XGetWindowAttributes(disp, iter->frame, &attr);
        if (attr.map_state == IsViewable) {
            (*wl_return)[i++] = iter;
        }
    }
    return i;
}

static inline void FreeWindowsArray(WindowList ***wl_array) {
    free(*wl_array);
}

WindowList* CreateWindowList(Window frame, Window window);

//WindowをWMの管理下に置きフレームをつける
Window CatchWindow(Window window);

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window, Boolean frameDestroyed);

void ReleaseAllWindows();

WindowList* FindFrame(Window window);

//クライアントをフレームに合わせる
void FitToFrame(WindowList *wl);

//フレームをクライアントに合わせる
void FitToClient(WindowList *wl);

void GetWindowTitle(Window window, char *buffer, size_t buffer_length);

void DrawFrame(WindowList *wl);

#endif
