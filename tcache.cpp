#include "tcache.h"

tcache::tcache(i32 ns, i32 as, i32 bs, i32 ofs){
  /* initialize cache parameters */
  sets = new cache_set[ns];
  mem = 0;
  map = 0;
  nsets = ns;
  bsize = bs;
  bvals = bs >> 3;
  assoc = as;
  imask = ns-1;
  oshift = 3 - ofs;
  bmask = (bs >> 3) - 1;

  ishift = log2(ns);
  bshift = log2(bs) - ofs;
  amask = -1 << bshift;

  // initialize pointer to L2 as zero
  next_level = 0;
  /* initialize statisitic counters */
  accs = 0;
  hits = 0;
  misses = 0;  
  writebacks = 0;
  allocs = 0;
 
#ifdef LINETRACK
  mcount = (i32*)calloc(nsets, sizeof(i32));
  acount = (i32*)calloc(nsets, sizeof(i32));
#endif

  for (i32 i=0;i<nsets;i++){
    /* distribute data blocks to cache sets */
    sets[i].blks = new cache_block[assoc];
    for (i32 j=0;j<assoc;j++){
      sets[i].blks[j].valid = 0;
      sets[i].blks[j].dirty = 0;
      sets[i].blks[j].tag = 0;
      sets[i].blks[j].value = 0;
    }
    /* initialize LRU info */ 
    item* node = new item();
    node->val = 0;
    sets[i].lru = node;
    for (i32 j=1;j<assoc;j++){
      node->next = new item();
      node->next->val = j;
      node = node->next;
    }
  }
}

void tcache::clearstats(){
  this->accs = 0;
  this->hits = 0;
  this->misses = 0;
  this->bwused = 0;
  writebacks = 0;
  allocs = 0;

#ifdef LINETRACK 
  for(int i=0;i<nsets;i++){
    mcount[i] = acount[i] = 0;
  }
#endif
}

void tcache::writeback(cache_block* bp, i32 addr){
  i32 ret = 0;
  i32 zero = 0;

  // L1 cache
  if (next_level != 0){
    next_level->copy(addr, bp);
    bwused += bsize;
  }

  // update maps on eviction
  if (map != 0 && bp->dirty == 1){
    for (i32 i=0;i<bvals;i++){
      if (bp->value[i] != 0){
	zero = 1;
	break;
      }
    }
    //if (zero == 0){ // all zeros
      // only call this if map is enabled - how?
      ret = map->update_block((addr&amask), zero);
      //}
  }

#ifdef DBG
  if ((addr & amask) <= DBG_ADDR && (addr & amask) + (bvals<<oshift) > DBG_ADDR){
    printf("%s Writing back (%x) zero(%u) mem(%u) map(%u) ret(%u): ", name, addr, zero, mem, map, ret);
  }
#endif

  if (mem != 0 && (zero == 1 || map == 0 || ret == 0)){
    for (i32 i=0;i<bvals;i++){
      //printf("writing %llu to mem at %u\n", bp->value[i], (addr & amask) + (i<<oshift));
#ifdef DBG
      if ((addr & amask) <= DBG_ADDR && (addr & amask) + (bvals<<oshift) > DBG_ADDR){
      printf("%llx,", bp->value[i]);
    }
#endif

      mem->write((addr & amask) + (i<<oshift), bp->value[i]);
    }
    bwused += bsize;
  }

#ifdef DBG
  if ((addr & amask) <= DBG_ADDR && (addr & amask) + (bvals<<oshift) > DBG_ADDR){
  printf("\n");
}
#endif

  //bwused += bsize;
  bp->dirty = 0;
  writebacks++;
}

