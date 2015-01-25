#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <stdio.h>
#include "atelier.h"
#include "window.h"
#include "panel.h"

#define RESIZE_THRESHOLD 4

typedef enum {
    EDGE_NONE   = 0,
    EDGE_TOP    = 1,
    EDGE_LEFT   = 1 << 1,
    EDGE_RIGHT  = 1 << 2,
    EDGE_BOTTOM = 1 << 3
} GrabbedEdge;

static KeyCode tab_key;
static Atom wm_protocols;
static Atom wm_delete_window;
static Atom net_wm_name;
static Atom wm_name;
static WindowList *dispose_requested;

static struct {
    XButtonEvent start;
    XWindowAttributes attr;
    GrabbedEdge edge;
} move_event;

extern WindowList *last_raised;

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

static void MapRequestHandler(XEvent *event, WindowList *wl) {
    printf(" -> MReq Event\n");
    XMapWindow(disp, CatchWindow(event->xmaprequest.window));
    XMapWindow(disp, event->xmaprequest.window);
    last_raised = FindFrame(event->xmaprequest.window);
    RaiseWindow(last_raised);
    DrawPanelSwitcher();
}

static void UnmapNotifyHandler(XEvent *event, WindowList *wl) {
    if (wl != NULL) {
        printf(" -> Unmap Event, LW:%d, LF:%d\n", event->xany.window, wl, wl->window, wl->frame);
        last_raised = wl->next? wl->next : wl->prev? wl->prev : NULL;
        ReleaseWindow(wl, FALSE);
        RaiseWindow(last_raised);
        DrawPanelSwitcher();
    } else {
        printf(" -> Unmap Event, Skip.\n");
    }
}

static void DestroyNotifyHandler(XEvent *event, WindowList *wl) {
    if (wl != NULL) {
        printf(" -> Destroy Event, LW:%d, LF:%d\n", event->xany.window, wl, wl->window, wl->frame);
        last_raised = wl->next? wl->next : wl->prev? wl->prev : NULL;
        ReleaseWindow(wl, TRUE);
        RaiseWindow(last_raised);
        DrawPanelSwitcher();
    } else {
        printf(" -> Destroy Event, Skip.\n");
    }
}

static void ConfigureRequestHandler(XEvent *event, WindowList *wl) {
    XConfigureRequestEvent *conreq = &event->xconfigurerequest;
    XConfigureWindow(disp, event->xany.window,
                     conreq->value_mask, &(XWindowChanges){
                             .x = conreq->x,
                             .y = conreq->y,
                             .width = conreq->width,
                             .height = conreq->height,
                             .border_width = conreq->border_width,
                             .sibling = conreq->above,
                             .stack_mode = conreq->detail
    });
    if (IsClient(wl, event->xany.window)) {
        FitToClient(wl);
    }
}

static void PropertyNotifyHandler(XEvent *event, WindowList *wl) {
    if (IsClient(wl, event->xany.window) && (event->xproperty.atom == net_wm_name || event->xproperty.atom == wm_name) && dispose_requested != wl) {
        printf(" -> PropertyNotify Event:(NET_)WM_NAME, LW:%d, LF:%d\n", event->xany.window, wl, wl->window, wl->frame);
        DrawFrame(wl);
        DrawPanelSwitcher();
    }
}

static void ExposeHandler(XEvent *event, WindowList *wl) {
    if (event->xexpose.count == 0 && IsFrame(wl, event->xexpose.window)) {
        printf(" -> Expose Event, LW:%d, LF:%d\n", event->xexpose.window, wl, wl->window, wl->frame);
        DrawFrame(wl);
    } else if (event->xexpose.count == 0 && IsPanel(event->xexpose.window)) {
        printf(" -> Expose Event, Draw Panel.\n");
        DrawPanel();
    } else {
        printf(" -> Expose Event, Skip.\n");
    }
}

static void MotionNotifyHandler(XEvent *event, WindowList *wl) {
    //余計なMotionNotifyイベントを全部捨てる
    while (XCheckTypedEvent(disp, MotionNotify, event));
    
    int x, y, width, height;
    x = move_event.attr.x;
    y = move_event.attr.y;
    width = move_event.attr.width;
    height = move_event.attr.height;

    // Resize
    if (move_event.edge & EDGE_TOP) {
        y += event->xbutton.y_root - move_event.start.y_root;
        height -= event->xbutton.y_root - move_event.start.y_root;
    }
    if (move_event.edge & EDGE_BOTTOM) {
        height += event->xbutton.y_root - move_event.start.y_root;
    }
    if (move_event.edge & EDGE_LEFT) {
        x += event->xbutton.x_root - move_event.start.x_root;
        width -= event->xbutton.x_root - move_event.start.x_root;
    }
    if (move_event.edge & EDGE_RIGHT) {
        width += event->xbutton.x_root - move_event.start.x_root;
    }

    // Move
    if (!move_event.edge) {
        x += event->xbutton.x_root - move_event.start.x_root;
        y += event->xbutton.y_root - move_event.start.y_root;
    }
                
    XMoveResizeWindow(disp, event->xmotion.window,
                      x, y, Max(1, width), Max(1, height));
    FitToFrame(wl);
}

