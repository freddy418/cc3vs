#include "cltcache.h"

cltcache::cltcache(i32 ncs, i32 nus, i32 ofs){
  if (ncs > 0){
    cl2 = new rgcache(ncs, ofs);
    cl2->set_name("RG");
  }else{
    cl2 = 0;
  }
  if (nus > 0){
    ul2 = new ccache(1, nus, 8, ofs);
    ul2->set_name("VL");
  }else{
    cl2 = 0;
  }

  oshift = 3 - ofs;
  os = ofs;
  next_level = 0;

  // update statistics
  accs = 0;
  hits = 0;
  misses = 0;
  bwused = 0;
  totalrefills = 0;
  cpbrefills = 0;
  totalwbs = 0;
  cpbwbs = 0;
  mem = 0;
}

// TODO:
void cltcache::allocate(i32 addr){
  // allocate in L2: is there a better way?
  next_level->allocate(addr);
}

void cltcache::write(i32 addr, i64 data){
  i32 uh, ch;
  i32 hit = 0;
  
  // TODO: check if value already exists in either cache and write one, invalidate the other
  if (cl2 != 0){
    ch = cl2->check(addr);
  }else{
    ch = 0;
  }
  if (ul2 != 0){
    uh = ul2->check(addr);
  }else{
    uh = 0;
  }

#ifdef DBG
  if (addr == DBG_ADDR){
    printf("Writing %llx to addr(%x): ch(%u) and uh(%u)\n", data, addr, ch, uh);
  }
#endif

  if (ch == 1 && uh ==1){
    printf("CL2(%u) and UL2(%u) must be mutually exclusive for addr(%u)!\n", ch, uh, addr);
    assert(0);
  }

  if (data == 0){
    /*if (addr == 4294954324){
      printf("Writing %llx to %x in cl2\n", data, addr);
      }*/
    hit = cl2->write(addr, data);
    if (uh == 1){
      /*if (addr == 4294954324){
	printf("Annuling %x in ul2\n", addr);
	}*/
      ul2->annul(addr);
    }
  }else{
    /*if (addr == 2923986208){
      printf("Writing %llx to %x in ul2\n", data, addr);
      }*/
    hit = ul2->write(addr, data);
    if (ch == 1){
      /*if (addr == 2923986208){
	printf("Annuling %x in cl2\n", addr);
	}*/
      cl2->annul(addr);
    }
  }

  if (hit == 1){
    hits++;
  }else{
    misses++;
  }
  accs++;
}

i32 cltcache::refill(i32 addr){
  i64 data, level;

  if (next_level != 0){
    data = next_level->read(addr);
    level = 2;
    bwused += 8;
  }
  else if (mem != 0){
    data = mem->read(addr);
    level = 3;
    bwused += 8;
  }
  else{
    level = 0;
    data = 0;
  }
    
#ifdef DEBUG
  printf("Refilling from next_level for address %X, read %8llX, level %d\n", addr, data, level);
    fflush(stdout);
#endif
  // send to cl2 or ul2 based on compressibility
  if (data == 0){
    //printf("refilling to range cache\n");
    cl2->refill(addr, data);
  }else{
    //printf("refilling to value cache\n");
    ul2->refill(addr, data);
  }

  totalrefills++;
  return (data == 0);
}

i64 cltcache::read(i32 addr){
  i32 ch, uh, compressible;;
  i64 value;

  // send read to both caches
  ch = cl2->check(addr);
  uh = ul2->check(addr);

#ifdef DBG
  if (addr == DBG_ADDR){
    printf("Read(%x) from ch(%u) and uh(%u)\n", addr, ch, uh);
  }
#endif

  // handle 2 cases when either compressed or uncompressed hits
  if (ch == 1 && uh == 0){    
#ifdef DEBUG
    printf("cltcache check: rg(%d), v(%d), calling range cache read\n", ch, uh);
#endif
    value = cl2->read(addr);
    hits++;
  }
  else if (ch == 0 && uh == 1){    
#ifdef DEBUG
    printf("cltcache check: rg(%d), v(%d), calling value cache read\n", ch, uh);
#endif
    value = ul2->read(addr);
    hits++;
  }
  else if (ch == 0 && uh == 0){
    // refill line from memory;
    compressible = refill(addr);
    // retry the access
    if (compressible == 0){
#ifdef DBG
      if (addr == DBG_ADDR){
	printf("Refilled to value cache, calling UL2 read\n");
      }
#endif
      value = ul2->read(addr);
    }else{
#ifdef DBG
      if (addr == DBG_ADDR){
	printf("Refilled to range cache, calling CL2 read\n");
      }
#endif
      value = cl2->read(addr);
    }
    misses++;
  }
  else {
    printf("CL2(%u) and UL2(%u) must be mutually exclusive for addr(%u)!\n", ch, uh, addr);
    assert(0);
  }

  accs++;
  return value;
}

