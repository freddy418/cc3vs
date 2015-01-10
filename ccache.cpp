#include "ccache.h"

ccache::ccache (i32 ns, i32 as, i32 bs, i32 os){
  /* initialize cache parameters */
  sets = new cache_set[ns];
  nsets = ns;
  bsize = bs;
  bvals = bs >> 3;
  assoc = as;
  imask = ns-1;
  oshift = 3 - os;
  bmask = (bs >> 3) - 1;
  ishift = log2(ns);
  bshift = log2(bs) - os;
  amask = -1 << bshift;

#ifdef DEBUG
  printf("Init ccache (nsets(%d), bsize(%d), bvals(%d), assoc(%d), imask(%X), oshift(%d), bmask(%X), ishift(%d), bshift(%d), amask(%X))\n", nsets, bsize, bvals, assoc, imask, oshift, bmask, ishift, bshift, amask);
  fflush(stdout);
#endif

  // initialize L2 pointer to zero
  next_level = 0;
  /* initialize statistic counters */
  accs = 0;
  hits = 0;
  misses = 0;
  totalwbs = 0;

  for (i32 i=0;i<nsets;i++){
    /* distribute data blocks to cache sets */
    sets[i].blks = new cache_block[as];
    for (i32 j=0;j<as;j++){
      sets[i].blks[j].valid = 0;
      sets[i].blks[j].dirty = 0;
      sets[i].blks[j].tag = 0;
      sets[i].blks[j].value = 0;
    }
    /* initialize LRU info */ 
    item* node = new item();
    node->val = 0;
    sets[i].lru = node;
    for (i32 j=1;j<as;j++){
      node->next = new item();
      node->next->val = j;
      node = node->next;
    }
  }

  anum = 0;
  //printf("allocated ccache with nsets=%u, bsize=%u, bvals=%u, assoc=%u, imask=%x, bmask=%x, ishift=%u, bshift=%u, oshift=%u, amask=%x, name=%s\n", nsets, bsize, bvals, assoc, imask, bmask, ishift, bshift, oshift, amask, name);
}

i32 ccache::check(i32 addr){
  i32 index, tag, hit;
  index = (addr >> bshift) & imask;
  tag = (addr >> (bshift + ishift));

#ifdef DEBUG
  printf("Calling ccache check for addr(%X) tag(%X)\n", addr, tag);
#endif

  // check tags
  hit = 0;
  for(i32 i=0;i<assoc;i++){
    if ((sets[index].blks[i].tag == tag) && (sets[index].blks[i].valid == 1)){
#ifdef DEBUG
      printf("ccache check for addr(%X), index(%d): tag(%X), valid(%d) matches\n", addr, index, sets[index].blks[i].tag, sets[index].blks[i].valid);
#endif
      hit = 1;
    }
  }

  return hit;
}

void ccache::annul(i32 addr){
  i32 index = (addr >> bshift) & imask;
  i32 tag = (addr >> (bshift + ishift));  
  i32 hitway = 0;
  cache_block* bp;

#ifdef DBG
  if (addr == DBG_ADDR){
    printf("Annuling %x in %s\n", addr, name);
  }
#endif

  // check tags
  for(i32 i=0;i<assoc;i++){
    if ((sets[index].blks[i].tag == tag) && (sets[index].blks[i].valid == 1)){
      hitway = i;
    }
  }

  bp = &(sets[index].blks[hitway]);
  bp->valid = 0;
  free(bp->value);
}

void ccache::writeback(cache_block* bp, i32 addr){
  i32 i, zero = 0;

  // L1 cache
  if (next_level != 0){
#ifdef DBG
    if (addr == DBG_ADDR){
      printf("%s writing back addr(%x) and data(%llx)\n", name, addr, bp->value[0]);
    }
#endif
    next_level->write(addr, bp->value[0]);
    bwused += bsize;
  }

  // update maps on eviction
  if (map != 0 && bp->dirty == 1){
    for (i=0;i<bvals;i++){
      if (bp->value[i] != 0){
	zero = 1;
	break;
      }
    }
    if (zero == 0){ // all zeros
      map->update_block(addr, 0);
    }
  }

  if (mem != 0 && (zero == 1 || map == 0)){
    for (i=0;i<bvals;i++){
      mem->write((addr & amask) + (i<<oshift), bp->value[i]);
    }
    bwused += bsize;
  }

  bp->dirty = 0;
  totalwbs++;
}

