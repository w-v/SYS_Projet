#ifndef UI_H
#define UI_H

#include <volume.h>
#include <equaliser.h>

#define CURS_VOL         0
#define CURS_EQ          1

#define EQ_UI_W         40
#define EQ_UI_H         20
#define EQ_SCALE_W      6
#define UI_PADDING      2
#define WIN_PADDING     1

#define VOL_UI_W         3
#define VOL_UI_H         20

void get_input();

void draw_ui();

void draw_controls();

void update_settings(int d, int c);

void clip();

#endif