void tcache::allocate(i32 addr){
  i32 tag, index, hit, hitway, wbaddr;
  cache_block* bp;

#ifdef DBG
  if ((addr & amask) <= DBG_ADDR && (addr & amask) + (bvals<<oshift) > DBG_ADDR){    
    printf("%s allocating line for addr(%x)\n", name, addr);
  }
#endif

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

  //hits+=hit;
  //accs++;
  //misses+=(1-hit);

  // if block is valid and dirty, write it back
  if (hit == 0){
    if (bp->valid == 1 && bp->dirty == 1){
      wbaddr = ((bp->tag) << (ishift+bshift)) + (index<<(bshift));
      // no need to allocate because allocate is not refilling
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
    bp->valid = 1;
    bp->dirty = 0;
    for (i32 i=0;i<bvals;i++){
      bp->value[i] = 0;
    }
  } // otherwise just update LRU info

  update_lru(&(sets[index]), hitway);
  allocs++;
}

void tcache::touch(i32 addr){
  i32 tag, index, hit, hitway;
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

  if (hit == 1){
    update_lru(&(sets[index]), hitway);
  }
}

void tcache::copy(i32 addr, cache_block* op){
  i32 tag, index, hitway, wbaddr, hit;
  cache_block* bp;

  /*if ((addr & amask) <= 4294941704 && (addr & amask) + (bvals<<oshift) > 4294941704){
    printf("%s copying to (%x)\n", name, addr);
    }*/

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
    // no need to refill, whole line is written from L1
    this->writeback(bp, wbaddr);
  }

  if (bp->value == 0){
    bp->value = (i64*) calloc(bvals, sizeof(i64));
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

  update_lru(&(sets[index]), hitway);
}

void tcache::refill(cache_block* bp, i32 addr){
  i32 i, index, tag;
  tag = (addr >> (bshift + ishift)); 
  index = (addr >> bshift) & imask;
  
  /*if ((addr & amask) <= 4294941704 && (addr & amask) + (bvals<<oshift) > 4294941704){
    printf("%s refilling to (%x) ", name, addr);
    }*/

  bp->tag = tag;
  bp->valid = 1;
  bp->dirty = 0;
  if (bp->value == 0){
    bp->value = (i64*) calloc(bvals, sizeof(i64));
    if (bp->value == 0){
      printf("FATAL: calloc ran out of memory!\n");
      exit(1);
    }
  }

  if (next_level != 0){
    for (i32 i=0;i<bvals;i++){
      bp->value[i] = next_level->read((addr & amask) + (i<<oshift));
    }
    //next_level->set_accs(next_level->get_accs() - (bvals-1));
    //next_level->set_hits(next_level->get_hits() - (bvals-1));
    //next_level->subaccs(addr, bvals-1);    
    bwused += bsize;
  }
  else if (mem != 0){
    //printf("sets(%u), bsize(%u) - Refill from memory - addr(%08X), index(%u), tag(%X)\n", nsets, bsize, addr, index, tag);
    //exit(1);
    for (i=0;i<bvals;i++){
      bp->value[i] = mem->read((addr & amask) + (i<<oshift));
#ifdef DBG
      if ((addr & amask) + (i<<oshift) == DBG_ADDR){
	printf("%s Reading (%x) from memory: %llx\n", name, (addr & amask) + (i<<oshift), bp->value[i]);
      }
#endif
    }
    bwused += bsize;
  }
  //printf("block size: %d, index: %d, addr: %X, bmask: %X\n", (bsize), (addr>>bshift)&(bmask), addr, bmask);
  /*if ((addr & amask) <= 4294941704 && (addr & amask) + (bvals<<oshift) > 4294941704){
    printf("\n");
    }*/
}

i64 tcache::read(i32 addr){
  i32 index = (addr >> bshift) & imask;
  i32 tag = (addr >> (bshift + ishift));  
  i32 hit = 0;
  i32 hitway = 0;
  i32 zero = 0;
  i32 wbaddr;
  cache_block* block;

  // check tags
  for(i32 i=0;i<assoc;i++){
    if ((sets[index].blks[i].tag == tag) && (sets[index].blks[i].valid == 1)){
      hit = 1;
      hitway = i;
      break;
    }
  }

#ifdef LINETRACK
  acount[index]++;
#endif

  // update bookkeeping
  if (hit == 1){
    hits++;
    block = &(sets[index].blks[hitway]);
#ifdef LOG
    // only one read happens for each range cache miss
    fprintf(tlog, "%s\n", name);
#endif
  }else{
    misses++;    
    hitway = sets[index].lru->val;
    block = &(sets[index].blks[hitway]);
    //printf("miss to index: %d on tag: %x, replaced %d\n", index, tag, hitway);
    if (block->valid == 1 && block->dirty == 1){
      // lock line in next level
      if (next_level != 0){
	next_level->touch(addr);
      }
      wbaddr = ((block->tag)<<(ishift+bshift)) + (index<<bshift);
      this->writeback(block, wbaddr);
    }

#ifdef LOG
    // only one read happens for each range cache miss
    fprintf(tlog, "m\n");
#endif
    this->refill(block, addr);
  }
  
  this->update_lru(&(sets[index]), hitway);
  accs++;

  //printf("bsize(%u), sets(%u) - Access: read, addr(%X), index(%X), tag(%X), block(%X)\n", bsize, nsets, addr, index, tag, ((addr>>(oshift))&bmask));

#ifdef DBG
  if (addr == DBG_ADDR){
    printf("%s %s Reading (%llx) from addr (%x) in index(%u) and way(%u)\n", name, hit==1?"hit":"miss",block->value[((addr>>(oshift))&bmask)], addr, index, hitway);
    }
#endif

  return block->value[((addr>>(oshift))&bmask)];
}

void tcache::write(i32 addr, i64 data){
  i32 index = (addr >> bshift) & imask;
  i32 tag = (addr >> (bshift + ishift));  
  i32 hit = 0;
  i32 hitway = 0;
  i32 zero = 0;
  i32 wbaddr;
  cache_block* block;

  // check tags
  for(i32 i=0;i<assoc;i++){
    block = &(sets[index].blks[i]);
    if ((sets[index].blks[i].tag == tag) && (sets[index].blks[i].valid == 1)){
      hit = 1;
      hitway = i;
    }
  }

#ifdef LINETRACK
  acount[index]++;
#endif

  // update bookkeeping
  if (hit == 1){
    hits++;
    block = &(sets[index].blks[hitway]);
  }else{
    misses++;    
    hitway = sets[index].lru->val;
    block = &(sets[index].blks[hitway]);
    //printf("miss to index: %d on tag: %x, replaced %d\n", index, tag, hitway);
    if (block->valid == 1 && block->dirty == 1){
      // lock line in next level
      if (next_level != 0){
	next_level->touch(addr);
      }
      wbaddr =  ((block->tag)<<(ishift+bshift)) + (index<<bshift);
      /*if (index == 224 && hitway == 7){
	printf("Writing back %x in index(%u) and way (%u) to memory\n", wbaddr, index, hitway);
	}*/
      this->writeback(block, wbaddr);
    }
    this->refill(block, addr);
  }

#ifdef DBG
  if (addr == DBG_ADDR){
    printf("%s Writing (%llx) to addr (%x) in index(%u) and way(%u)\n", name, data, addr, index, hitway);
  }
#endif
    
  block->value[((addr>>oshift)&bmask)] = data;
  block->dirty = 1;
  this->update_lru(&(sets[index]), hitway);
  accs++;
}

void tcache::stats(){
  i32 size = (nsets) * (assoc) * (bsize);

  printf("%s: %d KB cache:\n", name, size >> 10);
  printf("%s map = %X\n", name, map);
  printf("miss rate: %1.8f\n", (((double)misses)/(accs)));
  printf("%llu accesses, %llu hits, %llu misses, %llu writebacks, %llu allocs\n", accs, hits, misses, writebacks, allocs);
  printf("bandwidth used: %lu KB\n", (bwused >> 10));
 
#ifdef LINETRACK 
  for (i32 i=0;i<nsets;i++){
    printf("Set %u, Accesses %u, Misses %u\n", i, acount[i], mcount[i]);
  }
#endif
}


void tcache::update_lru(cache_set * set, unsigned int hitway){
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

void tcache::set_mem(tmemory* sp){
  mem = sp;
}

void tcache::set_map(mem_map* mp){
  map = mp;
}

void tcache::set_nl(tcache* cp){
  next_level = cp;
}

void tcache::set_name(char* cp){
  name = cp;
}

void tcache::set_anum(i32 n){
  anum = n;
}

i64 tcache::get_accs(){
  return accs;
}

i64 tcache::get_hits(){
  return hits;
}

void tcache::set_accs(i64 num){
  accs = num;
}

void tcache::set_hits(i64 num){
  hits = num;
}

