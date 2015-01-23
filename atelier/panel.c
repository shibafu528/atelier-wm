#include <X11/Xlib.h>
#include <string.h>
#include <stdio.h>
#include "atelier.h"
#include "panel.h"
#include "time.h"
#include "resource.h"

#define BUTTON_PIXMAP_WIDTH 22

Window panel;
static XColor bgcolor;

extern XFontSet fontset;
extern Boolean terminate;

typedef enum {
    BUTTON_QUIT,
    BUTTON_LAUNCHER,
    BUTTON_HOME,
    BUTTON_SIZE
} ButtonRes;

static BitmapRes* buttons[BUTTON_SIZE];

static inline void DrawButtonRes(ButtonRes identifier, int x, int y) {
    XCopyPlane(disp, buttons[identifier]->pixmap, panel, gc, 0, 0,
              buttons[identifier]->width, buttons[identifier]->height,
               x, y, 1);
}

static inline int GetXPoint(int exist_icons) {
    return 2 + BUTTON_PIXMAP_WIDTH * exist_icons;
}

static void InitPanelResources() {
    ReadStaticBitmap(panel, "shutdown.xbm", &buttons[BUTTON_QUIT]);
    ReadStaticBitmap(panel, "launch.xbm", &buttons[BUTTON_LAUNCHER]);
}

static void FreePanelResources() {
    for (int id = 0; id < BUTTON_SIZE; id++) {
        if (buttons[id] != NULL) {
            FreeBitmapRes(buttons[id]);
        }
    }
}

void InitPanel() {
    Colormap cmap;
    XSetWindowAttributes attr;
    cmap = DefaultColormap(disp, 0);
    bgcolor.red = bgcolor.green = bgcolor.blue = 8192;
    XAllocColor(disp, cmap, &bgcolor);

    attr.colormap = cmap;
    attr.override_redirect = True;
    attr.background_pixel = bgcolor.pixel;
    
    panel = XCreateWindow(disp, root,
                          0, 0,
                          DisplayWidth(disp, 0), PANEL_HEIGHT,
                          0, CopyFromParent, CopyFromParent, CopyFromParent,
                          CWColormap | CWOverrideRedirect | CWBackPixel,
                          &attr);
    XSelectInput(disp, panel, ExposureMask | ButtonPressMask);
    XMapWindow(disp, panel);

    InitPanelResources();
}

void DestroyPanel() {
    FreePanelResources();
    XDestroyWindow(disp, panel);
}

static inline void DrawPanelBackground(XWindowAttributes *attr) {
    XSetForeground(disp, gc, bgcolor.pixel);
    XFillRectangle(disp, panel, gc, 0, 0, attr->width, attr->height);
}

void DrawPanelClock() {
    XWindowAttributes attr;
    char time_str[80];
    XGetWindowAttributes(disp, panel, &attr);
    PrintCurrentTime(time_str, 80);
    {
        int str_width = XmbTextEscapement(fontset, time_str, strlen(time_str));
        XRectangle rect = {attr.width - GetXPoint(1) - 2 - str_width, 0, str_width, PANEL_HEIGHT};
        XSetClipRectangles(disp, gc, 0, 0, &rect, 1, Unsorted);
        DrawPanelBackground(&attr);
        XSetForeground(disp, gc, WhitePixel(disp, screen));
        XmbDrawString(disp, panel, fontset, gc,
                  attr.width - GetXPoint(1) - 2 - str_width,
                  18, time_str, strlen(time_str));
        XSetClipMask(disp, gc, None);
    }
}

void DrawPanel() {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, panel, &attr);

    DrawPanelBackground(&attr);
    XSetForeground(disp, gc, bgcolor.pixel);
    XSetBackground(disp, gc, WhitePixel(disp, screen));
    DrawButtonRes(BUTTON_LAUNCHER, GetXPoint(0), 1);
    DrawButtonRes(BUTTON_QUIT, attr.width - GetXPoint(1), 1);
    DrawPanelClock();
}

void RaisePanel() {
    XRaiseWindow(disp, panel);
}

int IsPanel(Window w) {
    return panel == w;
}

void OnClickPanel(XButtonEvent event) {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, panel, &attr);
    //なにか判定とかterminateとか
    if (GetXPoint(0) <= event.x && event.x <= GetXPoint(1)) {
        //ここでrorolina起動したい
    }
    if (attr.width - GetXPoint(1) <= event.x && event.x <= attr.width - GetXPoint(0)) {
        printf("OnClickPanel: terminate");
        terminate = TRUE;
    }
}
