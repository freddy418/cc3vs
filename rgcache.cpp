#include "rgcache.h"

rgcache::rgcache(i32 ns, i32 ofs){

  nsets = ns;
  lines = new rc_ent[nsets];

  for (i32 i=0;i<nsets;i++){
    lines[i].valid = 0;
    lines[i].dirty = 0;
    lines[i].la = 0;
    lines[i].ha = 0;
    lines[i].value = 0;
  }

  oshift = 3 - ofs;
  dsize = (1 << oshift);

  // initialize pointer to L2 as zero
  next_level = 0;
  // update statistics
  accs = 0;
  hits = 0;
  misses = 0;
  bwused = 0;
  totalrefills = 0;
  totalwbs = 0;
  
  /* initialize LRU info */ 
  item* node = new item();
  node->val = 0;
  lru = node;
  for (i32 j=1;j<nsets;j++){
    node->next = new item();
    node->next->val = j;
    node = node->next;
  }
}

void rgcache::annul(i32 addr){
  // TODO: implement me
  i32 hitway = nsets;

  /*if (addr == 4294941704){
    printf("Annuling %x in %s\n", addr, name);
    }*/

  for (i32 i=0;i<nsets;i++){
    if (lines[i].la <= addr && lines[i].ha >= addr && lines[i].valid == 1){
#ifdef DEBUG
      printf("rgcache check for addr(%X), index(%d): low(%X) and high(%X) and valid(%d) matches\n", addr, i, lines[i].la, lines[i].ha, lines[i].valid);
#endif
      hitway = i;
    }
  }

  if (hitway == nsets){
    printf("Annul should not be called if cache did not hit!\n");
    fflush(stdout);
    assert(0);
  }
    
  // case a: invalidate entire range
  if (lines[hitway].la == addr && lines[hitway].ha == addr && lines[hitway].valid == 1){
    /*if (addr == 4294952612){
      printf("Annul(%u): invalidate range entry(%u-%u)\n", addr);
      }*/
    lines[hitway].dirty = 0;
    lines[hitway].valid = 0;
  }
  // case b: decrement low end of existing range      
  else if (lines[hitway].la == addr && lines[hitway].valid == 1){
    /*if (addr == 4294952612){
      printf("Annul(%u): increment low end of range(%u-%u) to", addr, lines[hitway].la, lines[hitway].ha);
      }*/
    lines[hitway].la += dsize;
    /*if (addr == 4294952612){
      printf(" range(%u-%u)\n", lines[hitway].la, lines[hitway].ha);
      }*/
  }
  // case c: decrement high end of existing range     
  else if (lines[hitway].ha == addr && lines[hitway].valid == 1){
    /*if (addr == 4294952612){
      printf("Annul(%u): decrement high end of range(%u-%u)\n", addr, lines[hitway].la, lines[hitway].ha);
      }*/
    lines[hitway].ha -= dsize;
  }
  // case d: split range into two ranges
  else if ((lines[hitway].la < addr) && (addr < lines[hitway].ha) && (lines[hitway].valid == 1)){
    /*if (addr == 4294952612){
      printf("Annul(%u): splitting range %u(%u-%u) into 2 ranges: (%u-%u), (%u-%u)\n", addr, hitway, lines[hitway].la, lines[hitway].ha, lines[hitway].la, (addr-dsize), (addr+dsize), lines[hitway].ha);
      }*/
    write_cache(addr+dsize, lines[hitway].ha, lines[hitway].value);
    lines[hitway].ha = (addr-dsize);
    update_lru(hitway);
  }
  // otherwise: do nothing (shouldn't be called)
  else{
    //printf("Annul called and no action taken!\n");
    fflush(stdout);
    assert(0);
  }

}

