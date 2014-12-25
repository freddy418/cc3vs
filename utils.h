#ifndef UTILS_H
#define UTILS_H

#include <iostream>

typedef unsigned int i32;
typedef unsigned long long i64;

#ifdef LOG
extern FILE * tlog;
#endif

// lru implementation

typedef struct llnode 
{
  unsigned int val;
  struct llnode * next;
} item;

unsigned int pow2(unsigned int v);

typedef struct cache_blk_t
{
  i32 valid;
  i32 dirty;
  i32 tag;
  i64 * value;
} cache_block;

typedef struct cache_set
{
  cache_block* blks;
  item* lru;
} cache_set;

typedef struct dict_struct {
  i64 * v; // actual values used from compression
#ifndef STATIC_DICT
  i64 * c; // candidates
  i64 * rc;
  item * plru;
#endif
} comp_dict;

typedef struct range_cache_entry {
  i64 value;  // value of entry
  i32 la; // start address of range
  i32 ha; // end address of range
  i32 dirty;
  i32 valid;
} rc_ent;

#endif /* UTILS_H */
