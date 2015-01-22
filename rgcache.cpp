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
  i32 tmp;

#ifdef DBG
  if (addr == DBG_ADDR){
    printf("Annuling %x in %s\n", addr, name);
  }
#endif

  for (i32 i=0;i<nsets;i++){
    if (lines[i].la <= addr && lines[i].ha >= addr && lines[i].valid == 1){
#ifdef DBG
      if (lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR){
	printf("%s annul check(%u) for addr(%x), index(%u): low(%x) and high(%x) and valid(%u) matches\n", name, totalaccs, addr, i, lines[i].la, lines[i].ha, lines[i].valid);
      }
#endif
      hitway = i;
      break;
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
    if (lines[hitway].la == lines[hitway].ha){
      lines[hitway].valid = 0;
      lines[hitway].dirty = 0;
    }else{      
      lines[hitway].la += dsize;
    }
  }
  // case c: decrement high end of existing range     
  else if (lines[hitway].ha == addr && lines[hitway].valid == 1){
    /*if (addr == 4294952612){
      printf("Annul(%u): decrement high end of range(%u-%u)\n", addr, lines[hitway].la, lines[hitway].ha);
      }*/
    if (lines[hitway].la == lines[hitway].ha){
      lines[hitway].valid = 0;
      lines[hitway].dirty = 0;
    }else{      
      lines[hitway].ha -= dsize;
    }
  }
  // case d: split range into two ranges
  else if ((lines[hitway].la < addr) && (addr < lines[hitway].ha) && (lines[hitway].valid == 1)){
#ifdef DBG
    if (lines[hitway].la <= DBG_ADDR && lines[hitway].ha >= DBG_ADDR){
      printf("%s Annul(%u) for addr %x: splitting range %u(%x-%x) into 2 ranges: (%x-%x), (%x-%x)\n", name, totalaccs, addr, hitway, lines[hitway].la, lines[hitway].ha, lines[hitway].la, (addr-dsize), (addr+dsize), lines[hitway].ha);
    }
#endif
    tmp = lines[hitway].ha;
    lines[hitway].ha = (addr-dsize);
    write_cache(addr+dsize, tmp, lines[hitway].value);
  }
  // otherwise: do nothing (shouldn't be called)
  else{
    printf("Annul called and no action taken!\n");
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
      //break;
    }
#ifdef DBG
    if (addr == DBG_ADDR){
      printf("Check(%x): %s on entry(%u) with range(%x-%x)\n", addr, hit==1?"hit":"miss", i, lines[i].la, lines[i].ha);
    }
#endif
  }

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
  hit = 1; 
  write_cache(addr, addr, data);

  //accs++;
  /*if (hit == 1){
    hits++; // always hits... (make a new range)
  }else{
    misses++;
    }*/

  return hit;
}

