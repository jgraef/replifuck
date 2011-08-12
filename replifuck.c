/* replifuck.c - replifuck virtual machine
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
#include <stdio.h>
#include <string.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include "replifuck.h"



/* Iterator: Iterated over memory tree to compare page IDs */
static int rf_memory_pid_compare(const void *a, const void *b, void *userdata) {
  return GPOINTER_TO_INT(a)-GPOINTER_TO_INT(b);
}


/* Iterator: Iterates over memory tree and stops if index reached */
struct bt_memory_get_page_indexed_iterargs {
  unsigned int idx;
  int pid;
  rfword_t *page;
};
static gboolean bt_memory_get_page_indexed_iter(void *key, void *value, void *userdata) {
  struct bt_memory_get_page_indexed_iterargs *args = (struct bt_memory_get_page_indexed_iterargs*)userdata;

  if (args->idx>0) {
    args->idx--;

    return FALSE;
  }
  else {
    args->pid = GPOINTER_TO_INT(key);
    args->page = (rfword_t*)value;

    return TRUE;
  }
}


/* Randomize data */
static void rf_rand(rfvm_t *vm, void *data, unsigned int n) {
  unsigned int i;
  unsigned long tmp;
  gint32 *data32 = (gint32*)data;
  gint8 *data8 = (gint8*)data;

  for (i=0; i<n/4; i++) {
    data32[i] = (gint32)gsl_rng_get(vm->rand);
  }

  tmp = gsl_rng_get(vm->rand);
  for (i=0; i<n%4; i++) {
    data8[n-i-1] = (tmp>>(8*i))&0xFF;
  }
}


/* Returns TRUE if an mutation occurs */
static gboolean rf_rand_mutation(rfvm_t *vm, double rate) {
  return gsl_ran_binomial(vm->rand, rate, 1);
}


/* Return random position (gauss distribution) */
rfp_t rf_rand_p(rfvm_t *vm, rfp_t mu, rfsz_t sigma) {
  return ((rfp_t)gsl_ran_gaussian(vm->rand, (double)sigma))+mu;
}


/* Find matching paretheses */
static rfp_t rf_find_matching_parentheses(rfvm_t *vm, rfp_t p, struct rfth_page_cache *page_cache) {
  unsigned int level = 1;

  for (p++; level>0; p++) {
    switch (rf_memory_read(vm, p, page_cache)) {
      case '[':
        level++;
        break;
      case ']':
        level--;
        break;
    }
  }

  return p-1;
}




/* Create new VM */
rfvm_t *rf_vm_new_full(unsigned long seed, float rate_instr, float rate_mem, float rate_kill) {
  rfvm_t *vm;

  vm = (rfvm_t*)g_malloc(sizeof(rfvm_t));
  memset(vm, 0, sizeof(rfvm_t));
  vm->memory = g_tree_new_full(rf_memory_pid_compare, vm, NULL, g_free);
  vm->threads = g_ptr_array_new_with_free_func(g_free);
  vm->rand = gsl_rng_alloc(gsl_rng_taus);
  gsl_rng_set(vm->rand, seed);
  vm->mutations.rate_instr = rate_instr;
  vm->mutations.rate_mem = rate_mem;
  vm->mutations.rate_kill = rate_kill;

  return vm;
}

rfvm_t *rf_vm_new(void) {
  unsigned long seed;

  /* seed GSL's random number generator with glib's */
  seed = (unsigned long)g_random_int();

  return rf_vm_new_full(seed, 0.000001, 0.00001, 0.000001);
}


/* Free VM */
void rf_vm_free(rfvm_t *vm) {
  gsl_rng_free(vm->rand);
  g_tree_unref(vm->memory);
  g_ptr_array_free(vm->threads, TRUE);
}


/* Create thread */
rfth_t *rf_thread_add_full(rfvm_t *vm, rfp_t ip, rfp_t dp, rfp_t sp, const rfword_t *code, int code_length) {
  rfth_t *thread;

#ifdef rfVM_MAX_THREADS
  if (vm->threads->len>=RFVM_MAX_THREADS) {
    return NULL;
  }
#endif

  if (rf_memory_read(vm, ip, NULL)!=0) {
    return NULL;
  }

  thread = (rfth_t*)g_malloc(sizeof(rfth_t));
  memset(thread, 0, sizeof(rfth_t));
  thread->ip = ip;
  thread->dp = dp;
  thread->sp = sp;

  if (code!=NULL) {
    /* load code */
    if (code_length==-1) {
      code_length = strlen(code);
    }
    rf_load_data(vm, thread->ip, code, code_length);
  }

  g_ptr_array_add(vm->threads, thread);

  return thread;
}

