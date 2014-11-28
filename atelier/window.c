#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include "window.h"

extern Display *disp;
extern Screen screen;
extern Window root;
extern GC gc;
WindowList *windows;

WindowList* CreateWindowList(Window frame, Window window) {
    WindowList* wl = (WindowList*)malloc(sizeof(WindowList));
    wl->prev = NULL;
    wl->next = NULL;
    wl->frame = frame;
    wl->window = window;
    return wl;
}

//WindowをWMの管理下に置きフレームをつける
Window CatchWindow(Window window) {
    XWindowAttributes attr;
    Window frame;
    WindowList *wl;
    XGetWindowAttributes(disp, window, &attr);
    frame = XCreateSimpleWindow(disp, root,
                                attr.x,
                                attr.y,
                                attr.width + FRAME_BORDER * 2,
                                attr.height + FRAME_TITLE_HEIGHT + 1,
                                1,
                                BlackPixel(disp, screen),
                                WhitePixel(disp, screen));
    XSelectInput(disp, frame, ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | SubstructureRedirectMask | SubstructureNotifyMask);
    XReparentWindow(disp, window, frame, 1, FRAME_TITLE_HEIGHT);
    if (attr.map_state == IsViewable) {
        XMapWindow(disp, window);
        XMapWindow(disp, frame);
    }
    XAddToSaveSet(disp, window);
    wl = CreateWindowList(frame, window);
    if (windows == NULL) {
        windows = wl;
    } else {
        WindowList *connect_to = windows;
        while (connect_to->next != NULL) {
            connect_to = connect_to->next;
        }
        connect_to->next = wl;
        wl->prev = connect_to;
    }
    printf("CatchWindow W:%d, F:%d, WL:%d\n", window, frame, wl);
    return frame;
}

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window, Boolean frameDestroyed) {
    if (!frameDestroyed) {
        XDestroyWindow(disp, window->frame);
    }
    XSync(disp, False);
    if (windows == window) {
        windows = window->next;
    }
    if (window->prev != NULL) {
        window->prev->next = window->next;
    }
    if (window->next != NULL) {
        window->next->prev = window->prev;
    }
    free(window);
}

WindowList* FindFrame(Window window) {
    WindowList* wp = windows;
    while (wp != NULL && wp->window != window && wp->frame != window) {
        wp = wp->next;
    }
    return wp;
}

void DrawFrame(WindowList *wl) {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, wl->frame, &attr);
    
    XSetForeground(disp, gc, WhitePixel(disp, screen));
    XDrawRectangle(disp, wl->frame, gc, 0, 0, attr.width, attr.height);
    XFillRectangle(disp, wl->frame, gc, 0, 0, attr.width, 22);
}
