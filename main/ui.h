#ifndef UI_H
#define UI_H

#include "lvgl.h"

extern const char *btnmMapDefault[];

void keyEventCb(lv_event_t *e);
void makeGrid();

#endif