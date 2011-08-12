/* tui.c - TUI for replifuck
 * Copyright (C) 2011 by Janosch Gräf <janosch.graef@gmx.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/* TODO do not address threads by indexing!
 *      use a random ID as name for threads. next/prev should do the rest
 */

#include <ncurses.h>
#include <glib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#include "tui.h"
#include "replifuck.h"


#define TUI_CHAR_NOT_PRITABLE '.'
#define TUI_CHAR_PRINT(c) (isprint(c)?(c):TUI_CHAR_NOT_PRITABLE)


int tui_init(tui_t *tui, rfvm_t *vm) {
  rfth_t *thread;

  memset(tui, 0, sizeof(tui_t));

  tui->vm = vm;
  tui->cycles_per_frame = 1;
  thread = rf_get_thread(vm, tui->current_thread);
  if (thread!=NULL) {
    tui_memview_goto(tui, thread->ip);
  }

  tui->win_main = initscr();
  start_color();
  curs_set(0);
  noecho();
  nodelay(tui->win_main, TRUE);
  cbreak();
  keypad(tui->win_main, TRUE);
  init_pair(1, COLOR_YELLOW, COLOR_BLUE); 
  init_pair(2, COLOR_BLUE, COLOR_WHITE);

  color_set(2, NULL);
  attrset(A_BOLD);

  tui->win_vm = subwin(tui->win_main, 10, 38, 1, 1);
  tui->win_th = subwin(tui->win_main, 10, 39, 1, 40);
  tui->win_mem = subwin(tui->win_main, 10, 78, 12, 1);

  return 0;
}


void tui_finalize(tui_t *tui) {
  endwin();
}