rfth_t *rf_thread_add(rfvm_t *vm) {
  return rf_thread_add_full(vm, 0, 0, 0, NULL, -1);
}


/* Remove thread */
void rf_thread_remove(rfvm_t *vm, rfth_t *thread) {
  g_ptr_array_remove_fast(vm->threads, thread);
}


/* Run a cycle in thread */
gboolean rf_thread_cycle(rfvm_t *vm, rfth_t *thread) {
  rfp_t ip, pp;
  rfword_t instr, data;

  /* memory mutation */
  if (rf_rand_mutation(vm, vm->mutations.rate_mem)) {
    rf_memory_mutate(vm);
  }

  /* kill mutation */
  if (rf_rand_mutation(vm, vm->mutations.rate_kill)) {
    vm->mutations.num_kill++;
    return FALSE;
  }

  /* when an mutation occurs, the instruction is ignored */
  if (rf_rand_mutation(vm, vm->mutations.rate_instr)) {
    vm->mutations.num_instr++;
  }
  else {
    ip = thread->ip;
    instr = rf_memory_read(vm, ip, &thread->page_cache_ip);

    switch (instr) {
      /* increment data pointer */
      case '>':
        thread->dp++;
        break;

      /* decrement data pointer */
      case '<':
        thread->dp--;
        break;

      /* increment word at data pointer */
      case '+':
        data = rf_memory_read(vm, thread->dp, &thread->page_cache_dp);
        rf_memory_write(vm, thread->dp, data+1, &thread->page_cache_dp);
        break;

      /* decrement word at data pointer */
      case '-':
        data = rf_memory_read(vm, thread->dp, &thread->page_cache_dp);
        rf_memory_write(vm, thread->dp, data-1, &thread->page_cache_dp);
        break;

      /* set word at data pointer to random value */
      case ',':
        rf_rand(vm, &data, sizeof(data));
        rf_memory_write(vm, thread->dp, data, &thread->page_cache_dp);
        break;

      /* open parentheses */
      case '[':
        data = rf_memory_read(vm, thread->dp, &thread->page_cache_dp);
        if (data==0) {
          /* jump to matching parentheses */
          thread->ip = rf_find_matching_parentheses(vm, ip, &thread->page_cache_ip);
        }
        else if (thread->pstack==NULL || GPOINTER_TO_INT(thread->pstack->data)!=ip) {
          /* push pointer to this parentheses on stack */
          thread->pstack = g_slist_prepend(thread->pstack, GINT_TO_POINTER(ip));
        }
        break;

      /* closed parentheses */
      case ']':
        if (thread->pstack!=NULL) {
          data = rf_memory_read(vm, thread->dp, &thread->page_cache_dp);
          pp = GPOINTER_TO_INT(thread->pstack->data);
          /* check if '[' is still there */
          if (rf_memory_read(vm, pp, &thread->page_cache_ip)=='[') {
            if (data!=0) {
              /* jump back to matching parentheses (pop from stack) */
              thread->ip = pp-1;
            }
            else {
              thread->pstack = g_slist_delete_link(thread->pstack, thread->pstack);
            }
          }
        }
        /* else: unmatched ']', ignore */
        break;

      /* kills thread */
      case '*':
        return FALSE;
  
      /* fork - create another thread with IP & DP set to current thread's DP */
      case 'Y':
        rf_thread_add_full(vm, thread->dp, thread->dp, thread->dp, NULL, -1);
        break;

      /* push word at DP to stack */
      case '^':
        thread->sp--;
        data = rf_memory_read(vm, thread->dp, &thread->page_cache_dp);
        rf_memory_write(vm, thread->sp, data, &thread->page_cache_sp);
        break;

      /* pop word from stack to DP */
      case 'V':
        data = rf_memory_read(vm, thread->sp, &thread->page_cache_sp);
        rf_memory_write(vm, thread->dp, data, &thread->page_cache_dp);
        thread->sp++;
        break;

      /* set stack base */
      case '$':
        thread->sp = thread->dp;
        /* copy dp page cache to sp page cache */
        //memcpy(&thread->page_cache_sp, &thread->page_cache_dp, sizeof(struct rfth_page_cache));
        break;
    }
  }

  thread->ip++;
  thread->clock++;

  return thread->clock<RFTH_MAX_CYCLES;
}


/* Run a cycle in all threads */
void rf_vm_cycle(rfvm_t *vm) {
  unsigned int i;
  rfth_t *thread;

  for (i=0; i<vm->threads->len; i++) {
    thread = g_ptr_array_index(vm->threads, i);
    if (!rf_thread_cycle(vm, thread)) {
      rf_thread_remove(vm, thread);
      i--;
    }
  }

  vm->clock++;
}