static void ButtonPressHandler(XEvent *event, WindowList *wl) {
    if (wl != NULL) {
        last_raised = wl;
        RaiseWindow(wl);
        DrawPanelSwitcher();
    }
    if (IsFrame(wl, event->xany.window) && event->xbutton.button == Button3) {
        Atom *protocols;
        int protocols_num;
        XEvent delete_event;
        printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event->xbutton.button, event->xany.window, wl, wl->window, wl->frame);
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
    } else if (IsFrame(wl, event->xany.window) && event->xbutton.button == Button1) {
        Cursor cursor;
        printf(" -> BPress[%d] Event, LW:%d, LF:%d\n", event->xbutton.button, event->xany.window, wl, wl->window, wl->frame);
        printf(" -> X: %d, Y: %d\n", event->xbutton.x, event->xbutton.y);
        XGetWindowAttributes(disp, event->xbutton.window, &move_event.attr);
        move_event.start = event->xbutton;
        move_event.edge = GetGrabbedEdge(move_event.start, move_event.attr);
        switch (move_event.edge) {
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
        XGrabPointer(disp, event->xbutton.window, True,
                     PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync,
                     None, cursor, CurrentTime);
        printf(" -> Edge: %d\n", move_event.edge);
    } else if (IsPanel(event->xbutton.window) && event->xbutton.button == Button1) {
        printf(" -> BPress[%d] Event, LW:%d\n", event->xbutton.button, event->xany.window);
        OnClickPanel(event->xbutton);
    } else {
        printf(" -> BPress[%d] Event, Skip.\n", event->xbutton.button);
    }
}

static void ButtonReleaseHandler(XEvent *event, WindowList *wl) {
    printf(" -> BRelease[%d] Event.\n", event->xbutton.button);
    XUngrabPointer(disp, CurrentTime);
}

static void KeyPressHandler(XEvent *event, WindowList *wl) {
    if (event->xkey.keycode == tab_key) {
        printf(" -> KeyPress Event, SW:%d, LW:%d\n", event->xkey.subwindow, event->xany.window);
        if (last_raised == NULL) {
            last_raised = FindFrame(event->xkey.subwindow);
        }
        if (last_raised != NULL) {
            last_raised = event->xkey.state == Mod1Mask? GetNextWindow(last_raised) : GetPrevWindow(last_raised);
            RaiseWindow(last_raised);
            DrawPanelSwitcher();
        }
    } else {
        printf(" -> KeyPress Event SW:%d , Skip.\n", event->xkey.subwindow);
    }
}

static void (*handler[])(XEvent *event, WindowList *wl) = {
    [KeyPress] = KeyPressHandler,
    [ButtonPress] = ButtonPressHandler,
    [ButtonRelease] = ButtonReleaseHandler,
    [MotionNotify] = MotionNotifyHandler,
    [Expose] = ExposeHandler,
    [DestroyNotify] = DestroyNotifyHandler,
    [UnmapNotify] = UnmapNotifyHandler,
    [MapRequest] = MapRequestHandler,
    [ConfigureRequest] = ConfigureRequestHandler,
    [PropertyNotify] = PropertyNotifyHandler,
    [LASTEvent] = NULL
};

void InitEventHandler() {
    tab_key = XKeysymToKeycode(disp, XStringToKeysym("Tab"));
    wm_protocols = XInternAtom(disp, "WM_PROTOCOLS", True);
    wm_delete_window = XInternAtom(disp, "WM_DELETE_WINDOW", True);
    wm_name = XInternAtom(disp, "WM_NAME", True);
    net_wm_name = XInternAtom(disp, "_NET_WM_NAME", True);
}

void CallEventHandler(XEvent event) {
    WindowList *wl = FindFrame(event.xany.window);
    printf("Event %d, W:%d, WL:%d\n", event.type, event.xany.window, wl);
    if (event.type < LASTEvent && handler[event.type] != NULL) {
        handler[event.type](&event, wl);
    } else {
        printf(" -> Handler not found.\n", event.type, event.xany.window);
    }
}
