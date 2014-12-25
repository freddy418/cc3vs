/**

Copyright 2007
Georgia Tech Research Corporation
Atlanta, GA  30332-0415
All Rights Reserved

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

   * Redistributions of source code must retain the above copyright
   * notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   * copyright notice, this list of conditions and the following
   * disclaimer in the documentation and/or other materials provided
   * with the distribution.

   * Neither the name of the Goergia Tech Research Coporation nor the
   * names of its contributors may be used to endorse or promote
   * products derived from this software without specific prior
   * written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

**/

#include "syscall_functions.h"
#include "dytan.h"
/*
  Maintain a convienience map of file id -> file names
*/
map<int, string> openFiles;

void UnimplementedSystemCall(INT32 num, syscall_arguments args, void * v)
{
  if(SYS_nanosleep == num) return;

  fprintf(log, "system call %d unimplemented\n", num);
  //printf("system call %d unimplemented\n", num);
  //abort();
}

void Handle_ACCESS(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_ALARM(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_BRK(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_CHMOD(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_CLOSE(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_DUP(INT32 num, syscall_arguments args, void * v)
{
  int fd1 = (int) args.arg0;
  int fd2 = (int) args.ret;

  openFiles[fd2] = openFiles[fd1];
}

void Handle_FCNTL64(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_FLOCK(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

// Clear taint
void Handle_FSTAT64(INT32 num, syscall_arguments args, void * v)
{
  struct stat64 *buf = (struct stat64 *) args.arg1;
  ADDRINT start = (ADDRINT) buf;
  ADDRINT end = start + sizeof(struct stat64);

  // remove taint
  for(ADDRINT addr = start; addr < end; addr++) {

    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }
}

void Handle_FSYNC(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_FTRUNCATE(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

// Clear taint
void Handle_GETDENTS64(INT32 num, syscall_arguments args, void * v)
{
  struct dirent64 *dirp = (struct dirent64 *) args.arg1;
  unsigned int count = (unsigned int) args.arg2;

  ADDRINT start = (ADDRINT) dirp;
  ADDRINT end = start + count;

  for(ADDRINT addr = start; addr < end; addr++) {

    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }
}

void Handle_GETPID(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

// Clear taint
void Handle_GETTIMEOFDAY(INT32 num, syscall_arguments args, void * v)
{
  struct timeval *tv = (struct timeval *) args.arg0;
  ADDRINT tv_start = (ADDRINT) tv;
  ADDRINT tv_end = tv_start + sizeof(struct timeval);

  for(ADDRINT addr = tv_start; addr < tv_end; addr++) {
    //remove taint from addr 
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }

  struct timezone *tz = (struct timezone *) args.arg1;
  ADDRINT tz_start = (ADDRINT) tz;
  ADDRINT tz_end = tz_start + sizeof(struct timezone);

  for(ADDRINT addr = tz_start; addr < tz_end; addr++) {
    //remove taint from addr 
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }
  
}

void Handle_GETUID32(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_IOCTL(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_LINK(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_LLSEEK(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_LSEEK(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

// Clear taint
void Handle_LSTAT64(INT32 num, syscall_arguments args, void * v)
{
  struct stat64 *buf = (struct stat64 *) args.arg1;
  ADDRINT start = (ADDRINT) buf;
  ADDRINT end = start + sizeof(struct stat64);
  
  for(ADDRINT addr = start; addr < end; addr++) {
    //remove taint from addr 
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }
}

// Clear taint
void Handle_MMAP(INT32 num, syscall_arguments args, void * v)
{
  ADDRINT start = (ADDRINT) args.ret;
  ADDRINT end = start + (size_t) args.arg1;
  
  for(ADDRINT addr = start; addr < end; addr++) {
    //remove taint from addr 
     map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  } 
}

// Clear taint
void Handle_MMAP2(INT32 num, syscall_arguments args, void * v)
{
  ADDRINT start = (ADDRINT) args.ret;
  ADDRINT end = start + (size_t) args.arg1;
  
  for(ADDRINT addr = start; addr < end; addr++) {
    //remove taint from addr
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  } 
}

void Handle_MUNMAP(INT32 num, syscall_arguments args, void * v)
{
}

void Handle_MPROTECT(INT32 num, syscall_arguments args, void * v)
{
}

void Handle_OPEN(INT32 num, syscall_arguments args, void * v)
{

  openFiles[args.ret] = (const char *) args.arg0;
}


/*
  The read system call taints memory from the start of the second parameter
  to the start of the second parameter + the return value
 */
void Handle_READ(INT32 num, syscall_arguments args, void * v)
{
  ADDRINT fd = args.arg0;
  char *buf = (char *) args.arg1;
  int ret = args.ret;

  ADDRINT start = (ADDRINT) buf;
  ADDRINT end = start + ret;

  //bail if nothing was actually assigned to memory
  if(ret <= 0) return;
  
  bitset2 *s = bitset_init(NUMBER_OF_TAINT_MARKS);
  bitset_set_bit(s, currentTaintMark);
  
#ifdef TRACE
  if (tracing) {
    const char *sep = "";
    printf("**** New Taint [");
    printf("%s] assigned from SYS_READ(%x) to ADDR 0x%x - 0x%x **** \n", bitset_str(s), fd, start, end);
    fprintf(log, "**** New Taint [");
    fprintf(log, "%s] assigned from SYS_READ(%x) to ADDR 0x%x - 0x%x **** \n", bitset_str(s), fd, start, end);
    /*for(uint i = 0; i < s->nbits; i++) {
      if(bitset_test_bit(s, i)) {
	fprintf(log, "%s%d", sep, i);
	printf("%s%d", sep, i);
	sep = ", ";
      }
    }
    fprintf(log, "] assigned from SYS_READ(%x) to ADDR 0x%x - 0x%x **** \n", fd, start, end);
    printf("] assigned from SYS_READ(%x) to ADDR 0x%x - 0x%x **** \n", fd, start, end);*/
  }
#endif
  
  for(ADDRINT addr = start; addr < end; addr++) {
    memTaintMap[addr] = bitset_copy(s);
  }
  bitset_free(s);
  
  off_t currentOffset = lseek(args.arg0, 0, SEEK_CUR);
  fprintf(taintAssignmentLog, "%d - %s[%d-%#d] -> %#x-%#x\n",
	  currentTaintMark, 
	  openFiles[args.arg0].c_str(), 
	  currentOffset - args.ret, 
	  currentOffset,
	  start, end-1);
  fflush(taintAssignmentLog);
  
  
#ifdef TRACE
  if(tracing) {
    fprintf(log, "\t%#x-%#x <- read(%d)\n",
	    start, end - 1,
	    currentTaintMark);
    fflush(log); 
  }
#endif
  
  currentTaintMark++;
  //if(currentTaintMark == NUMBER_OF_TAINT_MARKS) currentTaintMark = 0;
}


// Clear taint
void Handle_READLINK(INT32 num, syscall_arguments args, void * v)
{
  char *buf = (char *) args.arg1;
  int ret = args.ret;

  ADDRINT start = (ADDRINT) buf;
  ADDRINT end = start + ret;

  for(ADDRINT addr = start; addr < end; addr++) {
    //remove taint from addr   
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }

  }
}

void Handle_RENAME(INT32 num, syscall_arguments args, void * v)
{
  //pass
}


// Clear taint
void Handle_RT_SIGACTION(INT32 num, syscall_arguments args, void * v)
{
  struct sigaction *oldact = (struct sigaction *) args.arg2;

  if(NULL != oldact) {
    ADDRINT start = (ADDRINT) oldact;
    ADDRINT end = start + sizeof(struct sigaction);
    
    for(ADDRINT addr = start; addr < end; addr++) {
      //remove taint from addr 
      map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
      
      if(memTaintMap.end() != iter) {
	bitset_free(iter->second);
	memTaintMap.erase(iter);
      }
    }
  }
}


// Clear taint
void Handle_RT_SIGPROCMASK(INT32 num, syscall_arguments args, void * v)
{
  sigset_t *oldset = (sigset_t *) args.arg2;
  
  if(NULL != oldset) {
    ADDRINT start = (ADDRINT) oldset;
    ADDRINT end = start + sizeof(sigset_t);
    
    for(ADDRINT addr = start; addr < end; addr++) {
      //remove taint from addr 
      map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
      
      if(memTaintMap.end() != iter) {
	bitset_free(iter->second);
	memTaintMap.erase(iter);
      }
    }
  }
}

void Handle_SET_THREAD_AREA(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_SOCKETCALL(INT32 num, syscall_arguments args, void * v)
{
  switch((int) args.arg0) {
  case SYS_SOCKET:
    //pass
    break;
  case SYS_CONNECT:
    //pass
    break;
  case SYS_BIND:
  case SYS_LISTEN:
  case SYS_ACCEPT:
  case SYS_GETSOCKNAME:
  case SYS_GETPEERNAME:
  case SYS_SEND:
  case SYS_RECV:
  case SYS_SENDTO:
  case SYS_RECVFROM:
  case SYS_SHUTDOWN:
  case SYS_SETSOCKOPT:
  case SYS_GETSOCKOPT:
  case SYS_SENDMSG:
  case SYS_RECVMSG:
  default:
    fprintf(log, "Unhandled socketcall %d\n", args.arg0);
    printf("Unhandled socketcall %d\n", args.arg0);
    //abort();
  }
}

// Clear taint
void Handle_STAT64(INT32 num, syscall_arguments args, void * v)
{
  struct stat64 *buf = (struct stat64 *) args.arg1;
  ADDRINT start = (ADDRINT) buf;
  ADDRINT end = start + sizeof(struct stat64);
  
  for(ADDRINT addr = start; addr < end; addr++) {
    //remove taint from addr 
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }
}

// Clear taint
void Handle_TIME(INT32 num, syscall_arguments args, void * v)
{
  time_t *t = (time_t *) args.arg0;
  if(NULL != t) {
    ADDRINT start = (ADDRINT) t;
    ADDRINT end = start + sizeof(time_t);
    
    for(ADDRINT addr = start; addr < end; addr++) {
      //remove taint from addr 
      map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
      
      if(memTaintMap.end() != iter) {
	bitset_free(iter->second);
	memTaintMap.erase(iter);
      }
    }
  }
}

// Clear taint
void Handle_UNAME(INT32 num, syscall_arguments args, void * v)
{
  struct utsname *buf = (struct utsname *) args.arg0;

  ADDRINT start = (ADDRINT) buf;
  ADDRINT end = start + sizeof(struct utsname);
  
  for(ADDRINT addr = start; addr < end; addr++) {
    //remove taint from addr 
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);

    if(memTaintMap.end() != iter) {
      bitset_free(iter->second);
      memTaintMap.erase(iter);
    }
  }
}

void Handle_UNLINK(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_UTIME(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_WRITE(INT32 num, syscall_arguments args, void * v)
{
  //pass
}

void Handle_WRITEV(INT32 num, syscall_arguments args, void * v)
{
  //pass
}
