#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"

#define FRAME_BORDER 2
#define FRAME_TITLE_HEIGHT 22

XFontSet fontset;
WindowList *windows = NULL;

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
    XAddToSaveSet(disp, window);
    frame = XCreateSimpleWindow(disp, root,
                                attr.x,
                                attr.y,
                                attr.width + FRAME_BORDER * 2 + attr.border_width * 2,
                                attr.height + FRAME_TITLE_HEIGHT + FRAME_BORDER + attr.border_width * 2,
                                1,
                                BlackPixel(disp, screen),
                                WhitePixel(disp, screen));
    XSelectInput(disp, frame, ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | SubstructureRedirectMask | SubstructureNotifyMask);
    XSelectInput(disp, window, PropertyChangeMask);
    XReparentWindow(disp, window, frame, FRAME_BORDER, FRAME_TITLE_HEIGHT);
    if (attr.map_state == IsViewable) {
        XMapWindow(disp, window);
        XMapWindow(disp, frame);
    }
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

void ReleaseAllWindows() {
    while (windows != NULL) {
        WindowList *next = windows->next;
        ReleaseWindow(windows, FALSE);
        windows = next;
    }
}

WindowList* FindFrame(Window window) {
    WindowList* wp = windows;
    while (wp != NULL && wp->window != window && wp->frame != window) {
        wp = wp->next;
    }
    return wp;
}

void FitToFrame(WindowList *wl) {
    XWindowAttributes frame_attr;
    XWindowAttributes window_attr;
    XGetWindowAttributes(disp, wl->frame, &frame_attr);
    XGetWindowAttributes(disp, wl->window, &window_attr);
    XMoveResizeWindow(disp, wl->window,
                      FRAME_BORDER, FRAME_TITLE_HEIGHT,
                      frame_attr.width - FRAME_BORDER * 2 - window_attr.border_width * 2,
                      frame_attr.height - FRAME_TITLE_HEIGHT - FRAME_BORDER - window_attr.border_width * 2);
}

void FitToClient(WindowList *wl) {
    XWindowAttributes window_attr;
    XGetWindowAttributes(disp, wl->window, &window_attr);
    XResizeWindow(disp, wl->frame,
                  window_attr.width + FRAME_BORDER * 2 + window_attr.border_width * 2,
                  window_attr.height + FRAME_TITLE_HEIGHT + FRAME_BORDER + window_attr.border_width * 2);
    XMoveWindow(disp, wl->window, FRAME_BORDER, FRAME_TITLE_HEIGHT);
}

void DrawFrame(WindowList *wl) {
    Atom net_wm_name = XInternAtom(disp, "_NET_WM_NAME", False);
    XWindowAttributes attr;
    XGetWindowAttributes(disp, wl->frame, &attr);
    
    XSetForeground(disp, gc, WhitePixel(disp, screen));
    XDrawRectangle(disp, wl->frame, gc, 0, 0, attr.width, attr.height);
    XFillRectangle(disp, wl->frame, gc, 0, 0, attr.width, 22);

    XSetForeground(disp, gc, BlackPixel(disp, screen));
    {
        int title_length = 512;
        char title[title_length];
        XTextProperty prop;
        title[0] = '\0';
        XGetTextProperty(disp, wl->window, &prop, net_wm_name);
        if (prop.nitems == 0) {
            XGetWMName(disp, wl->window, &prop);
        }
        if (prop.nitems > 0 && prop.value) {
            if (prop.encoding == XA_STRING) {
                strncpy(title, (char*) prop.value, title_length-1);
            } else {
                char **l = NULL;
                int count;
                XmbTextPropertyToTextList(disp, &prop, &l, &count);
                if (count > 0 && *l) {
                    strncpy(title, *l, title_length-1);
                }
                XFreeStringList(l);
            }
            title[511] = '\0';
        }
        XmbDrawString(disp, wl->frame, fontset, gc, 2, 16, title, strlen(title));
    }
}
