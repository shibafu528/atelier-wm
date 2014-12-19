#include <X11/Xlib.h>
#include "atelier.h"

// あとでヘッダに移す
void InitPanel();
void ShowPanel();
void HidePanel();
void DrawPanel();

#define PANEL_HEIGHT 22

Window panel;
XColor bgcolor;

void InitPanel() {
    Colormap cmap;
    XSetWindowAttributes attr;
    cmap = DefaultColormap(disp, 0);
    bgcolor.red = bgcolor.green = bgcolor.blue = 8192;
    XAllocColor(disp, cmap, &bgcolor);

    attr.colormap = cmap;
    attr.override_redirect = true;
    attr.background_pixel = bgcolor.pixel;
    
    panel = XCreateWindow(disp, root,
                          0, 0,
                          DisplayWidth(disp, 0), PANEL_HEIGHT,
                          0, CopyFromParent, CopyFromParent, CopyFromParent,
                          CWColormap | CWOverrideRedirect | CWBackPixel,
                          &attr);
    XMapWindow(disp, panel);
}