void ccache::touch(i32 addr){
  i32 tag, index, hit, hitway;
  cache_block* bp;

  // calculate tag, index, and lru entry
  index = (addr >> bshift) & imask;
  tag = (addr >> (bshift + ishift));
  hitway = sets[index].lru->val;

  // check hit
  hit = 0;
  for(i32 i=0;i<assoc;i++){
    bp = &(sets[index].blks[i]);
    if ((bp->tag == tag) && (bp->valid == 1)){
      hit = 1;
      hitway = i;
    }
  }

  // update lru to clamp line being touched
  if (hit == 1){
    update_lru(&(sets[index]), hitway);
  }
}

void ccache::copy(i32 addr, cache_block* op){
  i32 tag, index, hitway, wbaddr, hit;
  cache_block* bp;

  index = (addr >> bshift) & imask;
  tag = (addr >> (bshift + ishift));
  hitway = sets[index].lru->val;
  hit = 0;
  for(i32 i=0;i<assoc;i++){
    bp = &(sets[index].blks[i]);
    if ((bp->tag == tag) && (bp->valid == 1)){
      hit = 1;
      hitway = i;
    }
  }
  bp = &(sets[index].blks[hitway]);

  // if block is valid and dirty, write it back
  if ((hit == 0) && (bp->valid == 1) && (bp->dirty == 1)){
    wbaddr = ((bp->tag)<<(ishift+bshift)) + (index<<bshift);
    this->writeback(bp, wbaddr);
  }

  if (bp->value == 0){
    bp->value = new i64[bvals];
    if (bp->value <= 0){
      printf("FATAL: calloc ran out of memory!\n");
      exit(1);
    }
  }

  bp->tag = tag;
  bp->valid = op->valid;
  bp->dirty = op->dirty;
  for (i32 i=0;i<bvals;i++){
    bp->value[i] = op->value[i];
  }

  this->update_lru(&(sets[index]), hitway);
}

void ccache::refill(i32 addr, i64 data){
  i32 index, tag, line, wbaddr;
  cache_block* bp;

  index = 0; //(addr >> bshift) & imask; -- Fully Associative shortcut
  tag = (addr >> (bshift + ishift));
  line = sets[index].lru->val;
  bp = &(sets[index].blks[line]);

  if (bp->valid ==1 && bp->dirty == 1){
    if (next_level != 0){
      next_level->touch(addr);
    }
    wbaddr = ((bp->tag)<<(ishift+bshift)) + (index<<bshift);
    this->writeback(bp, wbaddr);
  }
  
  bp->tag = tag;
  if (bp->valid != 1){
    bp->value = new i64[bvals];
  }
  bp->value[0] = data;
  bp->valid = 1;
  bp->dirty = 0;
  update_lru(&(sets[index]), line);
}

i64 ccache::read(i32 addr){
  i32 index = (addr >> bshift) & imask;
  i32 tag = (addr >> (bshift + ishift));  
  i32 hit = 0;
  i32 hitway = 0;
  cache_block* block;

  accs++;

  // check tags
  for(i32 i=0;i<assoc;i++){
    if ((sets[index].blks[i].tag == tag) && (sets[index].blks[i].valid == 1)){
      hit = 1;
      hitway = i;
    }
  }

  if (hit == 1){
    hits++;
    block = &(sets[index].blks[hitway]);
    this->update_lru(&(sets[index]), hitway);
    return block->value[((addr>>(oshift))&bmask)];    
  }else{
    misses++;
    // This should not happen
    printf("ccache reads must not miss!\n");
    fflush(stdout);
    assert(0);
    return 0;
  }
}

