#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "window.h"
#include "panel.h"

#define FRAME_BORDER 2
#define FRAME_TITLE_HEIGHT 22

XFontSet fontset;
WindowList *windows = NULL;
WindowList *last_raised = NULL;

WindowList* CreateWindowList(Window frame, Window window) {
    WindowList* wl = (WindowList*)malloc(sizeof(WindowList));
    memset(wl, 0, sizeof(WindowList));
    wl->state = NormalState;
    wl->frame = frame;
    wl->window = window;
    return wl;
}

static int SetWMState(Window w, WMState state) {
    unsigned long data[2] = {state, None};
    Atom wm_state = XInternAtom(disp, "WM_STATE", False);
    XChangeProperty(disp, w, wm_state, wm_state, 32, PropModeReplace, (unsigned char*)data, 2);
    return state;
}

static WMState GetWMState(Window w) {
    Atom wm_state = XInternAtom(disp, "WM_STATE", False);
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop_return = NULL;
    WMState state_return = 0;

    XGetWindowProperty(disp, w, wm_state, 0, 2, False, wm_state,
                       &actual_type, &actual_format, &nitems, &bytes_after,
                       &prop_return);
    if (nitems == 1) {
        unsigned long *datap = (unsigned long*)prop_return;
        state_return = (WMState)datap[0];
    }
    XFree(prop_return);
    return state_return;
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
                                Max(attr.y, PANEL_HEIGHT),
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

void GetWindowTitle(Window window, char *buffer, size_t buffer_length) {
    Atom net_wm_name = XInternAtom(disp, "_NET_WM_NAME", False);
    XTextProperty prop;
    buffer[0] = '\0';
    XGetTextProperty(disp, window, &prop, net_wm_name);
    if (prop.nitems == 0) {
        XGetWMName(disp, window, &prop);
    }
    if (prop.nitems > 0 && prop.value) {
        if (prop.encoding == XA_STRING) {
            strncpy(buffer, (char*) prop.value, buffer_length-1);
        } else {
            char **l = NULL;
            int count;
            XmbTextPropertyToTextList(disp, &prop, &l, &count);
            if (count > 0 && *l) {
                strncpy(buffer, *l, buffer_length-1);
            }
            XFreeStringList(l);
        }
        buffer[buffer_length-1] = '\0';
    }
}

void DrawFrame(WindowList *wl) {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, wl->frame, &attr);
    
    XSetForeground(disp, gc, WhitePixel(disp, screen));
    XDrawRectangle(disp, wl->frame, gc, 0, 0, attr.width, attr.height);
    XFillRectangle(disp, wl->frame, gc, 0, 0, attr.width, 22);

    XSetForeground(disp, gc, BlackPixel(disp, screen));
    {
        char title[512];
        GetWindowTitle(wl->window, title, sizeof(title));
        XmbDrawString(disp, wl->frame, fontset, gc, 2, 16, title, strlen(title));
    }
}

void RaiseWindow(WindowList *wl) {
    XWindowAttributes attr;
    if (wl == NULL) return;
    XGetWindowAttributes(disp, wl->frame, &attr);
    if (wl->state == IconicState) {
        DeIconifyWindow(wl);
        XRaiseWindow(disp, wl->frame);
        XSetInputFocus(disp, wl->window, RevertToPointerRoot, CurrentTime);
        RaisePanel();
    } else if (attr.map_state == IsViewable) {
        XRaiseWindow(disp, wl->frame);
        XSetInputFocus(disp, wl->window, RevertToPointerRoot, CurrentTime);
        RaisePanel();
    }
}

void IconifyWindow(WindowList *wl) {
    wl->state = SetWMState(wl->window, IconicState);
    XUnmapWindow(disp, wl->window);
    XUnmapWindow(disp, wl->frame);
    last_raised = GetPrevWindow(wl);
    while (last_raised->state == IconicState && last_raised != wl) {
        last_raised = GetPrevWindow(last_raised);
    }
}

void DeIconifyWindow(WindowList *wl) {
    wl->state = SetWMState(wl->window, NormalState);
    XMapWindow(disp, wl->frame);
    XMapWindow(disp, wl->window);
}
