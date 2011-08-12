/* main.c - replifuck main program
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

#include <glib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "replifuck.h"
#include "tui.h"


#define INITIAL_POPULATION_SIZE 4


int main(int argc, char *argv[]) {
  tui_t tui;
  rfvm_t *vm;
  const char *path;
  unsigned int length, i;
  rfp_t p;

  /* get program path */
  if (argc>=2) {
    path = argv[1];
  }
  else {
    printf("Usage: %s FILE\n", argv[0]);
    return 1;
  }

  /* brainfuck */
  vm = rf_vm_new();

  for (i=0; i<INITIAL_POPULATION_SIZE; i++) {
    p = rf_rand_p(vm, 0, 2*RFMEM_PAGE_SIZE);
    length = rf_load_program(vm, path, p);
    rf_thread_add_full(vm, p, p, p, NULL, 0);
    printf("%s: at %ld, %u words\n", path, p, length);
  }

  /* TUI */
  tui_init(&tui, vm);
  tui_main(&tui);

  /* shutdown */
  rf_vm_free(vm);
  tui_finalize(&tui);

  return 0;
}


