#include <X11/Xlib.h>
#include <string.h>
#include "atelier.h"
#include "panel.h"
#include "time.h"
#include "resource.h"

#define BUTTON_PIXMAP_WIDTH 22

Window panel;
XColor bgcolor;
extern XFontSet fontset;

enum {
    BUTTON_QUIT,
    BUTTON_SIZE
};

static BitmapRes buttons[BUTTON_SIZE];

static inline int GetXPointFromRight(int exist_icons) {
    return 2 + BUTTON_PIXMAP_WIDTH * exist_icons;
}

static void InitPanelResources() {
    ReadStaticBitmap(panel, "shutdown.xbm", &buttons[BUTTON_QUIT]);
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
    XSelectInput(disp, panel, ExposureMask);
    XMapWindow(disp, panel);

    InitPanelResources();
}

void DestroyPanel() {
    XDestroyWindow(disp, panel);
}

void DrawPanel() {
    XWindowAttributes attr;
    char time_str[80];
    XGetWindowAttributes(disp, panel, &attr);
    PrintCurrentTime(time_str, 80);

    XSetForeground(disp, gc, bgcolor.pixel);
    XFillRectangle(disp, panel, gc, 0, 0, attr.width, attr.height);
    XSetForeground(disp, gc, WhitePixel(disp, screen));
    XmbDrawString(disp, panel, fontset, gc,
                  attr.width - GetXPointFromRight(0) - XmbTextEscapement(fontset, time_str, strlen(time_str)),
                  16, time_str, strlen(time_str));
}

void RaisePanel() {
    XRaiseWindow(disp, panel);
}

int IsPanel(Window w) {
    return panel == w;
}
