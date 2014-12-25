#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>
#include <iostream>
#include <algorithm>

#include "store.h"
#include "cltcache.h"
#include "tcache.h"
#include "memmap.h"

using namespace std;

#define OFFSET 1
#define RANGE 1<<16

#ifdef LOG
FILE *tlog;
#endif

//#define TEST 1

int bin2dec(char *bin)   
{
  int  b, k, m, n;
  int  len, sum = 0;
  len = strlen(bin) - 1;
  for(k = 0; k <= len; k++) 
    {
      n = (bin[k] - '0'); // char to numeric value
      if ((n > 1) || (n < 0)) 
	{
	  puts("\n\n ERROR! BINARY has only 1 and 0!\n");
	  return (0);
	}
      for(b = 1, m = len; m > k; m--) 
	{
	  // 1 2 4 8 16 32 64 ... place-values, reversed here
	  b *= 2;
	}
      // sum it up
      sum = sum + n * b;
      //printf("%d*%d + ",n,b);  // uncomment to show the way this works
    }
  return(sum);
}

int main(int argc, char** argv){
  if (argc != 9){
    printf( "usage: %s (ranges) (blocks) (l2_assoc) (l2_sets) (bsize) (skip) (dir) benchmark\n", argv[0]);
    exit(0);
  }

  // bookkeeping
  unsigned int lines = 0;
  unsigned long mismatches = 0;

  // stack variables
  unsigned int ranges; // number of ranges in RL1
  unsigned int blocks; // number of blocks in VL1
  unsigned int l2sets, l2assoc; // uncompressed L2 parameters
  unsigned int bsize, assoc, addr, zero, isRead;
  unsigned long value;
  unsigned long sval;
  unsigned int skip;

  ranges = atoi(argv[1]);
  blocks = atoi(argv[2]);
  l2assoc = atoi(argv[3]);
  l2sets = atoi(argv[4]);
  bsize = atoi(argv[5]);
  skip = atoi(argv[6]) * 1000000;

  // initialize cache and local variables;
  cltcache* dl1 = new cltcache(ranges, blocks, OFFSET);
  tcache* dl2 = new tcache(l2sets, l2assoc, bsize, OFFSET);
  mem_map* mp = new mem_map(0, 4096, bsize, 32, OFFSET); // added enable (0-off,1-on)
  tmemory* sp = new tmemory(OFFSET);
  FILE *in;

  dl1->set_nl(dl2);
  dl2->set_mem(sp);
  //dl2->set_map(mp);
  dl1->set_name("L1");
  dl2->set_name("L2");

  // single trace file implementation
  //FILE *in = fopen (argv[4], "r");
  // setup the socket connection
  /*int s, ns, len;
    struct sockaddr_un sock;
    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
    }

    sock.sun_family = AF_UNIX;
    strcpy(sock.sun_path, argv[10]);
    len = strlen(sock.sun_path) + sizeof(sock.sun_family);

    if (bind(s, (sockaddr*)&sock, len) == -1){
    perror("bind");
    exit(1);
    }
    printf("Attempting to listen on socket %d\n", s);
    listen(s, 5);
    printf("Waiting for connection on socket %d\n", s);
    if ((ns = accept(s, 0, 0)) == -1){
    perror("accept");
    exit(1);
    }
    printf("Connected!!\n\n");*/

#ifdef LOG
  char tf[512];
  //sprintf(tf, "%s/%s-taint.log", argv[6], argv[7]);
  sprintf(tf, "%s-taint.log", argv[8]);
  tlog = fopen(tf, "w");
  fprintf(stderr, "Writing accesses to file %s\n", tf);
  if (tlog == NULL){
    perror("Invalid file");
  }
#endif
     
#ifndef TEST
    
  unsigned int fcnt = 0;
  DIR *dp;
  struct dirent *dirp;
  if((dp  = opendir(argv[7])) == NULL) {
    cout << "Error(" << errno << ") opening " << argv[7] << endl;
    return errno;
  }
  while ((dirp = readdir(dp)) != NULL) {
    if ((strstr(dirp->d_name, argv[8]) != NULL) && (strstr(dirp->d_name, "log") != NULL)){
      fcnt++;
    }
  }
  closedir(dp);

  // hack to simplify debugging
  // fcnt = 1;

  if (fcnt == 0){
    fprintf(stderr, "No valid trace files of name %s found\n", argv[8]);
    exit(1);
  }

  char *buf, *buf1, *buf2;
  int bytes;

  buf = new char[64];
  buf1 = new char[256];
  buf2 = new char[256];

  for (unsigned int i = 0;i < fcnt;i++) {
    char file[512];
    sprintf(file, "%s/%s%d.log", argv[7], argv[8], i);
    in = fopen(file, "r");
    fprintf(stderr, "Reading from file %s\n", file);
    if (in == NULL){
      perror("Invalid file");
    }

    while(fgets(buf, 64, in)){
      //printf("socket (%d, %d): %s\n", bytes, errno, buf);
      sscanf(buf, "%s %x %s", buf1, &addr, buf2);
      value = strtoull(buf2, NULL, 16);
      addr = addr << OFFSET;
      //printf("type: %s, addr: %08X, value: %lX\n", buf1, addr, value);

      isRead = strncmp(buf1, "read", 4);
      if (mp != 0){
	zero = mp->lookup(addr);
      }else{
	zero = 1; // do the lookup
      }
      //printf("result of lookup for address %08X in memmap: %d, is
      //read?: %d\n", addr, zero,       strncmp(buf1, "write", 5));
      dl1->set_anum(lines);
      dl2->set_anum(lines);

      if (isRead == 0){
	// check the map first
	if (zero == 1){
	  sval = dl1->read(addr);
	}else{
	  sval = 0;
	}
	if (sval != value){
	  if (zero == 0){
	    if (mp != 0){
	      mp->update_block(addr, 1);
	    }
	    dl1->allocate(addr);
	    dl1->write(addr, value);
	    dl1->set_accs(dl1->get_accs() - 1);
	    dl1->set_hits(dl1->get_hits() - 1);
	  }else{
	    dl1->write(addr, value);
	    dl1->set_accs(dl1->get_accs() - 1);
	    dl1->set_hits(dl1->get_hits() - 1);	      
	    //printf("Access(%u): Store and trace unmatched for addr (%X): s(%lX), t(%lX)\n", lines, addr, sval, value);
	    mismatches++;
	  }
	}
	else{
	  if (sval == 0 && zero == 0 && mp != 0){
	    mp->get_tlb()->zeros++;
	  }
	}
      }else{
	if (zero == 0 && mp != 0){
	  mp->update_block(addr, 1);
	  //printf("Calling cache_allocate for %08X\n", addr);
	  dl1->allocate(addr); // special function to allocate a cache line with all zero
	}
	dl1->write(addr, value);
      }
      lines++;

      // clear stats collected during warmup
      if (lines == skip){
	dl1->clearstats();
	dl2->clearstats();
	mp->clearstats();
      }
    }
  }

#else

  printf("Starting cache sweep test\n");
  unsigned int count = 0;
  unsigned int matches = 0;

  printf("Setting Memory Values\n");
  for (unsigned int i=0;i<RANGE;i+=4){
    unsigned long long data = ((i>>2) % 32) > 30;
    //printf ("Writing (%llx) to address (%X)\n", data, i);    
    dl1->write(i, data);
  }
    
  printf("Reading back Memory Values\n");
  for (unsigned int i=0;i<RANGE;i+=4){
    unsigned long long exp = ((i>>2) % 32) > 30;
    unsigned long long act = dl1->read(i);
    if (exp != act){
      printf ("Data mismatch for address (%X), actual(%llX), expected(%llX)\n", i, act, exp);
      count++;
    }else{
      matches++;
    }

    if (count > 20){
      printf ("FAILED: too many read errors\n");
      exit(1);
    }
  }
  printf("PASSED: %u accesses matched\n", matches);
    
#endif

  if (mp != 0){
    mp->stats();
  }
  if (dl1 != 0){
    dl1->stats();
  }
  if (dl2 != 0){
    dl2->stats();
  }

  printf("%lu initialization mismatches encountered\n", mismatches);
  printf("Simulation complete after %u accesses\n", lines);

  return 0;
}
