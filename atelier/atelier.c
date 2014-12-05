#include <X11/Xlib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "atelier.h"
#include "window.h"

Display *disp;
Screen screen;
Window root;
GC gc;
Boolean terminate = FALSE;

void ConfigureRequestHandler(XConfigureRequestEvent event) {
    XWindowChanges change;
    change.x = event.x;
    change.y = event.y;
    change.width = event.width;
    change.height = event.height;
    change.border_width = event.border_width;
    change.sibling = event.above;
    change.stack_mode = event.detail;
    XConfigureWindow(disp, event.window, event.value_mask, &change);
}

static Boolean SetSignal(int signame, void (*sighandle)(int signum)) {
    return signal(signame, sighandle) == SIG_ERR ? FALSE : TRUE;
}

static void QuitHandler(int signum) {
    fprintf(stderr, "term! :%d\n", signum);
    terminate = TRUE;
}

int main(int argc, char* argv[]) {
    XEvent event;

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
    XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    XSync(disp, False);

    while (!terminate) {
        WindowList* wl;
        XNextEvent(disp, &event);

        wl = FindFrame(event.xany.window);
        printf("Event %d, W:%d, WL:%d\n", event.type, event.xany.window, wl);

        switch(event.type) {
        case MapRequest:
            printf(" -> MReq Event\n");
            XMapWindow(disp, CatchWindow(event.xmaprequest.window));
            XMapWindow(disp, event.xmaprequest.window);
            break;
        case UnmapNotify:
            if (wl != NULL) {
                printf(" -> Unmap Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                ReleaseWindow(wl, FALSE);
            } else {
                printf(" -> Unmap Event, Skip.\n");
            }
            break;
        case DestroyNotify:
            if (wl != NULL) {
                printf(" -> Destroy Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                ReleaseWindow(wl, TRUE);
            } else {
                printf(" -> Destroy Event, Skip.\n");
            }
            break;
        case ConfigureRequest:
            printf(" -> ConfigReq Event\n");
            ConfigureRequestHandler(event.xconfigurerequest);
            break;
        case Expose:
            if (event.xexpose.count == 0 && IsFrame(wl, event.xexpose.window)) {
                printf(" -> Expose Event, LW:%d, LF:%d\n", event.xexpose.window, wl, wl->window, wl->frame);
                DrawFrame(wl);
            } else {
                printf(" -> Expose Event, Skip.\n");
            }
            break;
        case ButtonPress:
            if (wl != NULL) {
                XRaiseWindow(disp, wl->frame);
            }
            if (IsFrame(wl, event.xany.window) && event.xbutton.button == Button3) {
                printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                XKillClient(disp, wl->window);
            } else {
                printf(" -> BPress[%d] Event, Skip.\n", event.xbutton.button);
            }
            break;
        }

        XSync(disp, False);
    }

    XFreeGC(disp, gc);

    //管理しているウィンドウをすべて解放する
    ReleaseAllWindows();
    XCloseDisplay(disp);
    return 0;
} 
