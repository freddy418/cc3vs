#include "store.h"

// create the pointers
tmemory::tmemory(i32 os){
  //printf("Entering create_memory\n");
  i32 pgs = 1<<(20+os);
  pages = new tpage*[pgs];
  for (i32 i=0;i<pgs;i++){
    pages[i] = 0;
  }
  pmask = pgs - 1;
  pshift = 12-os;
  fmask = (1<<pshift) - 1;
  ishift = 3 - os; // 3 bits for 64b values
  //printf("pages: %d, page mask: %08X, frame mask: %08X\n", pages, pmask, fmask);

  //printf("Leaving create_memory\n");
}

i64 tmemory::read(i32 addr){
  //printf("Entering mem_read\n");
  i32 fnum = (addr >> pshift) & pmask;
  i32 findex = addr & fmask;

  if (pages[fnum] == 0){
    //printf("Leaving mem_read\n");
    return 0;
  }else{
    //printf("Leaving mem_read\n");
    return pages[fnum]->data[findex>>(ishift)];
  }
}

void tmemory::write(i32 addr, i64 data){ 
  //printf("Entering mem_write\n");
  i32 fnum = (addr >> pshift) & pmask;
  i32 findex = addr & fmask;

  if (pages[fnum] == 0){
    pages[fnum] = new tpage();
  }
  pages[fnum]->data[findex>>(ishift)] = data;
  //printf("Leaving mem_write\n");
}
