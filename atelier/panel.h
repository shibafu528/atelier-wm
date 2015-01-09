#ifndef PANEL_H
#define PANEL_H

#include <X11/Xlib.h>

#define PANEL_HEIGHT 22

void InitPanel();
void ShowPanel();
void HidePanel();
void DrawPanel();
void RaisePanel();
int IsPanel(Window w);

#endif
