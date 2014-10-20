#include <stdio.h>
#include <X11/Xlib.h>

#define Screen int
#define TRUE 1
#define FALSE 0

int main(int argc, char* argv[]) {
    Display *disp;
    Screen screen;
    Window root;
    Window window;
    XEvent event;
    Atom atom;
    int loop = TRUE;
    
    disp = XOpenDisplay(NULL);
    screen = DefaultScreen(disp);
    root = DefaultRootWindow(disp);

    window = XCreateSimpleWindow(disp, root, 0, 0, 400, 300, 1, BlackPixel(disp, screen), WhitePixel(disp, screen));
    atom = XInternAtom(disp, "WM_DELETE_WINDOW", TRUE);

    XSetWMProtocols(disp, window, &atom, 1);
    //XSelectInput(disp, window, KeyPressMask);
    XMapWindow(disp, window);

    while(loop) {
        XNextEvent(disp, &event);
        
        switch(event.type) {
        case ClientMessage:
            loop = 0;
            break;
        }
    }

    XDestroyWindow(disp, window);
    XCloseDisplay(disp);
}
