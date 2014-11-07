#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define Boolean int
#define uint unsigned int
#define Screen int
#define TRUE 1
#define FALSE 0

#define FRAME_BORDER 1
#define FRAME_TITLE_HEIGHT 22

typedef struct _WindowList WindowList;
struct _WindowList {
    Window frame;
    Window window;
    WindowList *prev;
    WindowList *next;
};

Display *disp;
Screen screen;
Window root;
GC gc;
WindowList *windows;
Boolean terminate = FALSE;

static inline int IsFrame(WindowList *wl, Window w) {
    return wl != NULL && wl->frame == w;
}

static inline int IsClient(WindowList *wl, Window w) {
    return wl != NULL && wl->window == w;
}

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
    XChangeSaveSet(disp, window, SetModeInsert);
    XReparentWindow(disp, window, frame, 1, FRAME_TITLE_HEIGHT);
    if (attr.map_state == IsViewable) {
        XMapWindow(disp, window);
        XMapWindow(disp, frame);
    }
    XSync(disp, FALSE);
    wl = CreateWindowList(frame, window);
    if (windows == NULL) {
        windows = wl;
    } else {
        WindowList *connect_to = windows;
        while (connect_to->next != NULL) {
            connect_to = connect_to->next;
        }
        connect_to->next = wl;
    }
    printf("CatchWindow W:%d, F:%d, WL:%d\n", window, frame, wl);
    return frame;
}

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window, Boolean frameDestroyed) {
    if (!frameDestroyed) {
        XDestroyWindow(disp, window->frame);
    }
    XSync(disp, FALSE);
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

Boolean SetSignal(int signame, void (*sighandle)(int signum)) {
    return signal(signame, sighandle) == SIG_ERR ? FALSE : TRUE;
}

void QuitHandler(int signum) {
    fprintf(stderr, "term! :%d\n", signum);
    terminate = TRUE;
}

int main(int argc, char* argv[]) {
    XEvent event;

    windows = NULL;
    disp = XOpenDisplay(NULL);
    root = DefaultRootWindow(disp);

    {
        Window r, p;
        Window* children;
        uint children_num;
        XWindowAttributes attr;
        XQueryTree(disp, root, &r, &p, &children, &children_num);
        for (uint i = 0; i < children_num; i++) {
            XGetWindowAttributes(disp, children[i], &attr);
            if (!attr.override_redirect) {
                CatchWindow(children[i]);
            }
        }
        if (children != NULL) {
            XFree(children);
        }
    }

    gc = XCreateGC(disp, root, 0, NULL);
    //XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    XSync(disp, FALSE);

    while (!terminate) {
        WindowList* wl;
        while (XPending(disp)) {
            XNextEvent(disp, &event);

            wl = FindFrame(event.xany.window);
            printf("Event %d, W:%d, WL:%d\n", event.type, event.xany.window, wl);

            switch(event.type) {
            case MapRequest:
                printf(" -> MReq Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                XMapWindow(disp, CatchWindow(event.xmaprequest.window));
                XMapWindow(disp, event.xmaprequest.window);
                break;
            case UnmapNotify:
                printf(" -> Unmap Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                if (wl != NULL) {
                    ReleaseWindow(wl, FALSE);
                }
            case DestroyNotify:
                printf(" -> Destroy Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                if (wl != NULL) {
                    ReleaseWindow(wl, TRUE);
                }
                break;
            case Expose:
                printf(" -> Expose Event, LW:%d, LF:%d\n", event.xexpose.window, wl, wl->window, wl->frame);
                if (event.xexpose.count == 0 && IsFrame(wl, event.xexpose.window)) {
                    DrawFrame(wl);
                }
                break;
            case ButtonPress:
                printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                if (IsFrame(wl, event.xany.window) && event.xbutton.button == Button3) {
                    XDestroyWindow(disp, wl->frame);
                }
                break;
            }
        }
    }

    XFreeGC(disp, gc);

    //管理しているウィンドウをすべて解放する
    while (windows != NULL) {
        WindowList *next = windows->next;
        ReleaseWindow(windows, FALSE);
        windows = next;
    }
    XCloseDisplay(disp);
    return 0;
}
