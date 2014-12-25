/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2007 Intel Corporation 
All rights reserved. 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

#include "pin.H"
#include "instlib.H"
#include "time_warp.H"
#include "portability.H"
#include <vector>
#include <iostream>
#include <iomanip>
#include <fstream>

#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <string.h>

#define ENABLED 1
#define MAP 0
#define ARRONLY 1
#define TRACE2 1
#define OFFSET 1
#define FSIZE 1<<25

#ifdef MAP
   /* Map instantiation */
map<uint8_t *, uint8_t> Memory_map;
map<uint8_t *, uint8_t>::iterator it;
#endif

// map of allocated blocks (keys are start address of object, values
// are range of the object)
// moved into memory store object

map<ADDRINT, ADDRINT> Object_map;
pair<map<uint8_t *, uint8_t>::iterator,bool> ret;

FILE * fout;

VOID * lastEa;
uint64_t * lastEa2;
uint64_t lastValue2;
uint64_t * lastEa3;
uint64_t lastValue3;
uint64_t * lastEa4;
uint64_t lastValue4;
uint64_t * lastEa5;
uint64_t lastValue5;
uint64_t * lastEa6;
uint64_t lastValue6;
uint64_t * lastEa7;
uint64_t lastValue7;
uint64_t * lastEa8;
uint64_t lastValue8;
uint64_t lastEa_bit;

VOID Fini(int code, VOID * v);

ADDRINT stored_rsp;
INSTLIB::ICOUNT icount;
UINT32 accs;
UINT32 StartCount, StopCount;
UINT32 fnum;

KNOB<UINT32> KnobStartAt(KNOB_MODE_WRITEONCE, "pintool",
			 "s", "0", "early simpoint");
KNOB<UINT32> KnobStopAt(KNOB_MODE_WRITEONCE, "pintool",
                        "q", "2000", "number of instructions to instrument");
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
			    "o", "dt", "specify output file name");

/* ===================================================================== */

INT32 Usage()
{
    cerr <<
        "This pin tool monitors all memory acccesses made by an application\n"
        "\n";

    cerr << KNOB_BASE::StringKnobSummary();

    cerr << endl;

    return -1;
}

using namespace INSTLIB;

LOCALVAR VOID *WriteEa[PIN_MAX_THREADS];

VOID CaptureWriteEa(THREADID threadid, VOID * addr)
{
    WriteEa[threadid] = addr;
}

// Replace calls to the function salloc, which is used to indicate the size
// of objects allocated on the stack in the instrumented application and
// update the object map after each call
VOID salloc(ADDRINT start, ADDRINT chunk, ADDRINT end){
  ADDRINT size = (end-start) + ((chunk-start)-1);
  pair<map<ADDRINT, ADDRINT>::iterator,bool> iret;  
  
  iret = Object_map.insert(pair<ADDRINT, ADDRINT>((ADDRINT)start, ((ADDRINT)start)+size) );
  if (iret.second==false){
    Object_map.erase((ADDRINT)start);
    Object_map.insert( pair<ADDRINT, ADDRINT>((ADDRINT)start, (ADDRINT)start+size) );
  }
}

// this should be called whenever the stack pointer is moved
// deletes all stack allocated objects that resided in the frame
// of the returning function
VOID sfree(ADDRINT rsp){
  int diff = rsp - stored_rsp;
  
  if (diff > 0){
    //cout << "program stack moved back by " << diff << " bytes to " << hex << rsp << endl;
    for(ADDRINT i = rsp; i > stored_rsp; i--){
      //cout << "deleting object at " << hex << i << endl;
      Object_map.erase((ADDRINT)i);
    }
  }

  stored_rsp = rsp;
}

