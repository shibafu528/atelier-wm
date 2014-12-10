#include <X11/Xlib.h>
#include <X11/Xlocale.h>
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

extern XFontSet fontset;

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
    KeyCode tabKey;
    WindowList* lastRaised = NULL;

    disp = XOpenDisplay(NULL);
    root = DefaultRootWindow(disp);

    if (setlocale(LC_CTYPE, "") == NULL) {
        return 1;
    }
    if (!XSupportsLocale()) {
        return 1;
    }

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
                lastRaised = FindFrame(children[i]);
            }
        }
        if (children != NULL) {
            XFree(children);
        }
    }

    gc = XCreateGC(disp, root, 0, NULL);
    XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask);

    {
        int missing_count;
        char** missing_list;
        char* def_string;
        fontset = XCreateFontSet(disp,
                                 "-*-fixed-medium-r-normal--16-*-*-*",
                                 &missing_list,
                                 &missing_count,
                                 &def_string);

        if (fontset == NULL) {
            return 1;
        }

        XFreeStringList(missing_list);
    }
    
    //ウィンドウ切り替えのパッシブグラブ
    tabKey = XKeysymToKeycode(disp, XStringToKeysym("Tab"));
    XGrabKey(disp, tabKey, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, tabKey, Mod1Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    XSync(disp, False);

    while (!terminate) {
        static XButtonEvent move_start;
        static XWindowAttributes move_attr;
        WindowList* wl;
        XNextEvent(disp, &event);

        wl = FindFrame(event.xany.window);
        printf("Event %d, W:%d, WL:%d\n", event.type, event.xany.window, wl);

        switch(event.type) {
        case MapRequest:
            printf(" -> MReq Event\n");
            XMapWindow(disp, CatchWindow(event.xmaprequest.window));
            XMapWindow(disp, event.xmaprequest.window);
            lastRaised = FindFrame(event.xmaprequest.window);
            break;
        case UnmapNotify:
            if (wl != NULL) {
                printf(" -> Unmap Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                lastRaised = wl->next? wl->next : wl->prev? wl->prev : NULL;
                ReleaseWindow(wl, FALSE);
            } else {
                printf(" -> Unmap Event, Skip.\n");
            }
            break;
        case DestroyNotify:
            if (wl != NULL) {
                printf(" -> Destroy Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                lastRaised = wl->next? wl->next : wl->prev? wl->prev : NULL;
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
        case MotionNotify:
            while (XCheckTypedEvent(disp, MotionNotify, &event));
            {
                int xdiff, ydiff;
                xdiff = event.xbutton.x_root - move_start.x_root;
                ydiff = event.xbutton.y_root - move_start.y_root;
                XMoveWindow(disp, event.xmotion.window,
                            move_attr.x + xdiff,
                            move_attr.y + ydiff);
            }
            break;
        case ButtonPress:
            if (wl != NULL) {
                lastRaised = wl;
                XRaiseWindow(disp, wl->frame);
            }
            if (IsFrame(wl, event.xany.window) && event.xbutton.button == Button3) {
                printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                XKillClient(disp, wl->window);
            } else if (IsFrame(wl, event.xany.window) && event.xbutton.button ==Button1) {
                printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                XGrabPointer(disp, event.xbutton.window, True,
                             PointerMotionMask | ButtonReleaseMask,
                             GrabModeAsync, GrabModeAsync,
                             None, None, CurrentTime);
                XGetWindowAttributes(disp, event.xbutton.window, &move_attr);
                move_start = event.xbutton;
            } else {
                printf(" -> BPress[%d] Event, Skip.\n", event.xbutton.button);
            }
            break;
        case ButtonRelease:
            XUngrabPointer(disp, CurrentTime);
            break;
        case KeyPress:
            if (event.xkey.keycode == tabKey) {
                printf(" -> KeyPress Event, SW:%d, LW:%d\n", event.xkey.subwindow, event.xany.window);
                if (lastRaised == NULL) {
                    lastRaised = FindFrame(event.xkey.subwindow);
                }
                if (lastRaised != NULL) {
                    lastRaised = event.xkey.state == Mod1Mask? GetNextWindow(lastRaised) : GetPrevWindow(lastRaised);
                    XRaiseWindow(disp, lastRaised->frame);
                }
            } else {
                printf(" -> KeyPress Event SW:%d , Skip.\n", event.xkey.subwindow);
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
