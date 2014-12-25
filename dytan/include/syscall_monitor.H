/************************************************
 *      Copyright 2007                          *
 *      Georgia Tech Research Corporation       *
 *      Atlanta, GA  30332-0415                 *
 *      All Rights Reserved                     *
 ************************************************/

#ifndef __SYSCALL_MONITOR_H
#define __SYSCALL_MONITOR_H

#include <string.h>
#include <syscall.h>
#include <stdio.h>
#include "pin.H"

#include <map>
#include <vector>

typedef struct
{
  ADDRINT num;

  ADDRINT arg0;
  ADDRINT arg1;
  ADDRINT arg2;
  ADDRINT arg3;
  ADDRINT arg4;
  ADDRINT arg5;

  ADDRINT ret;
  ADDRINT err;
} syscall_arguments;

typedef void (*SyscallMonitorCallback)(INT32, syscall_arguments, void *);

class SyscallMonitor {

public:
  SyscallMonitor()
  {

  }
  
  void activate()
  {
    //InitLock(&lock);
    PIN_AddSyscallEntryFunction(SysBefore, this);
    PIN_AddSyscallExitFunction(SysAfter, this);
  }
  
  void addObserver(INT32 syscall_number, SyscallMonitorCallback callback, void * v)
  {
    observers[syscall_number].push_back(pair<SyscallMonitorCallback, void *>(callback, v));
  }
  
  void setDefaultObserver(SyscallMonitorCallback callback)
  {
    defaultObserver = callback;
  }

  void beginSyscall(UINT32 tid, INT32 num, 
		    ADDRINT arg0, ADDRINT arg1, ADDRINT arg2,
		    ADDRINT arg3, ADDRINT arg4, ADDRINT arg5)
  {
    //GetLock(&lock, 1);
    pendingSyscall = (syscall_arguments) {num, arg0, arg1, arg2, arg3, arg4, arg5};
    //ReleaseLock(&lock);
  }

  void endSyscall(UINT32 tid, CONTEXT *ctxt, SYSCALL_STANDARD std) //ADDRINT ret, ADDRINT err)
  {
    //GetLock(&lock, 1);
    syscall_arguments args = pendingSyscall;

    args.ret = PIN_GetSyscallReturn(ctxt, std);
    args.err = PIN_GetSyscallErrno(ctxt, std);

    if(observers.find(args.num) != observers.end()) {      
      for(vector<pair<SyscallMonitorCallback, void *> >::iterator iter = observers[args.num].begin(); iter != observers[args.num].end(); iter++) {
	iter->first(args.num, args, iter->second);
      }
    }
    else {
      if(defaultObserver) {
	defaultObserver(args.num, args, 0);
      }
    }
    //ReleaseLock(&lock);
  }
  
private:
  
  syscall_arguments pendingSyscall;
  map<int, vector< pair<SyscallMonitorCallback, void *> > > observers;
  
  SyscallMonitorCallback defaultObserver;

  //PIN_LOCK lock;
    
  static void SysBefore(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v){
    SyscallMonitor *monitor = static_cast<SyscallMonitor *>(v);
    monitor->beginSyscall(tid, PIN_GetSyscallNumber(ctxt, std),
                          PIN_GetSyscallArgument(ctxt, std, 0),
                          PIN_GetSyscallArgument(ctxt, std, 1),
                          PIN_GetSyscallArgument(ctxt, std, 2),
                          PIN_GetSyscallArgument(ctxt, std, 3),
                          PIN_GetSyscallArgument(ctxt, std, 4),
                          PIN_GetSyscallArgument(ctxt, std, 5));
  }
  
  static VOID SysAfter(THREADID tid, CONTEXT *ctxt, SYSCALL_STANDARD std, VOID *v){
    SyscallMonitor *monitor = static_cast<SyscallMonitor *>(v);
    monitor->endSyscall(tid, ctxt, std);
  }
  
}; 
#endif
