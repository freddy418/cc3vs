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

#define RANGE 1<<28

unsigned int totalaccs;

#ifdef LOG
FILE *tlog;
#endif

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
  if (argc != 11){
    printf( "usage: %s (tagoffset) (mapon) (ranges) (blocks) (l2_assoc) (l2_sets) (bsize) (skip) (dir) benchmark\n", argv[0]);
    exit(1);
  }

  // bookkeeping
  totalaccs = 0;
  unsigned long mismatches = 0;

  // stack variables
  unsigned int tagoffset, mapon;
  unsigned int ranges; // number of ranges in RL1
  unsigned int blocks; // number of blocks in VL1
  unsigned int l2sets, l2assoc; // uncompressed L2 parameters
  unsigned int bsize, assoc, addr, zero, isRead;
  unsigned long long value;
  unsigned long long sval;
  unsigned int skip;

  tagoffset = atoi(argv[1]);
  mapon = atoi(argv[2]);
  ranges = atoi(argv[3]);
  blocks = atoi(argv[4]);
  l2assoc = atoi(argv[5]);
  l2sets = atoi(argv[6]);
  bsize = atoi(argv[7]);
  skip = atoi(argv[8]) * 1000000;

  // initialize cache and local variables;
  cltcache* dl1 = new cltcache(ranges, blocks, tagoffset);
  tcache* dl2 = new tcache(l2sets, l2assoc, bsize, tagoffset);
  mem_map* mp = new mem_map(mapon, 4096, bsize, 32, tagoffset); // added enable (0-off,1-on)
  tmemory* sp = new tmemory(tagoffset);
  FILE *in;

  dl1->set_nl(dl2);
  dl2->set_mem(sp);
  dl2->set_map(mp);
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

  char *buf, *buf1, *buf2;
  int bytes;

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
  if((dp  = opendir(argv[9])) == NULL) {
    cout << "Error (" << errno << ") unable to find directory " << argv[9] << endl;
    return errno;
  }
  while ((dirp = readdir(dp)) != NULL) {
    if ((strstr(dirp->d_name, argv[10]) != NULL) && (strstr(dirp->d_name, "log") != NULL)){
      fcnt++;
    }
  }
  closedir(dp);

  // hack to simplify debugging
  // fcnt = 1;

  if (fcnt == 0){
    fprintf(stderr, "No valid trace files of name %s found\n", argv[10]);
    exit(1);
  }

  buf = new char[64];
  buf1 = new char[256];
  buf2 = new char[256];

  for (unsigned int i = 0;i < fcnt;i++) {
    char file[512];
    sprintf(file, "%s/%s%d.log", argv[9], argv[10], i);
    in = fopen(file, "r");
    fprintf(stderr, "Reading from file %s\n", file);
    if (in == NULL){
      perror("Invalid file");
    }

    while(fgets(buf, 64, in)){
      //printf("socket (%d, %d): %s\n", bytes, errno, buf);
      sscanf(buf, "%s %x %s", buf1, &addr, buf2);
      value = strtoull(buf2, NULL, 16);
      
      isRead = strncmp(buf1, "read", 4);
      /*if (mp != 0){
	zero = mp->lookup(addr);
      }else{
	zero = 1; // do the lookup
	}*/
      //printf("result of lookup for address %08X in memmap: %d, is
      //read?: %d\n", addr, zero,       strncmp(buf1, "write", 5));
      dl1->set_anum(totalaccs);
      dl2->set_anum(totalaccs);

#ifdef DBG
      if (addr == DBG_ADDR){
	printf("CACHESIM(%u): %s, addr: %X, value: %llX, zero: %d\n", totalaccs, buf1, addr, value, zero);
      }
#endif

      if (isRead == 0){
        sval = dl1->read(addr);
	if (sval != value){
	  printf("Access(%u): Store and trace unmatched for addr (%x): s(%llX), t(%llX)\n", totalaccs, addr, sval, value);
	  assert(0);
	}
      }else{
	dl1->write(addr, value);
      }
      totalaccs++;

      // clear stats collected during warmup
      if (totalaccs == skip){
	dl1->clearstats();
	dl2->clearstats();
	if (mp != 0){
          mp->clearstats();
        }
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
  printf("Simulation complete after %u accesses\n", totalaccs);

  return 0;
}
