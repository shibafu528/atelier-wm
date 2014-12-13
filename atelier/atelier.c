#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "atelier.h"
#include "window.h"

Display *disp;
Screen screen;
Window root;
GC gc;
Boolean terminate = FALSE;

extern XFontSet fontset;

typedef enum _GrabbedEdge GrabbedEdge;
enum _GrabbedEdge {
    EDGE_NONE,
    EDGE_TOP,
    EDGE_LEFT,
    EDGE_RIGHT,
    EDGE_BOTTOM
};

#define RESIZE_THRESHOLD 4

void ConfigureRequestHandler(XConfigureRequestEvent event) {
    WindowList *wl = FindFrame(event.window);
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

void RaiseWindow(WindowList *wl) {
    if (wl == NULL) return;
    XRaiseWindow(disp, wl->frame);
    XSetInputFocus(disp, wl->window, RevertToPointerRoot, CurrentTime);
}

static inline GrabbedEdge GetGrabbedEdge(XButtonEvent start, XWindowAttributes attr) {
    if      (start.y < RESIZE_THRESHOLD)               return EDGE_TOP;
    else if (start.x < RESIZE_THRESHOLD)               return EDGE_LEFT;
    else if (start.x > attr.width  - RESIZE_THRESHOLD) return EDGE_RIGHT;
    else if (start.y > attr.height - RESIZE_THRESHOLD) return EDGE_BOTTOM;
    else                                               return EDGE_NONE;
}

static inline int Max(int a, int b) {
    return a > b ? a : b;
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
    Atom wm_protocols;
    Atom wm_delete_window;

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

    //Atomの取得
    wm_protocols = XInternAtom(disp, "WM_PROTOCOLS", True);
    wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", True);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    XSync(disp, False);

    while (!terminate) {
        static XButtonEvent move_start;
        static XWindowAttributes move_attr;
        static GrabbedEdge move_edge;
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
            RaiseWindow(lastRaised);
            break;
        case UnmapNotify:
            if (wl != NULL) {
                printf(" -> Unmap Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                lastRaised = wl->next? wl->next : wl->prev? wl->prev : NULL;
                ReleaseWindow(wl, FALSE);
                RaiseWindow(lastRaised);
            } else {
                printf(" -> Unmap Event, Skip.\n");
            }
            break;
        case DestroyNotify:
            if (wl != NULL) {
                printf(" -> Destroy Event, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                lastRaised = wl->next? wl->next : wl->prev? wl->prev : NULL;
                ReleaseWindow(wl, TRUE);
                RaiseWindow(lastRaised);
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
                int x, y, width, height;
                x = move_attr.x;
                y = move_attr.y;
                width = move_attr.width;
                height = move_attr.height;
                
                switch (move_edge) {
                case EDGE_TOP:
                    y += event.xbutton.y_root - move_start.y_root;
                    height -= event.xbutton.y_root - move_start.y_root;
                    break;
                case EDGE_BOTTOM:
                    height += event.xbutton.y_root - move_start.y_root;
                    break;
                case EDGE_LEFT:
                    x += event.xbutton.x_root - move_start.x_root;
                    width -= event.xbutton.x_root - move_start.x_root;
                    break;
                case EDGE_RIGHT:
                    width += event.xbutton.x_root - move_start.x_root;
                    break;
                default:
                    // Move
                    x += event.xbutton.x_root - move_start.x_root;
                    y += event.xbutton.y_root - move_start.y_root;
                    break;
                }
                
                XMoveResizeWindow(disp, event.xmotion.window,
                                  x, y, Max(1, width), Max(1, height));
                FitFrame(wl);
            }
            break;
        case ButtonPress:
            if (wl != NULL) {
                lastRaised = wl;
                RaiseWindow(wl);
            }
            if (IsFrame(wl, event.xany.window) && event.xbutton.button == Button3) {
                Atom *protocols;
                int protocols_num;
                XEvent delete_event;
                printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                XGetWMProtocols(disp, wl->window, &protocols, &protocols_num);
                printf(" -> WM_PROTOCOLS * %d\n", protocols_num);
                for (int i = 0; i < protocols_num; i++) {
                    if (protocols[i] == wm_delete_window) {
                        printf(" -> Found WM_DELETE_WINDOW\n");
                        delete_event.xclient.type = ClientMessage;
                        delete_event.xclient.window = wl->window;
                        delete_event.xclient.message_type = wm_protocols;
                        delete_event.xclient.format = 32;
                        delete_event.xclient.data.l[0] = wm_delete_window;
                        delete_event.xclient.data.l[1] = CurrentTime;
                        break;
                    }
                }
                XFree(protocols);
                if (delete_event.xclient.type == ClientMessage) {
                    XSendEvent(disp, wl->window, False, NoEventMask, &delete_event);
                } else {
                    XKillClient(disp, wl->window);
                }
            } else if (IsFrame(wl, event.xany.window) && event.xbutton.button ==Button1) {
                printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                printf(" -> X: %d, Y: %d\n", event.xbutton.x, event.xbutton.y);
                XGrabPointer(disp, event.xbutton.window, True,
                             PointerMotionMask | ButtonReleaseMask,
                             GrabModeAsync, GrabModeAsync,
                             None, None, CurrentTime);
                XGetWindowAttributes(disp, event.xbutton.window, &move_attr);
                move_start = event.xbutton;
                move_edge = GetGrabbedEdge(move_start, move_attr);
                printf(" -> Edge: %d\n", move_edge);
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
                    RaiseWindow(lastRaised);
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
