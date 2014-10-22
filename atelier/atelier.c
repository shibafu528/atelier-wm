#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define Boolean int
#define Screen int
#define TRUE 1
#define FALSE 0

#define FRAME_BORDER 1
#define FRAME_TITLE_HEIGHT 22

typedef struct _WindowList WindowList;
struct _WindowList {
    Window frame;
    Window window;
    GC gc;
    WindowList *prev;
    WindowList *next;
};

Display *disp;
Screen screen;
Window root;
WindowList *windows;
Boolean terminate = FALSE;

WindowList* CreateWindowList(Window frame, Window window, GC gc) {
    WindowList* wl = (WindowList*)malloc(sizeof(WindowList));
    wl->prev = NULL;
    wl->next = NULL;
    wl->frame = frame;
    wl->window = window;
    wl->gc = gc;
    return wl;
}

//WindowをWMの管理下に置きフレームをつける
void CatchWindow(XMapRequestEvent event) {
    XWindowAttributes attr;
    Window frame;
    GC gc;
    WindowList *wl;
    XGetWindowAttributes(disp, event.window, &attr);
    frame = XCreateSimpleWindow(disp, root,
                                attr.x - FRAME_TITLE_HEIGHT,
                                attr.y - FRAME_BORDER,
                                attr.width + FRAME_BORDER * 2,
                                attr.height + FRAME_TITLE_HEIGHT + 1,
                                1,
                                BlackPixel(disp, screen),
                                WhitePixel(disp, screen));
    XSelectInput(disp, frame, ExposureMask | ButtonPressMask | ButtonReleaseMask | Button1MotionMask | SubstructureRedirectMask | SubstructureNotifyMask);
    XReparentWindow(disp, event.window, frame, 0, 0);
    XMapWindow(disp, event.window);
    XMapWindow(disp, frame);
    XSync(disp, FALSE);
    gc = XCreateGC(disp, frame, 0, NULL);
    wl = CreateWindowList(frame, event.window, gc);
    if (windows == NULL) {
        windows = wl;
    } else {
        windows->next = wl;
    }
}

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window) {
     XWindowAttributes attr;
     Window parent;
     XQueryTree(disp, window->frame, NULL, &parent, NULL, NULL);
     XGetWindowAttributes(disp, window->frame, &attr);
     XReparentWindow(disp, window->window, parent, attr.x, attr.y);
     XFreeGC(disp, window->gc);
     XDestroyWindow(disp, window->frame);
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
    
    XSetForeground(disp, wl->gc, BlackPixel(disp, screen));
    XDrawRectangle(disp, wl->frame, wl->gc, 0, 0, attr.width, attr.height);
    XFillRectangle(disp, wl->frame, wl->gc, 0, 0, attr.width, 22);
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

    XSelectInput(disp, root, SubstructureRedirectMask);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    while(!terminate) {
        XNextEvent(disp, &event);

        switch(event.type) {
        case MapRequest:
            CatchWindow(event.xmaprequest);
            break;
        case Expose:
            if (event.xexpose.count == 0) {
                WindowList* wl = FindFrame(event.xexpose.window);
                if (wl != NULL) {
                    DrawFrame(wl);
                }
            }
            break;
        }
    }

    //管理しているウィンドウをすべて解放する
    while (windows != NULL) {
        WindowList *next = windows->next;
        ReleaseWindow(windows);
        windows = next;
    }
    XCloseDisplay(disp);
}
