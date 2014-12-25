#ifndef CCACHE_H
#define CCACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>
#include "tcache.h"
#include "utils.h"
#include "memmap.h"
#include "store.h"

// cache implementation
class ccache {
  cache_set* sets;
  i32 nsets;
  i32 bsize;
  i32 bvals;
  i32 assoc;
  i32 imask;
  i32 bmask;
  i32 ishift;
  i32 bshift;
  i32 oshift;
  i32 amask;
  i64 accs;
  i32 anum;
  i64 hits;
  i64 misses;
  i64 bwused;
  i64 totalwbs;
  tmemory* mem;
  mem_map* map;
  char* name;
  tcache* next_level;  // next level
 public:
  ccache (i32 nsets, i32 bsize, i32 assoc, i32 os);
  void annul(i32 addr);
  i32 check(i32 addr);
  i64 read(i32 addr);
  i32 write(i32 addr, i64 data);
  void writeback(cache_block* bp, i32 addr);
  void refill(i32 addr, i64 dp);
  void stats();
  void update_lru(cache_set * set, i32 hitway);
  void copy(i32 addr, cache_block* op);
  void touch(i32 addr);
  i64 get_accs();
  void set_accs(i64 as);
  i64 get_hits();
  void set_hits(i64 hs);
  void set_anum(i32 n);
  void set_mem(tmemory* sp);
  void set_map(mem_map* mp);
  void set_name(char* cp);
  void clearstats();
  void set_dict(comp_dict* d);
  i32 getubw();
  void set_nl(tcache* cp);
};

#endif /* CCACHE_H */
