#ifndef PANEL_H
#define PANEL_H

#include <X11/Xlib.h>

#define PANEL_HEIGHT 22

void InitPanel();
void DestroyPanel();
void ShowPanel();
void HidePanel();
void DrawPanelClock();
void DrawPanelSwitcher();
void DrawPanel();
void RaisePanel();
int IsPanel(Window w);
void OnClickPanel(XButtonEvent event);

#endif