i32 rgcache::check(i32 addr){
  i32 hit = 0;
  i32 hitway = nsets;

  // do the read
  for (i32 i=0;i<nsets;i++){
    if (lines[i].la <= addr && lines[i].ha >= addr && lines[i].valid == 1){
#ifdef DEBUG
      printf("rgcache check for addr(%X), index(%d): low(%X) and high(%X) and valid(%d) matches\n", addr, i, lines[i].la, lines[i].ha, lines[i].valid);
#endif
      hit = 1;
      hitway = i;
    }
  }

  /*if (addr == 4294952612 && hit == 1){
    printf("Check(%u): hit on entry(%u) with range(%u-%u)\n", addr, hitway, lines[hitway].la, lines[hitway].ha);
    }*/

  return hit;
}

void rgcache::allocate(i32 addr){
  // nop
}

i32 rgcache::write(i32 addr, i64 data){
  // writes always hit, but what about the bookkeeping updates?
  i32 hit;

#ifdef LOG
    fprintf(tlog, "%s\n", name);
#endif
  hit = write_cache(addr, addr, data);

  accs++;
  if (hit == 1){
    hits++; // always hits... (make a new range)
  }else{
    misses++;
  }

  return hit;
}

i32 rgcache::write_cache(i32 low, i32 high, i64 data){
  i32 hit, hitway, hitway2;

  /*if (low == 4294952612 || high == 4294952612){
    printf("CH Write(%u-%u) called ", low, high);
    }*/
  
  // principle learned: merge/extend ranges as much as possible, create new range as a last resort
  //printf("writing %llu to %u-%u: ", data, low, high);

  // case b - start and end in 2 separate address ranges, create new aggregated range
  for (i32 i=0;i<nsets;i++){
    for (i32 j=0;j<nsets;j++){
      if ((lines[i].ha)+dsize==low && (lines[j].la)-dsize==high && lines[i].valid==1 && lines[j].valid==1 && (lines[i].value == lines[j].value) && (lines[i].value == data)){
	//printf("combining lines %u(%u-%u) and %u(%u-%u) to new range\n", i, lines[i].la, lines[i].ha, j, lines[j].la, lines[j].ha);
	/*if (low == 4294952612 || high == 4294952612 || i == 100 || j == 100){
	  printf("Writing (%u-%u) - aggregating 2 separate ranges %u(%u-%u) %u(%u-%u) to %u(%u-%u)\n", low, high, i, lines[i].la, lines[i].ha, j, lines[j].la, lines[j].ha, i, lines[i].la, lines[j].ha);
	  }*/
	lines[i].ha = lines[j].ha;
	lines[i].dirty = 1;
	lines[j].valid = 0;
	lines[j].dirty = 0;
	update_lru(i);
	// somehow make lines[j] available replacement next
	return 1;
      }
    }
  }

  // case c - input range extends one of existing ranges in either direction
  for (i32 i=0;i<nsets;i++){
    if (lines[i].la==high+dsize && lines[i].value==data && lines[i].valid==1){
      //printf("extended line %u(%u, %u) to (%u, %u)\n", i, lines[i].la, lines[i].ha, low, lines[i].ha);
      lines[i].la = low;
      lines[i].dirty = 1;
      update_lru(i);
      /*if (low == 4294952612 || high == 4294952612){
	printf("extend low of %u\n", i);
	}*/
      return 1;
    }
    else if (lines[i].ha==low-dsize && lines[i].value==data && lines[i].valid==1){
      //printf("extended line %u(%u, %u) to (%u, %u)\n", i, lines[i].la, lines[i].ha, lines[i].la, high);
      lines[i].ha = high;
      lines[i].dirty = 1;
      update_lru(i);
      /*if (low == 4294952612 || high == 4294952612){
	printf("extend high of %u\n", i);
	}*/
      return 1;
    }
  }

  // case a - new range lies within existing range
  for (i32 i=0;i<nsets;i++){
    if ((lines[i].la<=low) && (lines[i].ha>=high) && (lines[i].valid==1)){
      //printf("withing existing range, ");
      if (data == lines[i].value){
	// found it, don't do anything
	lines[i].dirty = 1;
	update_lru(i);
	/*if (low == 4294952612 || high == 4294952612){
	  printf("same value in existing range in %u\n", i);
	  }*/
	//printf("same value, no update\n");
	return 1;
      }else{
	// replace value in chunk
	if (lines[i].la == low && lines[i].ha == high){
	  lines[i].value = data;
	  lines[i].dirty = 1;
	  //printf("new value, same range\n");
	  update_lru(i);
	  /*if (low == 4294952612 || high == 4294952612){
	    printf("new value in existing range of %u\n", i);
	    }*/
	  return 1;
	}
	else if (lines[i].la == low){
	  // split into low (new) and high (truncated)
	  hitway = lru->val;
	  if (lines[hitway].valid == 1 && lines[hitway].dirty == 1){
	    writeback(hitway);
	  }
	  lines[i].la = high+dsize;	  
	  lines[hitway].la = low;
	  lines[hitway].ha = high;
	  lines[hitway].value = data;
	  lines[hitway].dirty = 1;
	  lines[hitway].valid = 1;
	  update_lru(hitway);
	  /*if (low == 4294952612 || high == 4294952612){
	    printf("split into low (%u new) and high (%u truncated)\n", i, hitway);
	    }*/
	  //printf("split into low (new) and high\n");
	  return 0;
	}
	else if (lines[i].ha == high){
	  // split into low (truncated) and high (new)
	  hitway = lru->val;
	  if (lines[hitway].valid == 1 && lines[hitway].dirty == 1){
	    writeback(hitway);
	  }
	  lines[i].ha = low-dsize;	  
	  lines[hitway].la = low;
	  lines[hitway].ha = high;
	  lines[hitway].value = data;
	  lines[hitway].dirty = 1;
	  lines[hitway].valid = 1;
	  update_lru(hitway);
	  /*if (low == 4294952612 || high == 4294952612){
	    printf("split into low (%u truncated) and high (%u new)\n", i, hitway);
	    }*/
	  return 0;
	}
	else {
	  // spit into 3 ranges, low (truncated), mid (new), high (truncated)
	  hitway = lru->val;
	  if (lines[hitway].valid == 1 && lines[hitway].dirty == 1){
	    writeback(hitway);
	  }
	  update_lru(hitway);
	  hitway2 = lru->val;
	  if (lines[hitway2].valid == 1 && lines[hitway2].dirty == 1){
	    writeback(hitway2);
	  }
	  update_lru(hitway2);
	  // old_low->low is old data
	  lines[hitway].la = lines[i].la;
	  lines[hitway].ha = low-dsize;
	  lines[hitway].value = lines[i].value;
	  lines[hitway].valid = 1;
	  // high->old_high is old data
	  lines[hitway2].la = high+dsize;
	  lines[hitway2].ha = lines[i].ha;
	  lines[hitway2].value = lines[i].value;
	  lines[hitway2].valid = 1;
	  // low->high is updated with data
	  lines[i].ha = low;
	  lines[i].la = high;
	  lines[i].value = data;
	  lines[i].dirty = 1;
	  //printf("split into low mid (new) and high\n");
	  /*if (low == 4294952612 || high == 4294952612){
	    printf("spit into 3 ranges, low (%u truncated), mid (%u new), high (%u truncated)\n", i, hitway, hitway2);
	    }*/
	  return 0;
	}
      }
    }
  }

  // case d - none of the end-points of the given range exist within any range in the range cache
  hitway = lru->val;
  if (lines[hitway].valid == 1 && lines[hitway].dirty == 1){
    writeback(hitway);
  }
  lines[hitway].la = low;
  lines[hitway].ha = high;
  lines[hitway].value = data;
  lines[hitway].dirty = 1;
  lines[hitway].valid = 1;
  update_lru(hitway);
  /*if (low == 4294952612 || high == 4294952612){
    printf("creating new range at %u (%u, %u)\n", hitway, low, high);
    printf("index(%u): %u-%u, valid(%u), dirty(%u)\n", 100, lines[100].la, lines[100].ha, lines[100].valid, lines[100].dirty);
    }*/
  return 0;
}

