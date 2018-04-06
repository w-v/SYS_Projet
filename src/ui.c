#include <ui.h>
#include <guestutils.h>
#include <ncurses.h>
#include <stdint.h>


void get_input(){
  int ch;
  switch (ch = getch()){
    case KEY_LEFT:
      usettings.cursor--;
      break;
    case KEY_RIGHT:
      usettings.cursor++;
      break;
    case KEY_UP:
      update_settings(+1, usettings.cursor);
      break;
    case KEY_DOWN:
      update_settings(-1, usettings.cursor);
      break;
    case '+':
      update_settings(+1, CURS_VOL);
      break;
    case '-':
      update_settings(-1, CURS_VOL);
      break;
    case 'b':
      usettings.eq_on = !usettings.eq_on;
      break;
    case 'e':
      usettings.eq_ui = !usettings.eq_ui;
      break;
    case 'v':
      usettings.vol_ui = !usettings.vol_ui;
      break;
    case 'q':
      clean_exit();
      break;
    default:
      break;
  }
  // keep the cursor from leaving the ui
  if(usettings.cursor < 0)
    usettings.cursor = 0;
  else if(usettings.cursor > CURS_EQ+N_FILTERS)
    usettings.cursor = CURS_EQ+N_FILTERS;
}

void update_settings(int d, int c){
  switch(c){
    case CURS_VOL:
      usettings.vol+=VOL_STEP*d;
      if (usettings.vol > MAX_VOL)
        usettings.vol = MAX_VOL;
      else if (usettings.vol < -MAX_VOL)
        usettings.vol = -MAX_VOL;
      break;
    default:
      usettings.eq_gains[c-CURS_EQ] += (EQ_MAX_GAIN*2/EQ_UI_H)*d;
      if (usettings.eq_gains[c-CURS_EQ] > EQ_MAX_GAIN)
        usettings.eq_gains[c-CURS_EQ] = EQ_MAX_GAIN;
      else if (usettings.eq_gains[c-CURS_EQ] < -EQ_MAX_GAIN)
        usettings.eq_gains[c-CURS_EQ] = -EQ_MAX_GAIN;
  }
}

void draw_controls(){
  int w, h, y, x;
  getmaxyx(stdscr, h, w);
  x = 4;
  mvprintw(h-2,x,"EQ ON/OFF: B(ypass)\tSHOW EQ UI: E(Q)\tSHOW VOL UI: V(olume)\tSET VOL: +/-\n");
  mvprintw(h-1,x,"NAVIGATE UI: LEFT/RIGHT\tSET GAINS: UP/DOWN\tEXIT: Q(uit)\n");
  
}

void draw_ui(){

  int w, h, y, x;
  getmaxyx(stdscr, h, w);
  w-=WIN_PADDING;
  h-=WIN_PADDING;
  x = w;
  y = WIN_PADDING;
  const char *scaley[5] = { "+20dB ", "+10dB ", "  0dB ", "-10dB ", "-20dB " };
  const char *scalex[N_FILTERS] = {" 30 ", " 65 ", "125 ", "250 ","500 ", " 1k ", " 2k ", " 4k ", " 8k ", "16k "};
  const char * onoff[2] = {"EQ:OFF", "EQ:ON"};
  int bar_h;  
  int bar_w = EQ_UI_W/N_FILTERS;
  int f,a,c;
  char ch;
  float d = EQ_UI_H/(EQ_MAX_GAIN*2.f);
  char eq_blanks[EQ_UI_W+EQ_SCALE_W+UI_PADDING+1];
  char vol_blanks[VOL_UI_W+UI_PADDING+1];
  // TODO : clear whats under
  if(usettings.eq_ui){
    x-= EQ_UI_W+EQ_SCALE_W;
    memset(&eq_blanks, ' ', sizeof(eq_blanks));
    eq_blanks[EQ_UI_W+EQ_SCALE_W+UI_PADDING] = '\0';
    for(f = 0; f < EQ_UI_H+UI_PADDING+WIN_PADDING; f++)
      mvprintw(y+f,x-1,"%s",eq_blanks);
    for(int i = 0; i < 5; i++)
      mvprintw(y+EQ_UI_H*i/4.f,x,"%s",scaley[i]);
    x+=EQ_SCALE_W;
    for(int i = 0; i < N_FILTERS; i++){
      mvprintw(y+EQ_UI_H+1,x+i*4,"%s",scalex[i]);
    }
    mvprintw(EQ_UI_H+UI_PADDING+WIN_PADDING-1,x-5,onoff[usettings.eq_on]);
    for(f = 0; f < N_FILTERS; f++){
      bar_h = ( EQ_MAX_GAIN+usettings.eq_gains[f] )*d;

      for(a = 0; a < bar_h+1; a++){

        if(usettings.cursor == CURS_EQ+f)
          ch = 'O';
        else
          ch = 'H';

        for(c = 0; c < bar_w; c++){
          mvaddch(y+EQ_UI_H-a,x+f*bar_w+c,ch);
        }

      }

    }
    x-= EQ_SCALE_W;
  } 
  if(usettings.vol_ui){
    x-= VOL_UI_W+UI_PADDING;
    memset(&vol_blanks, ' ', sizeof(vol_blanks));
    vol_blanks[VOL_UI_W+UI_PADDING] = '\0';
    for(f = 0; f < VOL_UI_H+3; f++)
      mvprintw(y+f,x-1,"%s",vol_blanks);
    mvprintw(y+VOL_UI_H+1,x,"VOL");
    d = ( VOL_UI_H/(MAX_VOL*2.f) );
    bar_h = ( MAX_VOL + usettings.vol ) * d;
    for(a = 0; a < bar_h; a++){
      if(a < bar_h+1){
        if(usettings.cursor == CURS_VOL)
          ch = 'O';
        else
          ch = 'H';
      }
      else {
        ch = ' ';
      }
      for(c = 0; c < VOL_UI_W; c++){
        mvaddch(y+VOL_UI_H-a,x+c,ch);
      }
    }

  }

}