void tui_win_main(tui_t *tui) {
  rfvm_t *vm = tui->vm;
  const char *title = "[replfuck]";
  unsigned int num_threads;
  int i;
  double memory_usage;
  rfth_t *thread = NULL;
  rfword_t b;

  clear();
  wbkgd(tui->win_main, COLOR_PAIR(1));
  box(tui->win_main, 0, 0);
  wbkgd(tui->win_vm, COLOR_PAIR(2));
  box(tui->win_vm, 0, 0);
  wbkgd(tui->win_th, COLOR_PAIR(2));
  box(tui->win_th, 0, 0);
  wbkgd(tui->win_mem, COLOR_PAIR(2));
  box(tui->win_mem, 0, 0);

  mvwaddstr(tui->win_main, 0, (80-strlen(title))/2, title);
  mvwprintw(tui->win_main, 22, 1, "%d cycles/frame", tui->cycles_per_frame);
  mvwprintw(tui->win_main, 22, 73, "[H]elp", tui->cycles_per_frame);

  num_threads = rf_get_num_threads(vm);
  memory_usage = ((double)rf_get_memory_usage(vm)/1024.0);

  mvwaddstr(tui->win_vm, 0, 1, "[Virtual Machine]");
  mvwprintw(tui->win_vm, 1, 2, "Clock:       %u", vm->clock);
  mvwprintw(tui->win_vm, 2, 2, "Threads:     %u", num_threads);
  mvwprintw(tui->win_vm, 3, 2, "Memory:      %.2f kB", memory_usage);
//  mvwprintw(tui->win_vm, 4, 2, "Error rate:  %.4f%%, %.4f%%, %.4f%%", 100.0*vm->mutations.rate_instr, 100.0*vm->mutations.rate_mem, 100.0*vm->mutations.rate_kill);
  mvwprintw(tui->win_vm, 4, 2, "Errors:      %u, %u, %u", vm->mutations.num_instr, vm->mutations.num_mem, vm->mutations.num_kill);

  if (num_threads==0) {
    mvwaddstr(tui->win_th, 0, 1, "[Thread (none)]");
  }
  else {
    if (tui->current_thread>=num_threads) {
      tui->current_thread = num_threads-1;
    }
    else if (tui->current_thread<0) {
      tui->current_thread = 0;
    }

    thread = rf_get_thread(vm, tui->current_thread);
    mvwprintw(tui->win_th, 0, 1, "[Thread %d/%d]", tui->current_thread+1, num_threads);
    mvwprintw(tui->win_th, 1, 2, "Clock:  %u", thread->clock);
    b = rf_memory_read(vm, thread->ip, NULL);
    mvwprintw(tui->win_th, 2, 2, "IP:     %d - %02X '%c'", thread->ip, b&0xFF, TUI_CHAR_PRINT(b));
    b = rf_memory_read(vm, thread->dp, NULL);
    mvwprintw(tui->win_th, 3, 2, "DP:     %d - %02X '%c'", thread->dp, b&0xFF, TUI_CHAR_PRINT(b));
    b = rf_memory_read(vm, thread->sp, NULL);
    mvwprintw(tui->win_th, 4, 2, "SP:     %d - %02X '%c'", thread->sp, b&0xFF, TUI_CHAR_PRINT(b));
    mvwprintw(tui->win_th, 5, 2, "IP PC:  %d -> %p", thread->page_cache_ip.pid, thread->page_cache_ip.page);
    mvwprintw(tui->win_th, 6, 2, "DP PC:  %d -> %p", thread->page_cache_dp.pid, thread->page_cache_dp.page);
    mvwprintw(tui->win_th, 7, 2, "SP PC:  %d -> %p", thread->page_cache_sp.pid, thread->page_cache_sp.page);
  }

  b = rf_memory_read(vm, tui->mem_p, NULL);
  mvwaddstr(tui->win_mem, 0, 1, "[Memory]");
  mvwprintw(tui->win_mem, 1, 2, "View: %d - %d", tui->mem_view, tui->mem_view+76);
  mvwprintw(tui->win_mem, 2, 2, "Pos:  %d - %02X '%c'", tui->mem_p, b&0xFF, TUI_CHAR_PRINT(b));
  mvwhline(tui->win_mem, 4, 1, ACS_HLINE, 76);
  mvwhline(tui->win_mem, 6, 1, ACS_HLINE, 76);
  mvwhline(tui->win_mem, 7, 1, ACS_HLINE, 76);
  mvwhline(tui->win_mem, 8, 1, ACS_HLINE, 76);

  i = tui->mem_p-tui->mem_view;
  if (i<0) {
    mvwaddch(tui->win_mem, 4, 1, '<');
  }
  else if (i>=76) {
    mvwaddch(tui->win_mem, 4, 76, '>');
  }
  else {
    mvwaddch(tui->win_mem, 4, i+1, 'V');
  }
  mvwaddstr(tui->win_mem, 4, i>35?1:77-8, "[SELECT]");

  if (thread!=NULL) {
    i = thread->ip-tui->mem_view;
    if (i<0) {
    mvwaddch(tui->win_mem, 6, 1, '<');
    }
    else if (i>=76) {
      mvwaddch(tui->win_mem, 6, 76, '>');
    }
    else {
      mvwaddch(tui->win_mem, 6, i+1, ACS_UARROW);
    }
    mvwaddstr(tui->win_mem, 6, i>35?1:77-4, "[IP]");

    i = thread->dp-tui->mem_view;
    if (i<0) {
      mvwaddch(tui->win_mem, 7, 1, '<');
    }
    else if (i>=76) {
      mvwaddch(tui->win_mem, 7, 76, '>');
    }
    else {
      mvwaddch(tui->win_mem, 7, i+1, ACS_UARROW);
    }
    mvwaddstr(tui->win_mem, 7, i>35?1:77-4, "[DP]");

    i = thread->sp-tui->mem_view;
    if (i<0) {
      mvwaddch(tui->win_mem, 8, 1, '<');
    }
    else if (i>=76) {
      mvwaddch(tui->win_mem, 8, 76, '>');
    }
    else {
      mvwaddch(tui->win_mem, 8, i+1, ACS_UARROW);
    }
    mvwaddstr(tui->win_mem, 8, i>35?1:77-4, "[SP]");

  }

  for (i=0; i<76; i++) {
    b = rf_memory_read(vm, tui->mem_view+i, NULL);
    if (isprint(b)) {
      mvwaddch(tui->win_mem, 5, i+1, b);
    }
    else {
      mvwaddch(tui->win_mem, 5, i+1, '.');
    }
  }

  refresh();
}


void tui_memview_goto(tui_t *tui, rfp_t p) {
  tui->mem_view = p-(0x10-(p%0x10));
  tui->mem_p = p;
}


gboolean tui_really_quit(tui_t *tui) {
  int key;

  clear();
  wbkgd(tui->win_main, COLOR_PAIR(1));
  box(tui->win_main, 0, 0);
  mvwaddstr(tui->win_main, 10, 5, "Really quit [y/N]?");
  refresh();

  nodelay(tui->win_main, FALSE);
  key = getch();
  nodelay(tui->win_main, TRUE);

  return key=='y';
}