void rgcache::refill(i32 addr, i64 data){
  write_cache(addr, addr, data);
  totalrefills++;
}

i64 rgcache::read(i32 addr){
  i32 hitway = 0;
  i32 hit = 0;
  i64 value;

  // do the read
  for (i32 i=0;i<nsets;i++){
    if (lines[i].la <= addr && lines[i].ha >= addr && lines[i].valid == 1){
      hit = 1;
      hitway = i;
    }
  }

  if (hit == 1){
    hits++;
    value = lines[hitway].value;
#ifdef LOG
    fprintf(tlog, "%s\n", name);
#endif
  }else{
    fflush(stdout);
    assert(0);
    misses++;
    // refill takes care of replacement
  }

  accs++;
  this->update_lru(hitway);
  return value;
}

void rgcache::writeback(i32 index){
  i32 size;

  // L1 cache
  if (next_level != 0){
    size = 0;
    for (i32 i=lines[index].la;i<=lines[index].ha;i+=dsize){

      //printf("writing %llu to L2 at %u\n", lines[index].value, i);
      /*if (i == 4294941704){
	printf("Writing back (%llx) for addr (%x)\n", lines[index].value, i);
	}*/

      next_level->write(i, lines[index].value);
      //mem->write(i, lines[index].value);
      size += 8;
    }
    bwused += size;    
  }

  if (mem != 0){
    size = 0;
    for (i32 i=lines[index].la;i<=lines[index].ha;i+=dsize){

      //printf("writing %llu to memory at %u\n", lines[index].value, i);

      mem->write(i, lines[index].value);
      size += 8;
    }
    bwused += size;
  }

  /*if (lines[index].la == 4294952612 || lines[index].ha == 4294952612 || index == 100){
    printf("writing back index(%u) of (%u-%u)\n", index, lines[index].la, lines[index].ha);
    }*/

  lines[index].dirty = 0;
  totalwbs++;
}