// Replace calls to the library function malloc and update
// the object map after each call
VOID * Jit_Malloc_IA32( CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0)
{
  //cout << "Jit_Malloc_IA32(" << hex << (ADDRINT) orgFuncptr << ", " 
  //       << hex << arg0 << ") " 
  //       << dec << endl;
  
  VOID * ret;
  pair<map<ADDRINT, ADDRINT>::iterator,bool> iret;
  
  PIN_CallApplicationFunction( context, PIN_ThreadId(),
			       CALLINGSTD_DEFAULT, orgFuncptr,
			       PIN_PARG(void *), &ret,
			       PIN_PARG(size_t), arg0,
			       PIN_PARG_END() );
  
  //cout << "return value = " << hex << (ADDRINT)ret << dec << endl;
  
  iret = Object_map.insert ( pair<ADDRINT, ADDRINT>((ADDRINT)ret, ((ADDRINT)ret)+arg0) );
  if (iret.second==false){
    Object_map.erase((ADDRINT)ret);
    Object_map.insert ( pair<ADDRINT, ADDRINT>((ADDRINT)ret, (ADDRINT)ret+arg0) );
  }
  
  return ret;
}

// Replace calls to the library function malloc and update
// the object map after each call
VOID * Jit_Calloc_IA32( CONTEXT * context, AFUNPTR orgFuncptr, size_t arg0, size_t arg1)
{
  cerr << "Jit_Calloc_IA32(" << hex << (ADDRINT) orgFuncptr << ", " 
       << hex << arg0 << ", " << hex << arg1 << ") " 
       << dec << endl;
  
  VOID * ret;
  pair<map<ADDRINT, ADDRINT>::iterator,bool> iret;
  
  PIN_CallApplicationFunction( context, PIN_ThreadId(),
                               CALLINGSTD_DEFAULT, orgFuncptr,
                               PIN_PARG(void *), &ret,
                               PIN_PARG(size_t), arg0,
                               PIN_PARG(size_t), arg1,
                               PIN_PARG_END() );
  
  cerr << "return value of calloc = " << hex << (ADDRINT)ret << dec << endl;
  
  iret = Object_map.insert ( pair<ADDRINT, ADDRINT>((ADDRINT)ret, ((ADDRINT)ret)+(arg0*arg1)) );
  if (iret.second==false){
    Object_map.erase((ADDRINT)ret);
    Object_map.insert ( pair<ADDRINT, ADDRINT>((ADDRINT)ret, (ADDRINT)ret+(arg0*arg1)) );
  }
  
  return ret;
}

// Replace calls to the library function free and update the object
// map aftet each call
VOID Jit_Free_IA32( CONTEXT * context, AFUNPTR orgFuncptr, void * ptr)
{
  //cout << "Jit_Free_IA32(" << hex << (ADDRINT) orgFuncptr << ","
  //       << hex << ptr << ") " 
  //       << dec << endl;

  PIN_CallApplicationFunction( context, PIN_ThreadId(),
			       CALLINGSTD_DEFAULT, orgFuncptr,
			       PIN_PARG(void),
			       PIN_PARG(void *), ptr,
			       PIN_PARG_END() );

  Object_map.erase((ADDRINT)ptr);
}

/* ===================================================================== */
// This routine replaces malloc, calloc, salloc, and clear

