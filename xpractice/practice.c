#include <stdio.h>
#include <X11/Xlib.h>

#define Screen int
#define TRUE 1
#define FALSE 0

void DrawFrame(Display *disp, Screen screen, Window window, GC gc) {
    XWindowAttributes attr;

    XGetWindowAttributes(disp, window, &attr);
    
    XSetForeground(disp, gc, BlackPixel(disp, screen));
    XDrawRectangle(disp, window, gc, 0, 0, attr.width, attr.height);
    XFillRectangle(disp, window, gc, 0, 0, attr.width, 22);
}

int main(int argc, char* argv[]) {
    Display *disp;
    Screen screen;
    Window root;
    Window window;
    XEvent event;
    Atom atom;
    GC gc;
    int loop = TRUE;
    
    disp = XOpenDisplay(NULL);
    screen = DefaultScreen(disp);
    root = DefaultRootWindow(disp);

    window = XCreateSimpleWindow(disp, root, 0, 0, 400, 300, 1, BlackPixel(disp, screen), WhitePixel(disp, screen));
    atom = XInternAtom(disp, "WM_DELETE_WINDOW", TRUE);
    gc = XCreateGC(disp, window, 0, NULL);

    XSetWMProtocols(disp, window, &atom, 1);
    XSelectInput(disp, window, ExposureMask);
    XMapWindow(disp, window);

    while(loop) {
        while (XPending(disp)) {
            XNextEvent(disp, &event);
            
            switch(event.type) {
            case Expose:
                if (event.xexpose.count == 0) {
                    DrawFrame(disp, screen, window, gc);
                }
                break;
            case ClientMessage:
                loop = 0;
                break;
            }
        }
    }

    XFreeGC(disp, gc);
    XDestroyWindow(disp, window);
    XCloseDisplay(disp);
}
