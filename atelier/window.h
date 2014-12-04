#ifndef WINDOW_H
#define WINDOW_H

#include <X11/Xlib.h>
#include "atelier.h"

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

WindowList* CreateWindowList(Window frame, Window window);

//WindowをWMの管理下に置きフレームをつける
Window CatchWindow(Window window);

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window, Boolean frameDestroyed);

void ReleaseAllWindows();

WindowList* FindFrame(Window window);

void DrawFrame(WindowList *wl);

#endif