VOID ImageLoad(IMG img, VOID *v)
{
  //cout << IMG_Name(img) << endl;
  PROTO proto_malloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
				       "malloc", PIN_PARG(int), PIN_PARG_END() );
  PROTO proto_calloc = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
                                       "calloc", PIN_PARG(int), PIN_PARG(int), PIN_PARG_END() );
  PROTO proto_free = PROTO_Allocate( PIN_PARG(void *), CALLINGSTD_DEFAULT,
				     "free", PIN_PARG(void *), PIN_PARG_END() );
    
  RTN rtn = RTN_FindByName(img, "malloc");
  if(RTN_Valid(rtn)){
    //cout << "Replacing malloc in " << IMG_Name(img) << endl;
    
    RTN_ReplaceSignature(
			 rtn, AFUNPTR( Jit_Malloc_IA32 ),
			 IARG_PROTOTYPE, proto_malloc,
			 IARG_CONTEXT,
			 IARG_ORIG_FUNCPTR,
			 IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			 IARG_END);
  }

  RTN rtn1 = RTN_FindByName(img, "calloc");
  if(RTN_Valid(rtn1)){
    cerr << "Replacing calloc in " << IMG_Name(img) << endl;      
    RTN_ReplaceSignature(
                         rtn1, AFUNPTR( Jit_Calloc_IA32 ),
                         IARG_PROTOTYPE, proto_calloc,
                         IARG_CONTEXT,
                         IARG_ORIG_FUNCPTR,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                         IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
                         IARG_END);
  }
  
  RTN rtn2 = RTN_FindByName(img, "free");
  if (RTN_Valid(rtn2)){
    //cout << "Replacing free in " << IMG_Name(img) << endl;
    
    RTN_ReplaceSignature(
			 rtn2, AFUNPTR( Jit_Free_IA32 ),
			 IARG_PROTOTYPE, proto_free,
			 IARG_CONTEXT,
			 IARG_ORIG_FUNCPTR,
			 IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			 IARG_END);
  }

  RTN rtn3 = RTN_FindByName(img, "stackalloc");
  if(RTN_Valid(rtn3)){
    //cout << "Replacing stackalloc in " << IMG_Name(img) << endl;
    
    RTN_Replace(rtn3, AFUNPTR(salloc));
  }

  RTN rtn4 = RTN_FindByName(img, "sfree");
  if(RTN_Valid(rtn4)){
    //cout << "Replacing sfree in " << IMG_Name(img) << endl;
    
    RTN_Replace(rtn4, AFUNPTR(sfree));
  }

  PROTO_Free(proto_malloc);
  PROTO_Free(proto_calloc);
  PROTO_Free(proto_free);
}

VOID CaptureEaAndValue(THREADID threadid, VOID * ea, uint32_t size, bool isRead)
{
  char fname[256];

  if (!(isRead)){
    ea = WriteEa[threadid];
  }

  switch(size){
  case 1://8 bits
    {
#ifndef ARRONLY
      uint8_t x;

      PIN_SafeCopy(&x, static_cast<UINT8*>(ea), 1);

#ifdef MAP
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x) );
      if (ret.second==false){
	Memory_map.erase(static_cast<UINT8*>(ea));
	Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x) );
      }
#endif
#endif
    }break;
  case 2://16 bits
    {
#ifndef ARRONLY
      uint8_t x1;
      uint8_t x2;
      PIN_SafeCopy(&x1, static_cast<UINT8*>(ea), 1);
      PIN_SafeCopy(&x2, static_cast<UINT8*>(ea)+1, 1);

#ifdef MAP
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
      if (ret.second==false){
	Memory_map.erase(static_cast<UINT8*>(ea));
	Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
      if (ret.second==false){
	Memory_map.erase(static_cast<UINT8*>(ea)+1);
	Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
      }
#endif
#endif
    }break;    
  case 4://32 bits
    {
      bool obj_ptr = false;
      VOID * oa;
      VOID * ob;
      uint32_t x5 = 10;

      PIN_SafeCopy(&x5, static_cast<UINT32*>(ea), sizeof(UINT32));

      // loop over all of the objects recorded and see if memory access
      // falls within an object
      for (map<ADDRINT, ADDRINT>::iterator it = Object_map.begin(); it != Object_map.end(); it++){
	// compare the address to the base and bounds
	if((ADDRINT)it->first < (ADDRINT)x5 && (ADDRINT)it->second > (ADDRINT)x5){
	  // memory value falls in bounds of an object
	  //cout << "found an object pointer\n";
	  oa = (VOID *)it->first;
	  ob = (VOID *)it->second;
	  obj_ptr = true;
	  break;
	}
      }

#ifdef ARRONLY
#ifdef TRACE2
      if (accs >= StartCount){
	if(obj_ptr == true){
	  fprintf(fout, "%s    %X    %X%08X\n\r", (isRead)?"read":"write", (UINT32)ea, (UINT32)oa, (UINT32)ob);
	}else{
	  fprintf(fout, "%s    %X    %X%08X\n\r", (isRead)?"read":"write", (UINT32)ea, 0, 0);
	}
	fflush(fout);

	// check for early exit
	++accs;
	if (accs >= (StartCount+StopCount)){
	  fprintf(stdout, "End Instrumentation at %d instructions\n", icount.Count());
	  fflush(stdout);
	  PIN_Detach();
	}
	else if (accs >= fnum * FSIZE){
	  // close and open new log
	  fclose(fout);
	  sprintf(fname, "tar -pczf %s%u.tar.gz %s%u.log", KnobOutputFile.Value().c_str(), fnum-1, KnobOutputFile.Value().c_str(), fnum-1);
	  system(fname);
	  sprintf(fname, "rm %s%u.log", KnobOutputFile.Value().c_str(), fnum-1);
	  system(fname);
	  sprintf(fname, "%s%u.log", KnobOutputFile.Value().c_str(), fnum);
	  fout = fopen(fname, "w");
	  ++fnum;
	}

#endif

#ifdef MAP
	uint8_t x1;
	uint8_t x2;
	uint8_t x3;
	uint8_t x4;
	PIN_SafeCopy(&x1, static_cast<UINT8*>(oa), 1);
	PIN_SafeCopy(&x2, static_cast<UINT8*>(oa)+1, 1);
	PIN_SafeCopy(&x3, static_cast<UINT8*>(oa)+2, 1);
	PIN_SafeCopy(&x4, static_cast<UINT8*>(oa)+3, 1);
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea));
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
	}
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea)+1);
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
	}
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+2, x3) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea)+2);
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+2, x3) );
	}
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+3, x4) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea)+3);
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+3, x4) );
	}
