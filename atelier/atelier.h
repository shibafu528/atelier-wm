#ifndef ATELIER_H
#define ATELIER_H

#define Boolean int
#define uint unsigned int
#define Screen int
#define TRUE 1
#define FALSE 0

extern Display *disp;
extern Screen screen;
extern Window root;
extern GC gc;

static inline int Max(int a, int b) {
    return a > b ? a : b;
}

static inline int Min(int a, int b) {
    return a > b ? b : a;
}

static inline void println(const char* line) {
    printf(line);
    printf("\n");
}

#endif
