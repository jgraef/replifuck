/* include/replifuck.h - replifuck virtual machine
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
 *
 * In fact this is a very special brainfuck:
 *  1) Multithreading: This brainfuck VM is able to execute programs in parallel
 *  2) Von-Neumann model: Code and data is put into same memory
 *  3) Stack: Threads have a stack pointer and data can be pushed ('^') or popped ('V')
 *  3) Memory is simulated to be endless
 *
 * Futher notes:
 *  - Since memory is endless, the command '*' is used as termination command.
 *  - Memory is initialized with random values, not 0
 *
 * TODO: manage threads in a list and supply previous/next
 * TODO: a Gtk GUI with more advanced widgest would be neat (e.g. population graph, signature scanning, 2D memory view)
 * TODO: unused pages should timeout, so that pages can be freed
 */

#ifndef _REPLIFUCK_H_
#define _REPLIFUCK_H_

#include <glib.h>
#include <gsl/gsl_rng.h>


/* Macro definitions */

/* size of allocated memory page */
#define RFMEM_PAGE_SIZE 0x1000

/* Magic string for dump files */
#define RFMEM_DUMP_MAGIC "!reprfuck memdump\n"
#define RFMEM_DUMP_MAGIC_LENGTH 18

/* Max. number of threads 
 * NOTE: Undefine for unlimited number
 */
#define RFVM_MAX_THREADS 512

/* Max. number of cycles per thread */
#define RFTH_MAX_CYCLES   50000


/* Data types */

/* Basic word (usally this is a byte */
typedef char rfword_t;

/* brainfuck pointers (long should be enough, since memory is limited, at least for my computer) */
typedef long rfp_t;

/* brainfuck size (signed version of pointer: unsigned long) */
typedef unsigned long rfsz_t;

/* Type of brainfuck VM */
typedef struct rfvm rfvm_t;

/* Type of brainfuck thread */
typedef struct rfth rfth_t;


/* Data structures */

/* Virtual machine */
struct rfvm {
  /* Random number generator */
  gsl_rng *rand;

  /* Memory tree (code & data) */
  GTree *memory;

  /* All execution threads */
  GPtrArray *threads; /* with rfth_t* */

  /* Clock (how many cycles this VM has done) */
  unsigned int clock;

  /* Error rate & number of occured mutations */
  struct {
    double rate_instr;
    double rate_mem;
    double rate_kill;
    unsigned int num_instr;
    unsigned int num_mem;
    unsigned int num_kill;
  } mutations;
};

/* Execution thread */
struct rfth_page_cache {
  int pid;
  rfword_t *page;
};
struct rfth {
  /* Instruction pointer */
  rfp_t ip;

  /* Data pointer */
  rfp_t dp;

  /* Stack pointer */
  rfp_t sp;

  /* Clock (how many cycles this thread has done) */
  unsigned int clock;

  /* Stack used for parentheses matching */
  GSList *pstack;

  /* Page lookup cache */
  struct rfth_page_cache page_cache_ip;
  struct rfth_page_cache page_cache_dp;
  struct rfth_page_cache page_cache_sp;
};



/* Function prototypes */

rfp_t rf_rand_p(rfvm_t *vm, rfp_t mu, rfsz_t sigma);
rfvm_t *rf_vm_new(void);
rfvm_t *rf_vm_new_with_seed(unsigned long seed);
void rf_vm_free(rfvm_t *vm);
void rf_vm_set_debug(rfvm_t *vm, gboolean on_off);
rfth_t *rf_thread_add_full(rfvm_t *vm, rfp_t ip, rfp_t dp, rfp_t sp, const rfword_t *code, int code_length);
rfth_t *rf_thread_add(rfvm_t *vm);
void rf_thread_remove(rfvm_t *vm, rfth_t *thread);
gboolean rf_thread_cycle(rfvm_t *vm, rfth_t *thread);
void rf_vm_cycle(rfvm_t *vm);
void rf_memory_mutate(rfvm_t *vm);
rfword_t rf_memory_read(rfvm_t *vm, rfp_t p, struct rfth_page_cache *page_cache);
void rf_memory_write(rfvm_t *vm, rfp_t p, rfword_t data, struct rfth_page_cache *page_cache);
rfsz_t rf_memory_load_data(rfvm_t *vm, rfp_t p, const rfword_t *data, rfsz_t n);
unsigned int rf_get_num_threads(rfvm_t *vm);
rfth_t *rf_get_thread(rfvm_t *vm, unsigned int i);
unsigned int rf_get_memory_usage(rfvm_t *vm);
rfsz_t rf_load_data(rfvm_t *vm, rfp_t p, const rfword_t *data, rfsz_t n);
int rf_load_program(rfvm_t *vm, const char *filename, rfp_t p);
gboolean rf_vm_store(rfvm_t *vm, const char *filename);


#endif /* _rf_H_ */

