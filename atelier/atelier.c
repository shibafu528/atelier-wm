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
    XSetWindowAttributes setattr;
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
    setattr.override_redirect = True;
    XChangeWindowAttributes(disp, frame, CWOverrideRedirect, &setattr);
    XSelectInput(disp, frame, ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | SubstructureRedirectMask | SubstructureNotifyMask);
    XReparentWindow(disp, window, frame, 1, FRAME_TITLE_HEIGHT);
    if (attr.map_state == IsViewable) {
        XMapWindow(disp, window);
        XMapWindow(disp, frame);
    }
    XChangeSaveSet(disp, window, SetModeInsert);
    XSync(disp, FALSE);
    wl = CreateWindowList(frame, window);
    if (windows == NULL) {
        windows = wl;
    } else {
        windows->next = wl;
    }
    return frame;
}

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window) {
     XDestroyWindow(disp, window->frame);
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
    while (wp != NULL && wp->window != window) {
        wp = wp->next;
    }
    return wp;
}

void DrawFrame(WindowList *wl) {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, wl->frame, &attr);
    
    XSetForeground(disp, gc, BlackPixel(disp, screen));
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
        XQueryTree(disp, root, &r, &p, &children, &children_num);
        for (uint i = 0; i < children_num; i++) {
            CatchWindow(children[i]);
        }
        if (children != NULL) {
            XFree(children);
        }
    }

    gc = XCreateGC(disp, root, 0, NULL);
    XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    while(!terminate) {
        WindowList* wl;
        XNextEvent(disp, &event);

        switch(event.type) {
        case MapRequest:
            XMapWindow(disp, CatchWindow(event.xmaprequest.window));
            XMapWindow(disp, event.xmaprequest.window);
            break;
        case UnmapNotify:
        case DestroyNotify:
            wl = FindFrame(event.xunmap.window);
            if (wl != NULL) {
                ReleaseWindow(wl);
            }
            break;
        case Expose:
            if (event.xexpose.count == 0) {
                wl = FindFrame(event.xexpose.window);
                if (wl != NULL) {
                    DrawFrame(wl);
                }
            }
            break;
        }
    }

    XFreeGC(disp, gc);

    //管理しているウィンドウをすべて解放する
    while (windows != NULL) {
        WindowList *next = windows->next;
        ReleaseWindow(windows);
        windows = next;
    }
    XCloseDisplay(disp);
    return 0;
}