#endif
#endif
      }else{
#ifdef ARRONLY
#ifdef TRACE2
	if (accs >= StartCount){
	  //fprintf(fout, "%s    %08X    %08X%08X\n", (isRead)?"read":"write", (UINT32)ea, 0, 0);
	  //fflush(fout);
	}
#endif
#endif

#ifndef ARRONLY
	PIN_SafeCopy(&x5, static_cast<UIN32*>(ea), sizeof(UINT32));
	
#ifdef TRACE2
	if (accs >= StartCount){
	  //fprintf(fout, "%s    %08X    %08X%08X\n", (isRead)?"read":"write", (UINT32)ea, (UINT32)x5);
	  //fflush(fout);
	}
	//cout  << (UINT32)ea << "     "  << (UINT32)x1 << endl;
	//cout  << (UINT32)ea+1 << "     "  << (UINT32)x2 << endl;
	//cout  << (UINT32)ea+2 << "     "  << (UINT32)x3 << endl;
	//cout  << (UINT32)ea+3 << "     "  << (UINT32)x4 << endl;
#endif
#ifdef MAP
	uint8_t x1;
	uint8_t x2;
	uint8_t x3;
	uint8_t x4;
	PIN_SafeCopy(&x1, static_cast<UINT8*>(ea), 1);
	PIN_SafeCopy(&x2, static_cast<UINT8*>(ea)+1, 1);
	PIN_SafeCopy(&x3, static_cast<UINT8*>(ea)+2, 1);
	PIN_SafeCopy(&x4, static_cast<UINT8*>(ea)+3, 1);
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea));
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
	}
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea)+1);
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
	}
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+2, x3) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea)+2);
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+2, x3) );
	}
	ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+3, x4) );
	if (ret.second==false){
	  Memory_map.erase(static_cast<UINT8*>(ea)+3);
	  Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+3, x4) );
	}
#endif
#endif
      }
    }break;
  case 8://64 bits
    {
#ifndef ARRONLY
      uint8_t x1;
      uint8_t x2;
      uint8_t x3;
      uint8_t x4;
      uint8_t x5;
      uint8_t x6;
      uint8_t x7;
      uint8_t x8;
      PIN_SafeCopy(&x1, static_cast<UINT8*>(ea), 1);
      PIN_SafeCopy(&x2, static_cast<UINT8*>(ea)+1, 1);
      PIN_SafeCopy(&x3, static_cast<UINT8*>(ea)+2, 1);
      PIN_SafeCopy(&x4, static_cast<UINT8*>(ea)+3, 1);
      PIN_SafeCopy(&x5, static_cast<UINT8*>(ea)+4, 1);
      PIN_SafeCopy(&x6, static_cast<UINT8*>(ea)+5, 1);
      PIN_SafeCopy(&x7, static_cast<UINT8*>(ea)+6, 1);
      PIN_SafeCopy(&x8, static_cast<UINT8*>(ea)+7, 1);   

#ifdef MAP
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea));
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea), x1) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+1);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+1, x2) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+2, x3) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+2);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+2, x3) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+3, x4) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+3);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+3, x4) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+4, x5) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+4);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+4, x5) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+5, x6) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+5);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+5, x6) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+6, x7) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+6);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+6, x7) );
      }
      ret =Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+7, x8) );
      if (ret.second==false){
        Memory_map.erase(static_cast<UINT8*>(ea)+7);
        Memory_map.insert ( pair<uint8_t *, uint8_t>(static_cast<UINT8*>(ea)+7, x8) );
      }