/* Mutate random bits in a random word in memory */
void rf_memory_mutate(rfvm_t *vm) {
  rfword_t bitmask;
  unsigned int off;
  struct bt_memory_get_page_indexed_iterargs args;

  /* random page */
  args.idx = gsl_rng_uniform_int(vm->rand, g_tree_nnodes(vm->memory));
  g_tree_foreach(vm->memory, bt_memory_get_page_indexed_iter, &args);

  /* random offset */
  off = gsl_rng_uniform_int(vm->rand, RFMEM_PAGE_SIZE);

  /* random bitmask */
  bitmask = (rfword_t)gsl_rng_uniform(vm->rand);
  
  /* flip bits */
  args.page[off] ^= bitmask;

  vm->mutations.num_mem++;
}


/* Calculate page ID and offset from brainfuck pointer */
static void rf_memory_get_pid_and_offset(rfp_t p, int *pid, unsigned int *off) {
  if (p<0) {
    p = -p;
    *pid = -(p/RFMEM_PAGE_SIZE)-1;
  }
  else {
    *pid = p/RFMEM_PAGE_SIZE;
  }
  *off = p%RFMEM_PAGE_SIZE;
}


/* Get memory page */
static rfword_t *rf_memory_lookup_page(rfvm_t *vm, int pid, struct rfth_page_cache *page_cache) {
  rfword_t *page;

  /* try to look in thread cache */
  if (page_cache!=NULL && page_cache->page!=NULL && page_cache->pid==pid) {
    return page_cache->page;
  }
  else {
    /* lookup page in memory tree */
    page = g_tree_lookup(vm->memory, GINT_TO_POINTER(pid));

    if (page==NULL) {
      /* create page */
      page = (rfword_t*)g_malloc(sizeof(rfword_t)*RFMEM_PAGE_SIZE);
      rf_rand(vm, page, RFMEM_PAGE_SIZE);
      g_tree_insert(vm->memory, GINT_TO_POINTER(pid), page);
    }

    /* cache page lookup */
    if (page_cache!=NULL) {
      page_cache->pid = pid;
      page_cache->page = page;
    }

    return page;
  }
}


/* Read word at position p */
rfword_t rf_memory_read(rfvm_t *vm, rfp_t p, struct rfth_page_cache *page_cache) {
  int pid;
  unsigned int off;
  rfword_t *page;

  rf_memory_get_pid_and_offset(p, &pid, &off);
  page = rf_memory_lookup_page(vm, pid, page_cache);
  return page[off];
}


/* Write word at position p */
void rf_memory_write(rfvm_t *vm, rfp_t p, rfword_t data, struct rfth_page_cache *page_cache) {
  int pid;
  unsigned int off;
  rfword_t *page;

  rf_memory_get_pid_and_offset(p, &pid, &off);
  page = rf_memory_lookup_page(vm, pid, page_cache);
  page[off] = data;
}


/* Utility function to load custom data into memory
 * TODO use memcpy
 */
rfsz_t rf_load_data(rfvm_t *vm, rfp_t p, const rfword_t *data, rfsz_t n) {
  rfp_t i;

  for (i=0; i<n; i++) {
    rf_memory_write(vm, p+i, data[i], NULL);
  }

  return i;
}


unsigned int rf_get_num_threads(rfvm_t *vm) {
  return vm->threads->len;
}


rfth_t *rf_get_thread(rfvm_t *vm, unsigned int i) {
  if (i>=0 && i<vm->threads->len) {
    return g_ptr_array_index(vm->threads, i);
  }
  else {
    return 0;
  }
}


unsigned int rf_get_memory_usage(rfvm_t *vm) {
  return g_tree_nnodes(vm->memory)*RFMEM_PAGE_SIZE;
}

int rf_load_program(rfvm_t *vm, const char *filename, rfp_t p) {
  FILE *fd;
  rfword_t c;
  char line[1024];
  unsigned int i;
  rfp_t p0 = p;

  fd = fopen(filename, "rt");
  if (fd==NULL) {
    return -1;
  }

  while (fgets(line, 1024, fd)!=NULL) {
    /* read until string end, line end or comment start */
    for (i=0; line[i]!=0 && line[i]!=';' && line[i]!='\n'; i++) {
      if (line[i]=='\\' && line[i+1]=='x') {
        /* escaped character \x## */
        c = strtoul(line+i+2, NULL, 16);
        rf_memory_write(vm, p, c, NULL);
        i += 3;
        p++;
      }
      else if (line[i]!=' ') {
        /* write character to memory */
        rf_memory_write(vm, p, line[i], NULL);
        p++;
      }
    }
  }

  fclose(fd);

  return (int)p-p0;
}