void rgcache::write_cache(i32 low, i32 high, i64 data){
  i32 hitway, hitway2;

  // principle learned: merge/extend ranges as much as possible, create new range as a last resort
  // case b - start and end in 2 separate address ranges, merge and create new aggregated range
  for (i32 i=0;i<nsets;i++){
    for (i32 j=0;j<nsets;j++){
      if ((lines[i].ha)+dsize==low && (lines[j].la)-dsize==high && lines[i].valid==1 && lines[j].valid==1 && (lines[i].value == lines[j].value) && (lines[i].value == data) && i!=j){
#ifdef DBG
	if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR && lines[j].la <= DBG_ADDR && lines[j].ha >= DBG_ADDR)){
	  printf("Combining lines %u(%x-%x) and %u(%x-%x) to new range at %u(%x-%x) of %llx\n", i, lines[i].la, lines[i].ha, j, lines[j].la, lines[j].ha, i, lines[i].la, lines[j].ha, lines[i].value);
	}
#endif
	lines[i].ha = lines[j].ha;
	lines[i].dirty = 1;
	lines[j].valid = 0;
	lines[j].dirty = 0;
	update_lru(i);
	// need to annul any other indexes with this range set
	for (i32 k=0;k<nsets;k++){
	  if (k == j || k == i){
	    continue;
      }else if (lines[k].la <= low && lines[k].ha >= high){
	// this is not a complete solution, relies on assumption line k only caches 1 value, what about the rest?
	    lines[k].valid = 0;
	    lines[k].dirty = 0;
          }
        }
	return;
      }
      // lies within another range with a different value, which is absorbed after the write
      else if (lines[i].valid == 1 && lines[j].valid==1 && lines[j].value==data && high+dsize >= lines[j].la && high+dsize <= lines[j].ha && low >= lines[i].la && low <= lines[i].ha && i!=j){
#ifdef DBG
	if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR && lines[j].la <= DBG_ADDR && lines[j].ha >= DBG_ADDR)){
	printf("Write of %llx (%x, %x) Partially absorbing line %u(%x-%x) of %llx and extending %u(%x-%x) of %llx\n", data, low, high, i, lines[i].la, lines[i].ha, lines[i].value, j, lines[j].la, lines[j].ha, lines[j].value);
	}
#endif
	if (lines[i].la == low){ // absorbed after update
	  lines[i].valid = 0;
	  lines[i].dirty = 0;
	  lines[i].value = 0;
        }else{
	  lines[i].ha = low-dsize;
        }
	lines[j].la = low;
	lines[j].dirty = 1;
	update_lru(j);
	return;
      }
      else if (lines[i].valid == 1 && lines[j].valid==1 && lines[j].value==data && low-dsize >= lines[j].la && low-dsize <= lines[j].ha && high >= lines[i].la && high <= lines[i].ha && i!=j){
#ifdef DBG
	if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR && lines[j].la <= DBG_ADDR && lines[j].ha >= DBG_ADDR)){
	  printf("Write of %llx (%x, %x) Partially absorbing line %u(%x-%x) of %llx and extending %u(%x-%x) of %llx\n", data, low, high, i, lines[i].la, lines[i].ha, lines[i].value, j, lines[j].la, lines[j].ha, lines[j].value);
	}
#endif
	if (lines[i].ha == high){
	  lines[i].valid = 0;
	  lines[i].dirty = 0;
	  lines[i].value = 0;
        }else{
	  lines[i].la = high+dsize;
        }
	lines[j].ha = high;
	lines[j].dirty = 1;
	update_lru(j);
	return;
      }
    }
  }

  // case c - input range extends one of existing ranges in either direction
  for (i32 i=0;i<nsets;i++){
    if (lines[i].la == high+dsize && lines[i].value == data && lines[i].valid==1){
#ifdef DBG
	if (low == DBG_ADDR || high == DBG_ADDR){
	printf("Extending line %u(%x, %x) to (%x, %x)\n", i, lines[i].la, lines[i].ha, low, lines[i].ha);
	}
#endif
      lines[i].la = low;
      lines[i].dirty = 1;
      update_lru(i);
      return;
    }
    else if (lines[i].ha == low-dsize && lines[i].value == data && lines[i].valid==1){
#ifdef DBG
      if (low == DBG_ADDR || high == DBG_ADDR){
      printf("Extending line %u(%x, %x) to (%x, %x)\n", i, lines[i].la, lines[i].ha, lines[i].la, high);
    }
#endif
      lines[i].ha = high;
      lines[i].dirty = 1;
      update_lru(i);
      return;
    }
  }

  // case a - new range lies within existing range
  for (i32 i=0;i<nsets;i++){
    if ((lines[i].la <= low) && (lines[i].ha >= high) && lines[i].valid==1){
      //printf("withing existing range, ");
      if (data == lines[i].value){
#ifdef DBG
	if (low == DBG_ADDR || high == DBG_ADDR){
	  printf("Write(%u) of %llx to %x, same value in entry %u(%x, %x) of %llx, no update\n", totalaccs, data, low, i, lines[i].la, lines[i].ha, lines[i].value);
	}
#endif
	// found it, don't do anything
	lines[i].dirty = 1;
	update_lru(i);
	return;
      }else{
	// replace value in chunk
	if (lines[i].la == low && lines[i].ha == high){
#ifdef DBG
	  if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR)){
	    printf("New value of %llx, same range in %u(%x, %x) of %llx\n", data, i, lines[i].la, lines[i].ha, lines[i].value);
	  }
#endif
	  lines[i].value = data;
	  lines[i].dirty = 1;
	  update_lru(i);
	  return;
	}
	else if (lines[i].la == low){
	  // split into low (new) and high (truncated)
#ifdef DBG
	  if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR)){
	    printf("Write into %u (%x, %x) of %llx with (%x, %x) of %llx forcing split into 2 ranges\n", i, lines[i].la, lines[i].ha, lines[i].value, low, high, data);
	  }
