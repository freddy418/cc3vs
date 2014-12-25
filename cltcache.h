#ifndef CLTCACHE_H
#define CLTCACHE_H

#include "utils.h"
#include "rgcache.h"
#include "ccache.h"
#include "memmap.h"
#include "store.h"

class cltcache {
  // l1 range
  rgcache* cl2;
  // uncompressed cache
  ccache* ul2;
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
  i32 os; 
  // next level
  tmemory * mem;
  mem_map* map;
  tcache* next_level;
  // string name
  char * name;
 public:
  cltcache(i32 csets, i32 usets, i32 os);
  i64 read(i32 addr);
  void write(i32 addr, i64 val);
  i32 refill(i32 addr);
  void copy(i32 addr, cache_block* op);
  void touch(i32 addr);
  void stats();
  void clearstats();
  void set_anum(i32 v);
  i64 get_accs();
  void set_accs(i64 as);
  i64 get_hits();
  void set_hits(i64 hs);
  void subaccs(i32 addr, i32 num);
  void set_name(char *cp);
  void set_map(mem_map* mp);
  void set_mem(tmemory* sp);
  void allocate(i32 addr);
  void set_nl(tcache* cp);
};

#endif /* CLTCACHE_H */
