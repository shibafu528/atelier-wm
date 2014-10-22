#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define Boolean int
#define Screen int
#define TRUE 1
#define FALSE 0

typedef struct {
    Window frame;
    Window window;
    struct WindowList *prev;
    struct WindowList *next;
} WindowList;

Display *disp;
Screen screen;
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
void CatchWindow(XMapRequestEvent event) {
    
}

//Windowからフレームを除去しWMの管理から外す
void ReleaseWindow(WindowList *window) {
     XWindowAttributes attr;
     Window parent;
     XQueryTree(disp, window->frame, NULL, &parent, NULL, NULL);
     XGetWindowAttributes(disp, window->frame, &attr);
     XReparentWindow(disp, window->window, parent, attr.x, attr.y);
     XDestroyWindow(disp, window->frame);
     if (window->prev != NULL) {
         window->prev->next = window->next;
     }
     if (window->next != NULL) {
         window->next->prev = window->prev;
     }
     free(window);
}

Boolean SetSignal(int signame, void (*sighandle)(int signum)) {
    return signal(signame, sighandle) == SIG_ERR ? FALSE : TRUE;
}

void QuitHandler(int signum) {
    printf("term! :%d\n", signum);
    terminate = TRUE;
}

int main(int argc, char* argv[]) {
    Window root;
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
        while (XPending(disp)) {
            XNextEvent(disp, &event);

            switch(event.type) {
            case MapRequest:
                break;
            }
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