#endif
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
	  return;
	}
	else if (lines[i].ha == high){
	  // split into low (truncated) and high (new)
#ifdef DBG
	  if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR)){
	    printf("Write into %u (%x, %x) of %llx with (%x, %x) of %llx forcing split into 2 ranges\n", i, lines[i].la, lines[i].ha, lines[i].value, low, high, data);
	  }
#endif
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
	  return;
	}
	else {
	  // spit into 3 ranges, low (truncated), mid (new), high (truncated)
#ifdef DBG
	  if (low == DBG_ADDR || high == DBG_ADDR || (totalaccs > DBG_ACCS && lines[i].la <= DBG_ADDR && lines[i].ha >= DBG_ADDR)){
	    printf("Write into %u(%x, %x) of %llx with (%x, %x) of %llx forcing split into 3 ranges\n", i, lines[i].la, lines[i].ha, lines[i].value, low, high, data);
	  }
#endif
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
	  lines[hitway].dirty = lines[i].dirty;
	  lines[hitway].valid = 1;
	  // high->old_high is old data
	  lines[hitway2].la = high+dsize;
	  lines[hitway2].ha = lines[i].ha;
	  lines[hitway2].value = lines[i].value;
	  lines[hitway2].dirty = lines[i].dirty;
	  lines[hitway2].valid = 1;
	  // low->high is updated with data
	  lines[i].ha = low;
	  lines[i].la = high;
	  lines[i].value = data;
	  lines[i].dirty = 1;
	  update_lru(i);
	  return;
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
#ifdef DBG
  if (low == DBG_ADDR || high == DBG_ADDR){
    printf("Creating new range at %u(%x, %x) with %llx\n", hitway, low, high, data);
      }
#endif
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
      break;
    }
  }

  if (hit == 1){
    hits++;
    value = lines[hitway].value;
#ifdef LOG
    fprintf(tlog, "%s\n", name);
#endif
  }else{
    printf("Read missed in RGCache\n");
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
#ifdef DBG
      if (i == DBG_ADDR){
	printf("Writing back (%llx) at %u(%x-%x) for addr (%x) to next_level\n", lines[index].value, index, lines[index].la, lines[index].ha, i);
      }
#endif
      next_level->write(i, lines[index].value);
      //mem->write(i, lines[index].value);
      size += 8;
    }
    bwused += size;    
  }
  else if (mem != 0){
    size = 0;
    for (i32 i=lines[index].la;i<=lines[index].ha;i+=dsize){
      //printf("Writing %llu to memory at %u\n", lines[index].value, i);
      mem->write(i, lines[index].value);
      size += 8;
    }
    bwused += size;
  }

  lines[index].dirty = 0;
  totalwbs++;
}

void rgcache::touch(i32 addr){
  // updates LRU information if block is already in cache
  for (i32 i=0;i<nsets;i++){
    if (lines[i].la <= addr && lines[i].ha >= addr){
      update_lru(i);
      break;
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