static void rf_thread_store_pstackitem(void *data, void *userdata) {
  rfp_t p = GPOINTER_TO_INT(data);
  FILE *fd = (FILE*)userdata;

  fwrite(&p, sizeof(rfp_t), 1, fd);
}

static gboolean rf_memory_store_page(void *key, void *value, void *userdata) {
  FILE *fd = (FILE*)userdata;
  gint32 pid = (gint32)GPOINTER_TO_INT(key);
  rfword_t *page = (rfword_t*)value;

  /* write page ID & page */
  fwrite(&pid, sizeof(pid), 1, fd);
  fwrite(page, sizeof(rfword_t), RFMEM_PAGE_SIZE, fd);

  return FALSE;
}

/* store VM state */
gboolean rf_vm_store(rfvm_t *vm, const char *filename) {
  FILE *fd;
  guint32 pagesize = (guint64)RFMEM_PAGE_SIZE;
  guint8 wordsize = (guint8)sizeof(rfword_t);
  guint32 num_pages = (guint32)g_tree_nnodes(vm->memory);
  guint32 clock;
  guint16 num;
  rfth_t *thread;
  unsigned int i;

  /* open file */
  fd = fopen(filename, "wb");
  if (fd==NULL) {
    return FALSE;
  }

  /* file magic */
  fputs(RFMEM_DUMP_MAGIC, fd);

  /* write page size, word size and number of pages */
  fwrite(&wordsize, sizeof(wordsize), 1, fd);
  fwrite(&pagesize, sizeof(pagesize), 1, fd);
  fwrite(&num_pages, sizeof(num_pages), 1, fd);

  /* store random number generator state */
  gsl_rng_fwrite(fd, vm->rand);

  /* store number of threads & VM clock */
  num = (guint16)vm->threads->len;
  fwrite(&num, sizeof(num), 1, fd);
  clock = (guint32)vm->clock;
  fwrite(&clock, sizeof(clock), 1, fd);

  /* store threads */
  for (i=0; i<vm->threads->len; i++) {
    thread = g_ptr_array_index(vm->threads, i);
    fwrite(&thread->ip, sizeof(rfp_t), 1, fd);
    fwrite(&thread->dp, sizeof(rfp_t), 1, fd);
    fwrite(&thread->sp, sizeof(rfp_t), 1, fd);
    clock = (guint32)thread->clock;
    fwrite(&clock, sizeof(clock), 1, fd);

    /* store parentheses stack */
    num = (guint16)g_slist_length(thread->pstack);
    fwrite(&num, sizeof(num), 1, fd);
    g_slist_foreach(thread->pstack, rf_thread_store_pstackitem, fd);
  }

  /* store memory */
  g_tree_foreach(vm->memory, rf_memory_store_page, fd);
  
  /* close file */
  fclose(fd);

  return TRUE;
}

/* load VM state
 */
gboolean rf_vm_load(rfvm_t *vm, const char *filename) {
  FILE *fd;
  guint32 pagesize, num_pages, pid;
  guint8 wordsize;
  rfword_t *page;
  unsigned int i;
  char magic[RFMEM_DUMP_MAGIC_LENGTH+1];

  /* open file */
  fd = fopen(filename, "rb");
  if (fd==NULL) {
    return FALSE;
  }

  /* check magic */
  fgets(magic, sizeof(magic), fd);
  if (strcmp(magic, RFMEM_DUMP_MAGIC)!=0) {
    fclose(fd);
    return FALSE;
  }

  /* read page size, word size & number of pages */
  fread(&pagesize, sizeof(pagesize), 1, fd);
  fread(&wordsize, sizeof(wordsize), 1, fd);
  fread(&num_pages, sizeof(num_pages), 1, fd);

  /* check page size & word size */
  if (pagesize!=RFMEM_PAGE_SIZE || wordsize!=sizeof(rfword_t)) {
    fclose(fd);
    return FALSE;
  }

  /* read pages */
  for (i=0; i<num_pages; i++) {
    /* read page ID */
    fread(&pid, sizeof(pid), 1, fd);

    /* read page */
    page = (rfword_t*)g_malloc(sizeof(rfword_t)*RFMEM_PAGE_SIZE);
    fread(page, sizeof(rfword_t), RFMEM_PAGE_SIZE, fd);

    /* insert page into memory tree */
    g_tree_insert(vm->memory, GINT_TO_POINTER(pid), page);
  }

  /* close file */
  fclose(fd);

  return TRUE;
}

