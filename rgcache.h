#ifndef RGCACHE_H
#define RGCACHE_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h"
#include "tcache.h"
#include "memmap.h"
#include "store.h"
#include <string.h>

class rgcache {
  // compressed cache lines
  rc_ent* lines;
  item* lru;
  i32 nsets;
  // statistics bookkeeping
  i64 accs;
  i64 hits;
  i64 totalrefills;
  i64 cpbrefills;
  i64 totalwbs;
  i64 cpbwbs;
  i64 misses;
  i64 bwused;
  i32 anum;
  i32 bvals;
  i32 bshift;
  i32 oshift;
  i32 amask;
  i32 dsize;
  // parameters
  tcache* next_level;  // next level
  tmemory * mem;
  mem_map* map;
  // string name
  char* name;
 public:
  rgcache(i32 ns, i32 ofs);
  void set_dict(char* file);
  i64 read(i32 addr);
  i32 check(i32 addr);
  i32 write(i32 addr, i64 data);
  void refill(i32 addr, i64 data);
  void evict(i64* dp);
  void copy(i32 addr, cache_block* op);
  void touch(i32 addr);
  void stats();
  void clearstats();
  void set_anum(i32 v);
  i32 check_comp(i64* din, i64* dout);
  i64 get_accs();
  void set_accs(i64 as);
  i64 get_hits();
  void set_hits(i64 hs);
  void subaccs(i32 addr, i32 num);
  void set_name(char* cp);
  void set_map(mem_map* mp);
  void set_mem(tmemory* sp);
  i32 get_lru();
  void update_lru(i32 entry);
  i32 write_cache(i32 low, i32 high, i64 data);
  void writeback(i32 index);
  void set_nl(tcache* cp);
  void annul(i32 addr);
  void allocate(i32 addr);
};

#endif /* RGCACHE_H */
