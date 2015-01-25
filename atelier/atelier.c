#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/cursorfont.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "atelier.h"
#include "window.h"
#include "panel.h"

Display *disp;
Screen screen;
Window root;
GC gc;
Boolean terminate = FALSE;

extern XFontSet fontset;

typedef enum _GrabbedEdge GrabbedEdge;
enum _GrabbedEdge {
    EDGE_NONE   = 0,
    EDGE_TOP    = 1,
    EDGE_LEFT   = 1 << 1,
    EDGE_RIGHT  = 1 << 2,
    EDGE_BOTTOM = 1 << 3
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
    if (IsClient(wl, event.window)) {
        FitToClient(wl);
    }
}

void RaiseWindow(WindowList *wl) {
    XWindowAttributes attr;
    if (wl == NULL) return;
    XGetWindowAttributes(disp, wl->frame, &attr);
    if (attr.map_state == IsViewable) {
        XRaiseWindow(disp, wl->frame);
        XSetInputFocus(disp, wl->window, RevertToPointerRoot, CurrentTime);
        RaisePanel();
    }
}

static inline GrabbedEdge GetGrabbedEdge(XButtonEvent start, XWindowAttributes attr) {
    GrabbedEdge edge = EDGE_NONE;
    if (start.y < RESIZE_THRESHOLD)               edge |= EDGE_TOP;
    if (start.x < RESIZE_THRESHOLD)               edge |= EDGE_LEFT;
    if (start.x > attr.width  - RESIZE_THRESHOLD) edge |= EDGE_RIGHT;
    if (start.y > attr.height - RESIZE_THRESHOLD) edge |= EDGE_BOTTOM;
    return edge;
}

static Boolean SetSignal(int signame, void (*sighandle)(int signum)) {
    return signal(signame, sighandle) == SIG_ERR ? FALSE : TRUE;
}

static void QuitHandler(int signum) {
    fprintf(stderr, "term! :%d\n", signum);
    terminate = TRUE;
}

int main(int argc, char* argv[]) {
    Cursor normal_cursor;
    XEvent event;
    KeyCode tabKey;
    WindowList* lastRaised = NULL;
    Atom wm_protocols;
    Atom wm_delete_window;
    Atom net_wm_name;
    Atom wm_name;

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

    //パネルの初期化
    InitPanel();
    
    //ウィンドウ切り替えのパッシブグラブ
    tabKey = XKeysymToKeycode(disp, XStringToKeysym("Tab"));
    XGrabKey(disp, tabKey, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, tabKey, Mod1Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);

    //Atomの取得
    wm_protocols = XInternAtom(disp, "WM_PROTOCOLS", True);
    wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", True);
    wm_name = XInternAtom(disp, "WM_NAME", True);
    net_wm_name = XInternAtom(disp, "_NET_WM_NAME", True);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    //カーソルの設定
    normal_cursor = XCreateFontCursor(disp, XC_left_ptr);
    XDefineCursor(disp, root, normal_cursor);

    XSync(disp, False);

    while (!terminate) {
        if (XPending(disp)) {
            static XButtonEvent move_start;
            static XWindowAttributes move_attr;
            static GrabbedEdge move_edge;
            static WindowList* dispose_requested;
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
            case PropertyNotify:
                if (IsClient(wl, event.xany.window) && (event.xproperty.atom == net_wm_name || event.xproperty.atom == wm_name) && dispose_requested != wl) {
                    printf(" -> PropertyNotify Event:(NET_)WM_NAME, LW:%d, LF:%d\n", event.xany.window, wl, wl->window, wl->frame);
                    DrawFrame(wl);
                }
                break;
            case Expose:
                if (event.xexpose.count == 0 && IsFrame(wl, event.xexpose.window)) {
                    printf(" -> Expose Event, LW:%d, LF:%d\n", event.xexpose.window, wl, wl->window, wl->frame);
                    DrawFrame(wl);
                } else if (event.xexpose.count == 0 && IsPanel(event.xexpose.window)) {
                    printf(" -> Expose Event, Draw Panel.\n");
                    DrawPanel();
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

                    // Resize
                    if (move_edge & EDGE_TOP) {
                        y += event.xbutton.y_root - move_start.y_root;
                        height -= event.xbutton.y_root - move_start.y_root;
                    }
                    if (move_edge & EDGE_BOTTOM) {
                        height += event.xbutton.y_root - move_start.y_root;
                    }
                    if (move_edge & EDGE_LEFT) {
                        x += event.xbutton.x_root - move_start.x_root;
                        width -= event.xbutton.x_root - move_start.x_root;
                    }
                    if (move_edge & EDGE_RIGHT) {
                        width += event.xbutton.x_root - move_start.x_root;
                    }

                    // Move
                    if (!move_edge) {
                        x += event.xbutton.x_root - move_start.x_root;
                        y += event.xbutton.y_root - move_start.y_root;
                    }
                
                    XMoveResizeWindow(disp, event.xmotion.window,
                                      x, y, Max(1, width), Max(1, height));
                    FitToFrame(wl);
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
                    dispose_requested = wl;
                    if (delete_event.xclient.type == ClientMessage) {
                        XSendEvent(disp, wl->window, False, NoEventMask, &delete_event);
                    } else {
                        XKillClient(disp, wl->window);
                    }
                } else if (IsFrame(wl, event.xany.window) && event.xbutton.button == Button1) {
                    Cursor cursor;
                    printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event.xbutton.button, event.xany.window, wl, wl->window, wl->frame);
                    printf(" -> X: %d, Y: %d\n", event.xbutton.x, event.xbutton.y);
                    XGetWindowAttributes(disp, event.xbutton.window, &move_attr);
                    move_start = event.xbutton;
                    move_edge = GetGrabbedEdge(move_start, move_attr);
                    switch (move_edge) {
                    case EDGE_LEFT:
                        cursor = XCreateFontCursor(disp, XC_left_side);
                        break;
                    case EDGE_RIGHT:
                        cursor = XCreateFontCursor(disp, XC_right_side);
                        break;
                    case EDGE_TOP:
                        cursor = XCreateFontCursor(disp, XC_top_side);
                        break;
                    case EDGE_BOTTOM:
                        cursor = XCreateFontCursor(disp, XC_bottom_side);
                        break;
                    default:
                        cursor = XCreateFontCursor(disp, XC_fleur);
                        break;
                    }
                    XGrabPointer(disp, event.xbutton.window, True,
                                 PointerMotionMask | ButtonReleaseMask,
                                 GrabModeAsync, GrabModeAsync,
                                 None, cursor, CurrentTime);
                    printf(" -> Edge: %d\n", move_edge);
                } else if (IsPanel(event.xbutton.window) && event.xbutton.button == Button1) {
                    printf(" -> BPress[%d] Event, LW:%d\n", event.xbutton.button, event.xany.window);
                    OnClickPanel(event.xbutton);
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
        } else {
            static struct timespec ts = {0, 10000000};
            DrawPanelClock();
            XSync(disp, False);
            nanosleep(&ts, NULL);
        }
    }

    printf("Quitting Atelier...\n");

    XFreeGC(disp, gc);
    XUndefineCursor(disp, root);

    //管理しているウィンドウをすべて解放する
    ReleaseAllWindows();
    printf("[OK] Released Windows\n");
    //パネルを始末する
    DestroyPanel();
    printf("[OK] Released Panel\n");
    //ディスプレイとの接続を切る
    XCloseDisplay(disp);
    printf("[OK] Closed Display Connection\n");
    printf("[OK] Reached Quit Atelier\n");
    return 0;
} 