void tui_show_help(tui_t *tui) {
  const char *help[] = {
    "F1 or H          Show help",
    "ESC or Q         Quit program",
    "UP/DOWN          Move memory view +/- 16 words",
    "PAGEUP/PAGEDOWN  Move memory view +/- one page (4096 words)",
    "LEFT/RIGHT       Move memory selection +/- one word",
    "BACKSPACE        Reset memory view and selection",
    "+/-              Increment/Decrement selected word",
    ",/.              Select next/previous thread",
    "SPACE            Do one cycle",
    "ENTER            Activate/Deactive autorun",
    "d                Dump VM state",
    "F5               Move memory view to selected thread's IP",
    "F6               Move memory view to selected thread's DP",
    "F7               Move memory view to selected thread's SP",
    NULL
  };
  unsigned int i;

  clear();
  wbkgd(tui->win_main, COLOR_PAIR(1));
  box(tui->win_main, 0, 0);

  mvwaddstr(tui->win_main, 0, 5, "[HELP]");

  for (i=0; help[i]!=NULL; i++) {
    mvwaddstr(tui->win_main, 2+i, 2, help[i]);
  }

  refresh();

  nodelay(tui->win_main, FALSE);
  getch();
  nodelay(tui->win_main, TRUE);
}


void tui_main(tui_t *tui) {
  rfvm_t *vm = tui->vm;
  rfth_t *thread;
  int key, i;
  GDateTime *datetime;
  char *filename;
  rfword_t tmp;

  tui->running = TRUE;

  while (tui->running) {
    if (tui->autostep) {
      for (i=0; i<tui->cycles_per_frame; i++) {
        rf_vm_cycle(vm);
      }
    }

    tui_win_main(tui);

    key = getch();
    switch (key) {
      case 'q':
      case 27:
        tui->running = !tui_really_quit(tui);
        break;
      case KEY_UP:
        tui->mem_view += 0x10;
        break;
      case KEY_DOWN:
        tui->mem_view -= 0x10;
        break;
      case KEY_PPAGE:
        tui->mem_view -= RFMEM_PAGE_SIZE;
        break;
      case KEY_NPAGE:
        tui->mem_view += RFMEM_PAGE_SIZE;
        break;
      case KEY_LEFT:
        tui->mem_p--;
        break;
      case KEY_RIGHT:
        tui->mem_p++;
        break;
      case '+':
        tmp = rf_memory_read(vm, tui->mem_p, &tui->page_cache);
        rf_memory_write(vm, tui->mem_p, tmp+1, &tui->page_cache);
        break;
      case '-':
        tmp = rf_memory_read(vm, tui->mem_p, &tui->page_cache);
        rf_memory_write(vm, tui->mem_p, tmp-1, &tui->page_cache);
        break;
      case ',':
        tui->current_thread--;
        break;
      case '.':
        tui->current_thread++;
        break;
      case KEY_BACKSPACE:
      case '\b':
        tui->mem_view = 0;
        tui->mem_p = 0;
        break;
      case ' ':
        if (!tui->autostep) {
          rf_vm_cycle(vm);
        }
        break;
      case '\n':
        tui->autostep = !tui->autostep;
        break;
      case KEY_F(8):
        datetime = g_date_time_new_now_local();
        filename = g_date_time_format(datetime, "vmstates/%a %b %e %H:%M:%S %Y.RFm");
        if (!rf_vm_store(vm, filename)) {
          beep();
        }
        flash();
        g_date_time_unref(datetime);
        g_free(filename);
        break;
      case KEY_F(5):
        thread = rf_get_thread(vm, tui->current_thread);
        if (thread!=NULL) {
          tui_memview_goto(tui, thread->ip);
        }
        break;
      case KEY_F(6):
        thread = rf_get_thread(vm, tui->current_thread);
        if (thread!=NULL) {
          tui_memview_goto(tui, thread->dp);
        }
        break;
      case KEY_F(7):
        thread = rf_get_thread(vm, tui->current_thread);
        if (thread!=NULL) {
          tui_memview_goto(tui, thread->sp);
        }
        break;
      case KEY_F(3):
        if (tui->cycles_per_frame>1) {
          tui->cycles_per_frame /= 2;
        }
        break;
      case KEY_F(4):
        tui->cycles_per_frame *= 2;
        break;
      case KEY_F(1):
      case 'h':
        tui_show_help(tui);
        break;
      case -1:
        usleep(50000);
        break;
    }
  }
}