#endif
#endif
    }break;
    
  default:
    // huge memory access
    break;
  }
}

VOID Instruction(INS ins)
{
#ifdef ENABLED
  if (INS_IsMemoryRead(ins) && !INS_IsPrefetch(ins))
    { 
      INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(CaptureEaAndValue), IARG_THREAD_ID, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_BOOL, true, IARG_END); //IARG_MEMORYREAD_EA  
    }

  if (INS_HasMemoryRead2(ins))
    {
      INS_InsertPredicatedCall(ins, IPOINT_BEFORE, AFUNPTR(CaptureEaAndValue), IARG_THREAD_ID, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_BOOL, true, IARG_END);
    }

  if (INS_IsMemoryWrite(ins))
    {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CaptureWriteEa), IARG_THREAD_ID, IARG_MEMORYWRITE_EA, IARG_END);

      if (INS_HasFallThrough(ins))
        {
	  INS_InsertPredicatedCall(ins, IPOINT_AFTER, AFUNPTR(CaptureEaAndValue), IARG_THREAD_ID, IARG_MEMORYWRITE_SIZE, IARG_MEMORYWRITE_SIZE, IARG_BOOL, false, IARG_END);
	}
      if (INS_IsBranchOrCall(ins))
	{
	  INS_InsertPredicatedCall(ins, IPOINT_TAKEN_BRANCH, AFUNPTR(CaptureEaAndValue), IARG_THREAD_ID, IARG_MEMORYWRITE_SIZE, IARG_MEMORYWRITE_SIZE, IARG_BOOL, false, IARG_END);          
	}
    }
  
#endif

  if (INS_RegWContain(ins, REG_STACK_PTR)){
    IPOINT where = IPOINT_AFTER;
    if (!INS_HasFallThrough(ins))
      where = IPOINT_TAKEN_BRANCH;

    INS_InsertPredicatedCall(ins, where, AFUNPTR(sfree),
			     IARG_REG_VALUE, REG_STACK_PTR,
			     IARG_END);
  }
}

VOID Trace(TRACE trace, VOID *v)
{
  for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
      for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
	{
	  Instruction(ins);
	}
    }
}

VOID Fini(int code, VOID * v)
{ 
  char fname[256];
  fprintf(fout, "End Instrumentation at %lld instructions\n", icount.Count());
  fflush(fout);
  fclose(fout);
  sprintf(fname, "tar -pczf %s%u.tar.gz %s%u.log", KnobOutputFile.Value().c_str(), fnum-1, KnobOutputFile.Value().c_str(), fnum-1);
  system(fname);
  sprintf(fname, "rm %s%u.log", KnobOutputFile.Value().c_str(), fnum-1);
  system(fname);
}

int main(int argc, char * argv[])
{
  char fname[256];
  PIN_InitSymbols();

  PIN_Init(argc, argv);
  IMG_AddInstrumentFunction(ImageLoad, 0);
  TRACE_AddInstrumentFunction(Trace, 0);
  
  PIN_AddFiniFunction(Fini, 0);
  
  fnum = 0;
  sprintf(fname, "%s%u.log", KnobOutputFile.Value().c_str(), fnum);
  fout = fopen(fname, "w");
  fnum++;

  // activate instruction counter
  StartCount = KnobStartAt.Value() * 1000000;
  StopCount = KnobStopAt.Value() * 1000000;
  icount.Activate();
  accs = 0;

  // Never returns
  PIN_StartProgram();

  return 1;
}