i32 ccache::write(i32 addr, i64 data){
  i32 index = (addr >> bshift) & imask;
  i32 tag = (addr >> (bshift + ishift));  
  i32 hit = 0;
  i32 hitway = 0;
  i32 zero = 0;
  i32 wbaddr;
  cache_block* block;

#ifdef DEBUG
  printf("Calling ccache write for addr(%X) tag(%X)\n", addr, tag);
#endif

  /*if (addr == 2928741272){
    printf("Writing %llx to %x in %s\n", data, addr, name);
    }*/
  /*if (index < 16){
    printf("\tIndex %d manipulated in ccache::write, hitway: %u\n", index, hitway);
    }*/

  // check tags
  for(i32 i=0;i<assoc;i++){
    //printf("Write - addr(%08X), index(%u), tag(%X), way%d tag (%X)\n", addr, index, tag, i, sets[index].blks[i].tag);
    block = &(sets[index].blks[i]);
    if ((sets[index].blks[i].tag == tag) && (sets[index].blks[i].valid == 1)){
      hit = 1;
      hitway = i;
    }
  }

  // update bookkeeping
  if (hit == 1){
    hits++;
    block = &(sets[index].blks[hitway]);
  }else{
    hitway = sets[index].lru->val;
    block = &(sets[index].blks[hitway]);
    if (block->valid == 1 && block->dirty == 1){
      // lock line in next level
      if (next_level != 0){
	next_level->touch(addr);
      }
      wbaddr =  ((block->tag)<<(ishift+bshift)) + (index<<bshift);
      this->writeback(block, wbaddr);
    }
    // skip the refill
  }
  
  block->tag = tag;
  if (block->valid != 1){
    block->value = new i64[bvals];
  }
  block->value[0] = data;
  block->valid = 1;
  block->dirty = 1;
  this->update_lru(&(sets[index]), hitway);

  return hit;
}

void ccache::stats(){
  i32 size = nsets * assoc * bsize;

  printf("%d-way %d-sets %s cache:\n", assoc, nsets, name);
  printf("%llu accesses, %llu hits, %llu misses, %llu writebacks\n", accs, hits, misses, totalwbs);
}

void ccache::update_lru(cache_set * set, unsigned int hitway){
  item * hitnode;
  item * node = set->lru;
  item * temp;

  /*temp = set->lru;
  printf("before: way %d referenced, (%d,", hitway, temp->val);
  while (temp->next != 0){
    temp = temp->next;
    printf("%d,", temp->val);
  }
  printf(")\n");*/

  if (node->next == 0){
    // direct-mapped cache
    return;
  }
  
  if (node->val == hitway){
    hitnode = node;
    set->lru = node->next;
    while (node->next != 0){
      node = node->next;
    }
  }else{
    while (node->next != 0){
      if (node->next->val == hitway){
	hitnode = node->next;
	node->next = hitnode->next;
      }else{
	node = node->next;
      }
    }
  }
  node->next = hitnode;
  hitnode->next = 0;

  /*temp = set->lru;
  printf("after: way %d referenced, (%d,", hitway, temp->val);
  while (temp->next != 0){
    temp = temp->next;
    printf("%d,", temp->val);
  }
  printf(")\n");*/
}

void ccache::set_mem(tmemory* sp){
  mem = sp;
}

void ccache::set_map(mem_map* mp){
  map = mp;
}

void ccache::set_name(char *cp){
  name = cp;
}

void ccache::set_anum(i32 n){
  anum = n;
}

i32 ccache::getubw(){
  return bwused;
}

i64 ccache::get_accs(){
  return accs;
}

void ccache::set_accs(i64 as){
  accs = as;
}

i64 ccache::get_hits(){
  return hits;
}

void ccache::set_hits(i64 hs){
  hits = hs;
}

void ccache::clearstats(){
  this->accs = 0;
  this->hits = 0;
  this->misses = 0;
  this->bwused = 0;
  this->totalwbs = 0;
}

void ccache::set_nl(tcache* cp){
  next_level = cp;
}
