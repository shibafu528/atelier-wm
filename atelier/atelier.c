#include <X11/Xlib.h>
#include <X11/Xlocale.h>
#include <X11/cursorfont.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "atelier.h"
#include "resource.h"
#include "window.h"
#include "panel.h"

Display *disp;
Screen screen;
Window root;
GC gc;
Boolean terminate = FALSE;

extern XFontSet fontset;
extern WindowList *last_raised;

void InitEventHandler();
void CallEventHandler(XEvent event);

static Boolean SetSignal(int signame, void (*sighandle)(int signum)) {
    return signal(signame, sighandle) == SIG_ERR ? FALSE : TRUE;
}

static void QuitHandler(int signum) {
    fprintf(stderr, "term! :%d\n", signum);
    terminate = TRUE;
}

static void CaptureExistWindows() {
    Window r, p;
    Window* children;
    uint children_num;
    XQueryTree(disp, root, &r, &p, &children, &children_num);
    for (uint i = 0; i < children_num; i++) {
        XWindowAttributes attr;
        XGetWindowAttributes(disp, children[i], &attr);
        if (!attr.override_redirect) {
            CatchWindow(children[i]);
            last_raised = FindFrame(children[i]);
        }
    }
    if (children != NULL) {
        XFree(children);
    }
}

static int InitFontSet() {
    int missing_count;
    char** missing_list;
    char* def_string;
    fontset = XCreateFontSet(disp,
                             "-*-fixed-medium-r-normal--16-*-*-*",
                             &missing_list,
                             &missing_count,
                             &def_string);

    if (fontset == NULL) {
        return FALSE;
    }

    XFreeStringList(missing_list);
    return TRUE;
}

int main(int argc, char* argv[]) {
    disp = XOpenDisplay(NULL);
    root = DefaultRootWindow(disp);

    //ロケール周りの初期化
    if (setlocale(LC_CTYPE, "") == NULL) {
        return 1;
    }
    if (!XSupportsLocale()) {
        return 1;
    }

    //既知のウィンドウの取得
    CaptureExistWindows();

    //GCの生成
    gc = XCreateGC(disp, root, 0, NULL);
    
    //rootウィンドウから情報を取得できるようにする
    XSelectInput(disp, root, SubstructureRedirectMask | SubstructureNotifyMask);

    //フォントの取得
    if (!InitFontSet()) {
        return 1;
    }

    //コンフィグの読み込み
    LoadConfig();

    //イベントハンドラの初期化
    InitEventHandler();

    //パネルの初期化
    InitPanel();
    
    //ウィンドウ切り替えのパッシブグラブ
    KeyCode tabKey = XKeysymToKeycode(disp, XStringToKeysym("Tab"));
    XGrabKey(disp, tabKey, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(disp, tabKey, Mod1Mask | ShiftMask, root, True, GrabModeAsync, GrabModeAsync);

    //シグナルをキャッチする
    SetSignal(SIGINT, QuitHandler);
    SetSignal(SIGQUIT, QuitHandler);
    SetSignal(SIGTERM, QuitHandler);

    //カーソルの設定
    Cursor normal_cursor = XCreateFontCursor(disp, XC_left_ptr);
    XDefineCursor(disp, root, normal_cursor);

    //ここまでのリクエストを全てフラッシュする
    XSync(disp, False);

    //メインループ
    XEvent event;
    while (!terminate) {
        if (XPending(disp)) {
            XNextEvent(disp, &event);
            CallEventHandler(event);
            XSync(disp, False);
        } else {
            DrawPanelClock();
            XSync(disp, False);
            nanosleep(&(struct timespec){0, 10000000}, NULL);
        }
    }

    println("Quitting Atelier...");

    //GCとカーソルの解放
    XFreeGC(disp, gc);
    XUndefineCursor(disp, root);

    //管理しているウィンドウをすべて解放する
    ReleaseAllWindows();
    println("[OK] Released Windows");
    //パネルを始末する
    DestroyPanel();
    println("[OK] Released Panel");
    //ディスプレイとの接続を切る
    XCloseDisplay(disp);
    println("[OK] Closed Display Connection");
    println("[OK] Reached Quit Atelier");
    return 0;
} 