void cltcache::touch(i32 addr){
  // updates LRU information if block is already in cache

  // check if either cache has line
  i32 ch = cl2->check(addr);
  i32 uh = ul2->check(addr);

  if (ch == 0 && uh == 1){
    ul2->touch(addr);  
  }
  else if (ch == 1 && uh == 0){
    cl2->touch(addr);
  }
  else if (ch == 0 && uh == 0){
    // don't need to update LRU of nonexistent cache line
  }
  else if (ch == 1 && uh == 1){
    printf("CL2 and UL2 must be mutually exclusive!\n");
    exit(1);
  }
}

void cltcache::stats(){
  printf("Unified %s Cache\n", name);
  printf("%llu accesses, %llu hits, %llu misses\n", accs, hits, misses);
  printf("miss rate: %1.8f\n", (((double)misses)/(accs)));
  printf("bandwidth used: %llu KB\n", ((bwused + ul2->getubw()) >> 10));
  printf("total refills: %llu\n", totalrefills);
  printf("compressible refills: %llu\n", cpbrefills);
  printf("total writebacks: %llu\n", totalwbs);
  printf("compressible writebacks: %llu\n", cpbwbs);

  cl2->stats();
  ul2->stats();
}

void cltcache::clearstats(){
  this->accs = 0;
  this->hits = 0;
  this->misses = 0;
  this->bwused = 0;
  this->totalrefills = 0;
  this->cpbrefills = 0;
  this->totalwbs = 0;
  this->cpbwbs = 0;
  cl2->clearstats();
  ul2->clearstats();
}

void cltcache::set_anum(i32 v){
  anum = v;
  cl2->set_anum(v);
  ul2->set_anum(v);
}

void cltcache::subaccs(i32 addr, i32 num){
  // cleanup after a refill ( which does read)
  // check if either cache has line
  i32 ch = cl2->check(addr);
  i32 uh = ul2->check(addr);

  if (ch == 0 && uh == 1){
    //printf("Reducing UL2 access by %u\n", num);
    ul2->set_accs(ul2->get_accs() - num);
    ul2->set_hits(ul2->get_hits() - num);
  }
  else if (ch == 1 && uh == 0){
    //printf("Reducing CL2 access by %u\n", num);
    cl2->set_accs(cl2->get_accs() - num);
    cl2->set_hits(cl2->get_hits() - num);
  }
  else if (ch == 0 && uh == 0){
    // don't do anything
  }
  else if (ch == 1 && uh == 1){
    printf("CL2 and UL2 must be mutually exclusive!\n");
    exit(1);
  }
}

i64 cltcache::get_accs(){
  return accs;
}

void cltcache::set_accs(i64 as){
  accs = as;
}

i64 cltcache::get_hits(){
  return hits;
}

void cltcache::set_hits(i64 hs){
  hits = hs;
}

void cltcache::set_mem(tmemory* sp){
  mem = sp;
  ul2->set_mem(sp);
  cl2->set_mem(sp);
}

void cltcache::set_map(mem_map* mp){
  map = mp;
  ul2->set_map(mp);
  cl2->set_map(mp);
}

void cltcache::set_name(char *cp){
  name = cp;
}

void cltcache::set_nl(tcache* cp){
    next_level = cp;
    ul2->set_nl(cp);
    cl2->set_nl(cp);
}