void rgcache::touch(i32 addr){
  // updates LRU information if block is already in cache
  for (i32 i=0;i<nsets;i++){
    if (lines[i].la <= addr && lines[i].ha >= addr){
      update_lru(i);
    }
  }
}

void rgcache::stats(){
  printf("%s: %d entry range cache:\n", name, nsets);
  printf("%llu accesses, %llu hits, %llu misses, %llu writebacks\n", accs, hits, misses, totalwbs);
  printf("miss rate: %1.8f\n", (((double)misses)/(accs)));
  printf("total refills: %llu\n", totalrefills);
  printf("bandwidth used: %llu KB\n", (bwused >> 10));
}

void rgcache::clearstats(){
  this->accs = 0;
  this->hits = 0;
  this->misses = 0;
  this->bwused = 0;
  this->totalrefills = 0;
  this->totalwbs = 0;
}

void rgcache::set_anum(i32 v){
  anum = v;
}

void rgcache::subaccs(i32 addr, i32 num){
  // cleanup after a refill ( which does read)
  // check if either cache has line
  this->accs -= num;
  this->hits -= num;
}

i64 rgcache::get_accs(){
  return accs;
}

void rgcache::set_accs(i64 as){
  accs = as;
}

i64 rgcache::get_hits(){
  return hits;
}

void rgcache::set_hits(i64 hs){
  hits = hs;
}

void rgcache::set_mem(tmemory* sp){
  mem = sp;
}

void rgcache::set_map(mem_map* mp){
  map = mp;
}

void rgcache::set_name(char* cp){
  name = cp;
}

void rgcache::set_nl(tcache* cp){
  next_level = cp;
}

void rgcache::update_lru(i32 hitway){
  item * hitnode;
  item * node = lru;
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
    lru = node->next;
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
