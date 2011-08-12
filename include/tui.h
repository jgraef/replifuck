/* include/tui.h - TUI for replifuck
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


#ifndef _TUI_H_
#define _TUI_H_

#include <glib.h>
#include <ncurses.h>
#include <stdio.h>

#include "replifuck.h"


typedef struct tui_S tui_t;


struct tui_S {
  WINDOW *win_main;
  WINDOW *win_vm;
  WINDOW *win_mem;
  WINDOW *win_th;

  gboolean running;
  gboolean autostep;
  unsigned int cycles_per_frame;

  rfvm_t *vm;
  int current_thread;
  rfp_t mem_view;
  rfp_t mem_p;
  struct rfth_page_cache page_cache;
};


int tui_init(tui_t *tui, rfvm_t *vm);
void tui_finalize(tui_t *tui);
void tui_win_main(tui_t *tui);
void tui_memview_goto(tui_t *tui, rfp_t p);
gboolean tui_really_quit(tui_t *tui);
void tui_show_help(tui_t *tui);
void tui_main(tui_t *tui);

#endif /* _TUI_H_ */

