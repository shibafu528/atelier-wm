#include <X11/Xlib.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include "atelier.h"
#include "window.h"
#include "panel.h"
#include "time.h"
#include "resource.h"

#define BUTTON_PIXMAP_WIDTH 22

typedef enum {
    BUTTON_QUIT,
    BUTTON_LAUNCHER,
    BUTTON_HOME,
    BUTTON_SIZE
} ButtonRes;

Window panel;

extern WindowList* windows;
extern WindowList* last_raised;
extern XFontSet fontset;
extern Boolean terminate;

static XColor bgcolor;
static XColor highlight_color;
static BitmapRes* buttons[BUTTON_SIZE];
static int clock_text_width;

static struct {
    WindowList** windows;
    int num;
    int item_width;
} last_switchable;

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
    Colormap cmap = DefaultColormap(disp, 0);
    bgcolor.red = bgcolor.green = bgcolor.blue = 8192;
    highlight_color.red = highlight_color.green = highlight_color.blue = 16384;
    XAllocColor(disp, cmap, &bgcolor);
    XAllocColor(disp, cmap, &highlight_color);
    
    panel = XCreateWindow(disp, root,
                          0, 0,
                          DisplayWidth(disp, 0), PANEL_HEIGHT,
                          0, CopyFromParent, CopyFromParent, CopyFromParent,
                          CWColormap | CWOverrideRedirect | CWBackPixel,
                          &(XSetWindowAttributes){.colormap = cmap,
                                  .override_redirect = True,
                                  .background_pixel = bgcolor.pixel});
    XSelectInput(disp, panel, ExposureMask | ButtonPressMask);
    XMapWindow(disp, panel);
    InitPanelResources();
}

void DestroyPanel() {
    FreePanelResources();
    if (last_switchable.windows != NULL) {
        FreeWindowsArray(&last_switchable.windows);
    }
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
    PrintCurrentTime(time_str, sizeof(time_str));
    
    clock_text_width = XmbTextEscapement(fontset, time_str, strlen(time_str));
    XSetClipRectangles(disp, gc, 0, 0, &(XRectangle){attr.width - GetXPoint(1) - 2 - clock_text_width, 0, clock_text_width, PANEL_HEIGHT}, 1, Unsorted);
    DrawPanelBackground(&attr);
    XSetForeground(disp, gc, WhitePixel(disp, screen));
    XmbDrawString(disp, panel, fontset, gc,
                  attr.width - GetXPoint(1) - 2 - clock_text_width,
                  18, time_str, strlen(time_str));
    XSetClipMask(disp, gc, None);
}

void DrawPanelSwitcher() {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, panel, &attr);

    int panel_width = attr.width;
    int left_margin = GetXPoint(1) + 2;
    int right_margin = GetXPoint(1) + clock_text_width + 2;
    XRectangle mask = {0, 0, panel_width - left_margin - right_margin, PANEL_HEIGHT};
    XSetClipRectangles(disp, gc, left_margin, 0, &mask, 1, Unsorted);
    DrawPanelBackground(&attr);
    XSetBackground(disp, gc, bgcolor.pixel);
    XSetForeground(disp, gc, WhitePixel(disp, screen));
    {
        if (last_switchable.windows != NULL) {
            FreeWindowsArray(&last_switchable.windows);
        }
        last_switchable.num = GetSwitchableWindows(windows, &last_switchable.windows);
        last_switchable.item_width = mask.width = Min(160, mask.width / DivSafe(last_switchable.num));
        for (int i = 0; i < last_switchable.num && last_switchable.windows[i] != NULL; i++) {
            char title[512];
            GetWindowTitle(last_switchable.windows[i]->window, title, sizeof(title));
            XSetClipRectangles(disp, gc, left_margin, 0, &mask, 1, Unsorted);
            if (last_switchable.windows[i] == last_raised) {
                XSetForeground(disp, gc, highlight_color.pixel);
                XFillRectangle(disp, panel, gc, left_margin, 0, last_switchable.item_width, PANEL_HEIGHT);
                XSetForeground(disp, gc, WhitePixel(disp, screen));
            }
            if (last_switchable.windows[i]->state == IconicState) {
                char title_cpy[512];
                strcpy(title_cpy, title);
                snprintf(title, sizeof(title), "[%s]", title_cpy);
            }
            XmbDrawString(disp, panel, fontset, gc, left_margin + 2, 18, title, strlen(title));
            left_margin += last_switchable.item_width + 2;
        }
    }
    XSetClipMask(disp, gc, None);
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
    DrawPanelSwitcher();
}

void RaisePanel() {
    XRaiseWindow(disp, panel);
}

int IsPanel(Window w) {
    return panel == w;
}

static inline Boolean OnRange(int start, int target, int end) {
    return start <= target && target <= end;
}

void OnClickPanel(XButtonEvent event) {
    XWindowAttributes attr;
    XGetWindowAttributes(disp, panel, &attr);
    if (OnRange(GetXPoint(0), event.x, GetXPoint(1))) {
        //Launcherボタン
        println("OnClickPanel: launcher");
        int pid = fork();
        if (pid == 0) {
            if (fork() == 0) {
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
                execlp(config.launcher_path, config.launcher_path, NULL);
                return;
            } else {
                exit(0);
            }
        } else {
            int status;
            waitpid(pid, &status, 0);
        }
    } else if (OnRange(attr.width - GetXPoint(1), event.x, attr.width - GetXPoint(0))) {
        //Terminateボタン
        println("OnClickPanel: terminate");
        terminate = TRUE;
    } else if (OnRange(GetXPoint(1) + 2, event.x, attr.width - GetXPoint(1) - clock_text_width)) {
        //タスクスイッチャー領域のクリックイベント
        int selected = (event.x - GetXPoint(1) - 2) / DivSafe(last_switchable.item_width);
        printf("OnClickPanel: selected %d\n", selected);
        if (selected < last_switchable.num) {
            WindowList *wl = last_switchable.windows[selected];
            if (wl == last_raised && wl->state != IconicState) {
                IconifyWindow(wl);
            } else {
                DeIconifyWindow(wl);
                RaiseWindow(wl);
                last_raised = wl;
            }
            DrawPanelSwitcher();
        }
    }
}
