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

#include <assert.h>
#include <iostream>
#include <fstream>
#include "dytan.h"

 //#include "config_parser.h"
#include "syscall_monitor.H"
#include "replace_functions.h"
#include "syscall_functions.h"

#include "RoutineGraph.H"
#include "BasicBlock.H"

#include "taint_func_args.h"
#include "taint_generator.h"
#include "taint_source_path.h"
#include "taint_source_network.h"
#include "taint_source_func.h"

 //#define SPATIAL 1
 //#define DEBUG 1
#define FSIZE 1<<25

INSTLIB::ICOUNT icount;
UINT32 accs;
UINT32 fnum;

//using namespace XED;

map<REG, bitset2 *> regTaintMap;
map<ADDRINT, bitset2 *> memTaintMap;
map<ADDRINT, bitset2 *> controlTaintMap;

UINT32 StartCount;
UINT32 StopCount;

FILE *log;
FILE *taintAssignmentLog;
FILE *fout;

bitset2 *dest;
bitset2 *src;
bitset2 *eax;
bitset2 *edx;
bitset2 *base;
bitset2 *idx;
bitset2 *eflags;
bitset2 *cnt;

bool tracing;

int currentTaintMark = 0;

InstrumentFunction instrument_functions[XED_ICLASS_LAST];

KNOB<BOOL> KnobTrace( KNOB_MODE_WRITEONCE, "pintool",
		      "tracing", "0", "set the bool tracing variable");

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
			    "o", "dytan.dat", "specify output file name");

KNOB<UINT32> KnobStartAt(KNOB_MODE_WRITEONCE, "pintool",
			 "s", "0", "simpoint");

KNOB<UINT32> KnobStopAt(KNOB_MODE_WRITEONCE, "pintool",
			"q", "2000", "number of million memory accesses to instrument");

inline VOID update_accs(int count){
  char fname[256];
  accs += count;  
  if (accs >= StartCount + StopCount){ // early exit check
    fprintf(stdout, "End Instrumentation at %d instructions\n", icount.Count());
    fflush(stdout);
    PIN_Detach();
  }
  else if (accs >= fnum * FSIZE){    // close and open new log
    fclose(fout);
    sprintf(fname, "tar -pczf %s%u.tar.gz %s%u.log", KnobOutputFile.Value().c_str(), fnum-1, KnobOutputFile.Value().c_str(), fnum-1);
    system(fname);
    sprintf(fname, "rm %s%u.log", KnobOutputFile.Value().c_str(), fnum-1);
    system(fname);
    sprintf(fname, "%s%u.log", KnobOutputFile.Value().c_str(), fnum);
    fout = fopen(fname, "w");
    ++fnum;
  }
}

VOID docount() { 
  if (accs >= StartCount + StopCount){
    printf("End Instrumentation at %llu instructions\n", icount.Count());
    fflush(stdout);
    PIN_Detach();
  }
}

VOID Fini(INT32 code, VOID *v)
{

#ifdef SPATIAL
  ADDRINT taintsize = 0;
  for(map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.begin();
      iter != memTaintMap.end(); iter++) {
    if(!bitset_is_empty(iter->second)){
      taintsize++;
      //fprintf(fout, "%u %s\n", iter->first, bitset_tostring(iter->second).c_str());
      fprintf(fout, "%u %X%08X\n", iter->first, (iter->second)->base, (iter->second)->bound);
    }
  }

  //fprintf(fout, "\n%u bytes\n", addrmax - addrmin);
  fprintf(fout, "%u taint marks\n", currentTaintMark);
  //fprintf(fout, "Taint map size: %d\n", taintsize);
#endif

  //DumpMap();

  printf("Program Complete at %ld Instructions\n", icount.Count());
}
	
/*
  Dumps the instruction to the log file
*/
void Print(ADDRINT address, string *disas)
{
  if(tracing) {
    PIN_LockClient();
    RTN rtn = RTN_FindByAddress(address);
    PIN_UnlockClient();
    
    fprintf(log, "%#x: %s [%s]\n", address, disas->c_str(),
	    RTN_Valid(rtn) ? RTN_Name(rtn).c_str(): "");
    fflush(log);
  }
}

void ClearTaintSet(bitset2 *set)
{
#ifdef DEBUG
  printf("**dytan** TaintForRegister Entry\n");
#endif

  bitset_reset(set);

#ifdef DEBUG
  printf("**dytan** TaintForRegister Exit\n");
#endif
}

/* copies the taint marks for the register into the out bitset2 parameter */
void TaintForRegister(REG reg, bitset2 *set)
{
#ifdef DEBUG
  printf("**dytan** TaintForRegister Entry\n");
#endif

  map<REG, bitset2 *>::iterator iter = regTaintMap.find(reg);
  if(regTaintMap.end() != iter) {
    bitset_set_bits(set, iter->second);
  }
  else {
    // this is important becuase we use global storage it's possible that
    // set will already have values
    bitset_reset(set);
  }

#ifdef TRACE
  if(tracing) {
    const char *sep = "";
    if(REG_valid(reg)) {
      fprintf(log, "\t-%s[", REG_StringShort(reg).c_str());
      //fprintf(log, "%s]\n", bitset_str(set));
      fprintf(log, "%X%08X]\n", set->base, set->bound);
    }
  }
#endif
	
#ifdef DEBUG
  printf("**dytan** TaintForRegister Exit\n");
#endif
}

void TaintForRepMov(ADDRINT start, ADDRINT size,
		    ADDRINT dstart, ADDRINT dsize)
{
#ifdef DEBUG
  printf("**dytan** TaintForRepMov Entry\n");
#endif

  //printf("beginning TaintForRepMov rstart: %x rsize: %d\n", start, size);
  assert(size == dsize);

  // rep mov moves x bytes from source to destination
  // so iterate through all read bytes and copy taint marks of each byte
  // to the destination bytes
  
  for(ADDRINT i = 0; i < size; i++) {
    ADDRINT addr = start + i;
    ADDRINT dest_addr = dstart + i;

    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
    
    if(memTaintMap.end() == iter){
      if (accs >= StartCount){
	fprintf(fout, "read\t%08X\t0\n", addr);
	fprintf(fout, "write\t%08X\t0\n", dest_addr);
	update_accs(2);
      }
    }else{
      //    if(memTaintMap.end() != iter) {
      /*if(!bitset_is_empty(iter->second)){
	printf("Union of taint for address 0x%x ", addr);
	for(uint i=0; i<(iter->second)->nbits; i++){
	cout << bitset_test_bit(iter->second, i);
	}
	cout << " with SRC\n";
	}*/      
      memTaintMap[dest_addr] = bitset_copy(iter->second);

      if (accs >= StartCount){
	//fprintf(fout, "read\t%08X\t%s\n", addr, (bitset_tostring(iter->second)).c_str());
	//fprintf(fout, "write\t%08X\t%s\n", dest_addr, (bitset_tostring(iter->second)).c_str());
	
	fprintf(fout, "read\t%X\t%X%08X\n", addr, (iter->second)->base, (iter->second)->bound);
	fprintf(fout, "write\t%X\t%X%08X\n", dest_addr, (iter->second)->base, (iter->second)->bound);

	update_accs(2);
      }

#ifdef TRACE
      if (tracing) {
	const char *sep = "";
        fprintf(log, "\t-%#x[] <- %#x[", dest_addr, addr);
	//fprintf(log, "%s]\n", bitset_str(iter->second));
	fprintf(log, "%X%08X]\n", (iter->second)->base, (iter->second)->bound);
      }
#endif	  
    }
  }

#ifdef DEBUG
  printf("**dytan** TaintForRepMov Exit\n");
#endif
}

/* Return in the out parameter, set, the union of the taint marks
   from memory address start to start + size - 1, and if IMPLICIT is
   defined the taint marks for the base and index registers used to
   access memory
*/
void TaintForMemory(ADDRINT start, ADDRINT size,
		    REG baseReg, REG indexReg,
		    bitset2 *set)
{
#ifdef DEBUG
  printf("**dytan** TaintForMemory Entry\n");
#endif

  // need to clear out set incase there are preexisting values
  bitset_reset(set);

  for(ADDRINT addr = start; addr < start + size; addr++) {
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
    if(memTaintMap.end() != iter) {
      bitset_union(set, iter->second);  
      if (accs >= StartCount){
	//fprintf(fout, "read\t%08X\t%s\n", addr, (bitset_tostring(iter->second)).c_str());
	fprintf(fout, "read\t%X\t%X%08X\n", addr, (iter->second)->base, (iter->second)->bound);
	update_accs(1);
      }
    }else{
      if (accs >= StartCount){
	fprintf(fout, "read\t%08X\t0\n", addr);
	update_accs(1);
      }
    }
  }

#ifdef TRACE
  const char *sep = "";
  if(tracing) {
    fprintf(log, "\t-%#x-%#x[", start, start + size - 1);
    //fprintf(log, "%s]\n", bitset_str(set));
    fprintf(log, "%X%08X]\n", set->base, set->bound);
  }
#endif

#ifdef IMPLICIT
  if(REG_valid(baseReg)) {
    map<REG, bitset2 *>::iterator iter = regTaintMap.find(baseReg);
    if(regTaintMap.end() != iter) {
      bitset_union(set, iter->second);
#ifdef TRACE
      if(tracing) {
	sep = "";
	fprintf(log, ", %s[", REG_StringShort(baseReg).c_str());
	//fprintf(log, "%s]\n", bitset_str(set));
	fprintf(log, "%X%08X]\n", set->base, set->bound);
      }
#endif     
      
    }
  }
  
  if(REG_valid(indexReg)) {
    map<REG, bitset2 *>::iterator iter = regTaintMap.find(indexReg);
    if(regTaintMap.end() != iter) {
      bitset_union(set, iter->second);
		
#ifdef TRACE
      if(tracing) {
	sep = "";
	fprintf(log, ", %s[", REG_StringShort(indexReg).c_str());
	//fprintf(log, "%s]\n", bitset_str(iter->second));
	fprintf(log, "%X%08X]\n", (iter->second)->base, (iter->second)->bound);
      }
#endif

    }
  }
#endif

#ifdef TRACE
  if(tracing) {
    fprintf(log, "\n");
  }
#endif

#ifdef DEBUG  
  printf("**dytan** TaintForMemory Exit\n");
#endif
}

/* 
   sets the taint marks associated with the dest register to the union
   of the bitset2s passed in the varargs parameter
 */
void SetTaintForRegister(ADDRINT amount, REG dest, ...)
{
#ifdef DEBUG
  printf("**dytan** SetTaintForRegister Entry\n");
#endif

  va_list ap;
  bitset2 *src;

  if(LEVEL_BASE::REG_ESP == dest ||
     LEVEL_BASE::REG_EBP == dest) return;

  bitset2 *tmp = bitset_init(NUMBER_OF_TAINT_MARKS);

  va_start(ap, dest);
  
  for (ADDRINT i=1;i<amount;i++) {
    assert((src = va_arg(ap, bitset2 *)) != NULL);     
    
    bitset_union(tmp, src); //va_arg(ap, bitset22 *));
  }
  
  va_end(ap);

  // control flow
  /*bitset2 *controlTaint = bitset_init(NUMBER_OF_TAINT_MARKS);
  for(map<ADDRINT, bitset2 *>::iterator iter = controlTaintMap.begin();
      iter != controlTaintMap.end(); iter++) {
    bitset_union(controlTaint, iter->second);
  }
  bitset_union(tmp, controlTaint);*/

  /* This is where we account for subregisters */
  /*
    This isn't totally complete yet.  For example edi and esi are not
     included and setting [A|B|C|D]X won't set the super or subregisters
  */

  //eax
  if(LEVEL_BASE::REG_EAX == dest) {
    //ax
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_AX)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_AX], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_AX] = bitset_copy(tmp);
    }
    
    //al
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_AH)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_AH], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_AH] = bitset_copy(tmp);
    }
    
    //ah
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_AL)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_AL], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_AL] = bitset_copy(tmp);
    }
  }
  
  //ebx
  else if(LEVEL_BASE::REG_EBX == dest) {
    //bx
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_BX)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_BX], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_BX] = bitset_copy(tmp);
    }
    
    //bl
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_BH)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_BH], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_BH] = bitset_copy(tmp);
    }
    
    //bh
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_BL)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_BL], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_BL] = bitset_copy(tmp);
    }
  }

  //ecx
  else if(LEVEL_BASE::REG_ECX == dest) {
    //cx
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_CX)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_CX], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_CX] = bitset_copy(tmp);
    }
    
    //cl
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_CH)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_CH], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_CH] = bitset_copy(tmp);
    }
    
    //ch
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_CL)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_CL], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_CL] = bitset_copy(tmp);
    }
  }

  //edx
  else if(LEVEL_BASE::REG_EDX == dest) {
    //dx
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_DX)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_DX], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_DX] = bitset_copy(tmp);
    }
    
    //dl
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_DH)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_DH], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_DH] = bitset_copy(tmp);
    }
    
    //dh
    if(regTaintMap.end() != regTaintMap.find(LEVEL_BASE::REG_DL)) {
      bitset_set_bits(regTaintMap[LEVEL_BASE::REG_DL], tmp);
    }
    else {
      regTaintMap[LEVEL_BASE::REG_DL] = bitset_copy(tmp);
    }
  }


  if(regTaintMap.end() != regTaintMap.find(dest)) {
    bitset_set_bits(regTaintMap[dest], tmp);
  }
  else {
    regTaintMap[dest] = bitset_copy(tmp);
  }

#ifdef TRACE
  if (tracing) {
    fprintf(log, "\t%s[", REG_StringShort(dest).c_str());
    //fprintf(log, "%s]\n", bitset_str(tmp));
    fprintf(log, "%X%08X]\n", tmp->base, tmp->bound);
  }  
#endif

  bitset_free(tmp);
  //bitset_free(controlTaint);

#ifdef DEBUG	
  printf("**dytan** SetTaintForRegister Exit\n");
#endif
}

/* Clears the taint marks associated with a register */
void ClearTaintForRegister(REG reg)
{
#ifdef DEBUG
  printf("**dytan** ClearTaintForRegister Entry\n");
#endif

  /*// control flow
  bitset2 *controlTaint = bitset_init(NUMBER_OF_TAINT_MARKS);
  for(map<ADDRINT, bitset2 *>::iterator iter = controlTaintMap.begin();
      iter != controlTaintMap.end(); iter++) {
    bitset_union(controlTaint, iter->second);
  }

  map<REG, bitset2 *>::iterator iter = regTaintMap.find(reg);
  if(regTaintMap.end() != iter) {
    bitset_set_bits(iter->second, controlTaint);
    }*/


#ifdef TRACE
  if(tracing) {
    const char *sep = "";
    fprintf(log, "\t%s <- cf[", REG_StringShort(reg).c_str());

    sep = "";
    /*for(int i = 0; i < (int)controlTaint->nbits; i++) {
      if(bitset_test_bit(controlTaint, i)) {
	fprintf(log, "%s%d", sep, i);
	sep = ", ";
      }
      }*/
    fprintf(log, "]\n");
  }
#endif

  //bitset_free(controlTaint);

#ifdef DEBUG
  printf("**dytan** ClearTaintForRegister Entry\n");
#endif
}

/* Clears taint marks associted with the range of memory */
void ClearTaintForMemory(ADDRINT start, ADDRINT size)
{
#ifdef DEBUG
  printf("**dytan** ClearTaintForMemory Entry\n");
#endif
  
  // control flow
  /*bitset2 *controlTaint = bitset_init(NUMBER_OF_TAINT_MARKS);
    
    for(map<ADDRINT, bitset2 *>::iterator iter = controlTaintMap.begin();
    iter != controlTaintMap.end(); iter++) {
    bitset_union(controlTaint, iter->second);
    }
    
    for(ADDRINT addr = start; addr < start + size; addr++) {
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
    if(memTaintMap.end() != iter) {
    bitset_set_bits(iter->second, controlTaint);
    
    }
    }*/
  for(ADDRINT addr = start; addr < start+size; addr++){    
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
    if(memTaintMap.end() != iter) {
      //bitset_set_bits(iter->second, controlTaint);
      if (accs >= StartCount){  
	fprintf(fout, "write\t%08X\t0\n", addr);
	update_accs(1);
      }
      // clear the taint
      memTaintMap.erase(iter);
    }
  }

#ifdef TRACE
  if(tracing) {
    const char *sep = "";
    fprintf(log, "\t%#x-%#x <- cf[", start, start + size - 1);

    /*sep = "";
    for(int i = 0; i < (int)controlTaint->nbits; i++) {
      if(bitset_test_bit(controlTaint, i)) {
	fprintf(log, "%s%d", sep, i);
	sep = ", ";
      }
      }*/
    fprintf(log, "]\n");
  }
#endif
  
  //bitset_free(controlTaint);

#ifdef DEBUG	
  printf("**dytan** ClearTaintForMemory Exit\n");
#endif
}

/* Set the taint marks associated with the memory range to the union
   of the bitset2s passed in the varargs parameter
*/
void SetTaintForMemory(ADDRINT amount, ADDRINT start, ADDRINT size, ...)
{
#ifdef DEBUG
  printf("**dytan** SetTaintForMemory Entry\n");
#endif

  va_list ap;
  bitset2 *src;

  bitset2 *tmp = bitset_init(NUMBER_OF_TAINT_MARKS);
  
  va_start(ap, size);

  for(ADDRINT i=2;i<amount;i++){
    assert((src = va_arg(ap, bitset2 *)) != NULL);
    bitset_union(tmp, src);
  }
  
  va_end(ap);

  // control flow
  /*bitset2 *controlTaint = bitset_init(NUMBER_OF_TAINT_MARKS);

  for(map<ADDRINT, bitset2 *>::iterator iter = controlTaintMap.begin();
      iter != controlTaintMap.end(); iter++) {
    bitset_union(controlTaint, iter->second);
  }
  bitset_union(tmp, controlTaint);*/


  for(ADDRINT addr = start; addr < start + size; addr++) {
    if (accs >= StartCount){
      //fprintf(fout, "write\t%08X\t%s\n", addr, (bitset_tostring(tmp)).c_str());
      fprintf(fout, "write\t%X\t%X%08X\n", addr, tmp->base, tmp->bound);

      update_accs(1);
    }
    if(memTaintMap.end() != memTaintMap.find(addr)) {
      bitset_set_bits(memTaintMap[addr], tmp);
    }
    else {
      memTaintMap[addr] = bitset_copy(tmp);
    }
  }
  
#ifdef TRACE
  if(tracing) {
    const char *sep = "";
    fprintf(log, "\t%#x-%#x[", start, start + size - 1);
    //fprintf(log, "%s] <- cf[", bitset_str(tmp));
    fprintf(log, "%X%08X] <- cf[", tmp->base, tmp->bound);

    sep = "";
    /*for(int i = 0; i < (int)controlTaint->nbits; i++) {
      if(bitset_test_bit(controlTaint, i)) {
	fprintf(log, "%s%d", sep, i);
	sep = ", ";
      }
      }*/
    fprintf(log, "]\n");
  }
#endif
  
  bitset_free(tmp);
  //bitset_free(controlTaint);

#ifdef DEBUG	
  printf("**dytan** SetTaintForMemory Exit\n");
#endif
}

#include "instrument_functions.c"

void PushControl(ADDRINT addr)
{
#ifdef DEBUG
  printf("**dytan** PushControl Entry\n");
#endif

#ifdef TRACE
  if(tracing) {
    fprintf(log, "\tpush control: %#x\n", addr);
    fflush(log);
  }
#endif

  if(regTaintMap.end() == regTaintMap.find(LEVEL_BASE::REG_EFLAGS) ||
     bitset_is_empty(regTaintMap[LEVEL_BASE::REG_EFLAGS])) return;

  /*if(controlTaintMap.end() == controlTaintMap.find(addr)) {
    controlTaintMap[addr] = bitset_init(NUMBER_OF_TAINT_MARKS);
    }*/

  //bitset_union(controlTaintMap[addr], regTaintMap[LEVEL_BASE::REG_EFLAGS]);

  //dump control taint map
#ifdef TRACE
  if(tracing) {
    /*for(map<ADDRINT, bitset2 *>::iterator iter = controlTaintMap.begin();
	iter != controlTaintMap.end(); iter++) {
      
      const char *sep = "";
      fprintf(log, "\t\t-%#x - [", iter->first);
      for(int i = 0; i < (int)iter->second->nbits; i++) {
	if(bitset_test_bit(iter->second, i)) {
	  fprintf(log, "%s%d", sep, i);
	  sep = ", ";
	}
      }
      fprintf(log, "]\n");
      }*/
  }
#endif

#ifdef DEBUG	
  printf("**dytan** PushControl Exit\n");
#endif
}

void PopControl(int n, ...)
{
#ifdef DEBUG
  printf("**dytan** PopControl Entry\n");
#endif

#ifdef TRACE
  if(tracing) {
    fprintf(log, "\tpop control: ");
  }
#endif

  va_list ap;
  ADDRINT addr;
  const char *sep = "";

  va_start(ap, n);
  
  for (; n; n--) {
    
    addr = va_arg(ap, ADDRINT);

#ifdef TRACE
    if(tracing) {
      fprintf(log, "%s%#x\n", sep, addr);
      fflush(log);
      sep = ", ";
    }
#endif
  }
  va_end(ap);

#ifdef TRACE
  if(tracing) {
    fprintf(log, "\n");
    fflush(log);
  }
#endif

#ifdef DEBUG	
  printf("**dytan** PopControl Exit\n");
#endif
}


static void Controlflow(RTN rtn, void *v)
{
  string rtn_name = RTN_Name(rtn);

  IMG img = SEC_Img(RTN_Sec(rtn));

  if(LEVEL_CORE::IMG_TYPE_SHAREDLIB == IMG_Type(img)) return;

  RTN_Open(rtn);

  RoutineGraph *rtnGraph = new RoutineGraph(rtn);

  map<ADDRINT, set<ADDRINT> > controls;

  for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {

    if(XED_CATEGORY_COND_BR == INS_Category(ins)) {

      ADDRINT addr = INS_Address(ins);
      BasicBlock *block = rtnGraph->addressMap[addr];
      if(NULL == block) {
	printf("block is null\n");
	fflush(stdout);
      }

      BasicBlock *ipdomBlock = block->getPostDominator();
      if(NULL == ipdomBlock) {
	printf("ipdomBlock is null in %s\n", rtn_name.c_str());
	fflush(stdout);
      }
      ADDRINT ipdomAddr = ipdomBlock->startingAddress;
   
      if(controls.find(ipdomAddr) == controls.end()) {
	controls[ipdomAddr] = set<ADDRINT>();
      }

      controls[ipdomAddr].insert(addr);

      //      printf("placing push call: %#x - %s\n", addr,
      //     INS_Disassemble(ins).c_str());
      
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(PushControl),
		     IARG_PTR, addr,
		     IARG_END);
    }
  }
  
  for(INS ins = RTN_InsHead(rtn); INS_Valid(ins); ins = INS_Next(ins)) {

    ADDRINT addr = INS_Address(ins);
   
    if(controls.end() == controls.find(addr)) continue;

    IARGLIST args = IARGLIST_Alloc();

    for(set<ADDRINT>::iterator iter = controls[addr].begin();
	iter != controls[addr].end(); iter++) {
      IARGLIST_AddArguments(args, IARG_ADDRINT, *iter, IARG_END);
      //      printf("\t%#x\n", *iter);
    }
      

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(PopControl),
		   IARG_UINT32, controls[addr].size(),
		   IARG_IARGLIST, args,
		   IARG_END);
    IARGLIST_Free(args);
  }
  
  delete rtnGraph;

  RTN_Close(rtn);
}

static void Dataflow(INS ins, void *v)
{
  xed_iclass_enum_t opcode = (xed_iclass_enum_t) INS_Opcode(ins);

  // add code here to count the number of instructions that were executed	
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(docount), IARG_END);

  (*instrument_functions[opcode])(ins, v);
}

static void Trace(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(Print),
		 IARG_ADDRINT, INS_Address(ins),
		 IARG_PTR, new string(INS_Disassemble(ins)),
		 IARG_END);
}

void SetNewTaintForMemory(ADDRINT addr, ADDRINT size, int taint_mark)
{
#ifdef DEBUG
  printf("**dytan** SetNewTaintForMemory Entry\n");
#endif

  if (taint_mark == -1) {
      taint_mark = currentTaintMark;
      currentTaintMark++;
  }
  bitset2 *tmp = bitset_init(NUMBER_OF_TAINT_MARKS);
  bitset_set_bit(tmp, taint_mark);
 
  for(ADDRINT address = (ADDRINT) addr; address < (ADDRINT) addr + size; address++) {
    if (accs >= StartCount){
      //fprintf(fout, "write\t%08X\t%s\n", addr, (bitset_tostring(tmp)).c_str());
      fprintf(fout, "write\t%X\t%X%08X\n", addr, tmp->base, tmp->bound);
      update_accs(1);
    }
    memTaintMap[address] = bitset_copy(tmp);
  }
  
#ifdef TRACE
  const char *sep = "";
  fprintf(log, "@%#x-%#x[", addr, (ADDRINT) addr + size - 1);
  //fprintf(log, "%s]\n", bitset_str(tmp));
  fprintf(log, "%X%08X]\n", tmp->base, tmp->bound);
#endif
  
  fprintf(taintAssignmentLog, "%d ->%#x-%#x\n",
	  taint_mark, 
	  addr,
	  addr + size - 1);
  fflush(taintAssignmentLog);
  
  
  bitset_free(tmp);
	
#ifdef DEBUG	
  printf("**dytan** SetNewTaintForMemory Exit\n");
#endif
}

int main(int argc, char **argv)
{
  char fname[256];
	
    /**
     * read configuration file and populate conf with
     * the configuration details
     */
    //int result = parseConfig(argc, argv, conf);

    PIN_InitSymbols();
    PIN_Init(argc, argv);

    PIN_AddFiniFunction(Fini, 0);

    //set_framework_options(conf);
    /*
     * Set up taint propagation
     */

    dest = bitset_init(NUMBER_OF_TAINT_MARKS);
    src = bitset_init(NUMBER_OF_TAINT_MARKS);
    eax = bitset_init(NUMBER_OF_TAINT_MARKS);
    edx = bitset_init(NUMBER_OF_TAINT_MARKS);
    base = bitset_init(NUMBER_OF_TAINT_MARKS);
    idx = bitset_init(NUMBER_OF_TAINT_MARKS);
    eflags = bitset_init(NUMBER_OF_TAINT_MARKS);
    cnt = bitset_init(NUMBER_OF_TAINT_MARKS);

    /*
     * Set up taint sinks
     */
/*    vector<sink>::iterator itr2 = conf->sinks.begin();
    // Iterate over taint sinks
    while (itr2 != conf->sinks.end()) {
        sink snk = *itr2;
        itr2++;
        //TODO
    } */

    IMG_AddInstrumentFunction(ReplaceUserFunctions, 0);

    RTN_AddInstrumentFunction(taint_routines, 0);

#ifdef TRACE
    INS_AddInstrumentFunction(Trace, 0);
#endif

    //if (conf->prop.dataflow)
    INS_AddInstrumentFunction(Dataflow, 0);

    // set a default handling function that aborts.  This makes sure I don't
    // miss instructions in new applications
    for(int i = 0; i < XED_ICLASS_LAST; i++) {
        instrument_functions[i] = &UnimplementedInstruction;
    }

    instrument_functions[XED_ICLASS_ADD] = &Instrument_ADD; // 1
    instrument_functions[XED_ICLASS_PUSH] = &Instrument_PUSH; // 2
    instrument_functions[XED_ICLASS_POP] = &Instrument_POP; // 3
    instrument_functions[XED_ICLASS_OR] = &Instrument_OR; // 4

    instrument_functions[XED_ICLASS_ADC] = &Instrument_ADC; // 6
    instrument_functions[XED_ICLASS_SBB] = &Instrument_SBB; // 7
    instrument_functions[XED_ICLASS_AND] = &Instrument_AND; // 8

    //  instrument_functions[XED_ICLASS_DAA] = &Instrument_DAA; // 11
    instrument_functions[XED_ICLASS_SUB] = &Instrument_SUB; // 12

    //  instrument_functions[XED_ICLASS_DAS] = &Instrument_DAS; // 14
    instrument_functions[XED_ICLASS_XOR] = &Instrument_XOR; // 15

    //  instrument_functions[XED_ICLASS_AAA] = &Instrument_AAA; // 17
    instrument_functions[XED_ICLASS_CMP] = &Instrument_CMP; // 18

    //  instrument_functions[XED_ICLASS_AAS] = &Instrument_AAS; // 20
    instrument_functions[XED_ICLASS_INC] = &Instrument_INC; // 21
    instrument_functions[XED_ICLASS_DEC] = &Instrument_DEC; // 22

    //  instrument_functions[XED_ICLASS_PUSHAD] = &Instrument_PUSHAD; // 25
    //  instrument_functions[XED_ICLASS_POPAD] = &Instrument_POPAD; // 27
    //  instrument_functions[XED_ICLASS_BOUND] = &Instrument_BOUND; // 28
    //  instrument_functions[XED_ICLASS_ARPL] = &Instrument_ARPL; // 29

    instrument_functions[XED_ICLASS_IMUL] = &Instrument_IMUL; // 35
    //  instrument_functions[XED_ICLASS_INSB] = &Instrument_INSB; // 36

    //  instrument_functions[XED_ICLASS_INSD] = &Instrument_INSD; // 38
    //  instrument_functions[XED_ICLASS_OUTSB] = &Instrument_OUTSB; // 39

    //  instrument_functions[XED_ICLASS_OUTSD] = &Instrument_OUTSD; // 41
    instrument_functions[XED_ICLASS_JO] = &Instrument_Jcc; //42
    instrument_functions[XED_ICLASS_JNO] = &Instrument_Jcc; //43
    instrument_functions[XED_ICLASS_JB] = &Instrument_Jcc; //43
    instrument_functions[XED_ICLASS_JNB] = &Instrument_Jcc; //45
    instrument_functions[XED_ICLASS_JZ] = &Instrument_Jcc; //46
    instrument_functions[XED_ICLASS_JNZ] = &Instrument_Jcc; //47
    instrument_functions[XED_ICLASS_JBE] = &Instrument_Jcc; //48
    instrument_functions[XED_ICLASS_JNBE] = &Instrument_Jcc; //49
    instrument_functions[XED_ICLASS_JS] = &Instrument_Jcc; //50
    instrument_functions[XED_ICLASS_JNS] = &Instrument_Jcc; //51
    instrument_functions[XED_ICLASS_JP] = &Instrument_Jcc; //52
    instrument_functions[XED_ICLASS_JNP] = &Instrument_Jcc; //53
    instrument_functions[XED_ICLASS_JL] = &Instrument_Jcc; //54
    instrument_functions[XED_ICLASS_JNL] = &Instrument_Jcc; //55
    instrument_functions[XED_ICLASS_JLE] = &Instrument_Jcc; //56
    instrument_functions[XED_ICLASS_JNLE] = &Instrument_Jcc; //57

    instrument_functions[XED_ICLASS_TEST] = &Instrument_TEST; //59
    instrument_functions[XED_ICLASS_XCHG] = &Instrument_XCHG; //60
    instrument_functions[XED_ICLASS_MOV] = &Instrument_MOV; //61
    instrument_functions[XED_ICLASS_LEA] = &Instrument_LEA; //62

    instrument_functions[XED_ICLASS_PAUSE] = &Instrument_PAUSE; //64

    instrument_functions[XED_ICLASS_CWDE] = &Instrument_CWDE; //67

    instrument_functions[XED_ICLASS_CDQ] = &Instrument_CDQ; //70
    //  instrument_functions[XED_ICLASS_CALL_FAR] = &Instrument_CALL_FAR; //71
    //  instrument_functions[XED_ICLASS_WAIT] = &Instrument_WAIT; //72

    instrument_functions[XED_ICLASS_PUSHFD] = &Instrument_PUSHFD; //74

    instrument_functions[XED_ICLASS_POPFD] = &Instrument_POPFD; //77

    //  instrument_functions[XED_ICLASS_POPFD] = &Instrument_SAHF; //79
    //  instrument_functions[XED_ICLASS_POPFD] = &Instrument_LAHF; //80
    instrument_functions[XED_ICLASS_MOVSB] = &Instrument_MOVSB; //81
    instrument_functions[XED_ICLASS_MOVSW] = &Instrument_MOVSW; //82
    instrument_functions[XED_ICLASS_MOVSD] = &Instrument_MOVSD; //83

    instrument_functions[XED_ICLASS_CMPSB] = &Instrument_CMPSB; //85

    //  instrument_functions[XED_ICLASS_CMPSD] = &Instrument_CMPSD; //87

    instrument_functions[XED_ICLASS_STOSB] = &Instrument_STOSB; //89
    //  instrument_functions[XED_ICLASS_STOSW] = &Instrument_STOSW; //90
    instrument_functions[XED_ICLASS_STOSD] = &Instrument_STOSD; //91

    //  instrument_functions[XED_ICLASS_LODSB] = &Instrument_LODSB; //93

    //  instrument_functions[XED_ICLASS_LODSD] = &Instrument_LODSD; //95

    instrument_functions[XED_ICLASS_SCASB] = &Instrument_SCASB; //97

    //  instrument_functions[XED_ICLASS_SCASD] = &Instrument_SCASD; //99

    instrument_functions[XED_ICLASS_RET_NEAR] = &Instrument_RET_NEAR; //102
    //  instrument_functions[XED_ICLASS_LES] = &Instrument_LES; //103
    //  instrument_functions[XED_ICLASS_LDS] = &Instrument_LDS; //104

    //  instrument_functions[XED_ICLASS_ENTER] = &Instrument_ENTER; //106
    instrument_functions[XED_ICLASS_LEAVE] = &Instrument_LEAVE; //107
    //  instrument_functions[XED_ICLASS_RET_FAR] = &Instrument_RET_FAR; //108
    //  instrument_functions[XED_ICLASS_INT3] = &Instrument_INT3; //109
    instrument_functions[XED_ICLASS_INT] = &Instrument_INT; //110
    //  instrument_functions[XED_ICLASS_INT0] = &Instrument_INT0; //111

    //  instrument_functions[XED_ICLASS_IRETD] = &Instrument_IRETD; //113

    //  instrument_functions[XED_ICLASS_AAM] = &Instrument_AAM; //115
    //  instrument_functions[XED_ICLASS_AAD] = &Instrument_AAD; //116
    //  instrument_functions[XED_ICLASS_SALC] = &Instrument_SALC; //117
    //  instrument_functions[XED_ICLASS_XLAT] = &Instrument_XLAT; //118

    //  instrument_functions[XED_ICLASS_LOOPNE] = &Instrument_LOOPNE; //120
    //  instrument_functions[XED_ICLASS_LOOPE] = &Instrument_LOOPE; //121
    //  instrument_functions[XED_ICLASS_LOOP] = &Instrument_LOOP; //122
    instrument_functions[XED_ICLASS_JRCXZ] = &Instrument_Jcc; //123
    //  instrument_functions[XED_ICLASS_IN] = &Instrument_IN; //124
    //  instrument_functions[XED_ICLASS_OUT] = &Instrument_OUT; //125
    instrument_functions[XED_ICLASS_CALL_NEAR] = &Instrument_CALL_NEAR; //126
    instrument_functions[XED_ICLASS_JMP] = &Instrument_JMP; //127
    //  instrument_functions[XED_ICLASS_JMP_FAR] = &Instrument_JMP_FAR; //128

    //  instrument_functions[XED_ICLASS_INT_l] = &Instrument_INT_l; //130

    instrument_functions[XED_ICLASS_HLT] = &Instrument_HLT; //133
    //  instrument_functions[XED_ICLASS_CMC] = &Instrument_CMC; //134

    //  instrument_functions[XED_ICLASS_CLC] = &Instrument_CLC; //136
    //  instrument_functions[XED_ICLASS_STC] = &Instrument_STC; //137
    //  instrument_functions[XED_ICLASS_CLI] = &Instrument_CLI; //138
    //  instrument_functions[XED_ICLASS_STI] = &Instrument_STI; //139
    instrument_functions[XED_ICLASS_CLD] = &Instrument_CLD; //140
    instrument_functions[XED_ICLASS_STD] = &Instrument_STD; //141

    instrument_functions[XED_ICLASS_RDTSC] = &Instrument_RDTSC; //169

    instrument_functions[XED_ICLASS_CMOVB] = &Instrument_CMOVcc; //177
    instrument_functions[XED_ICLASS_CMOVNB] = &Instrument_CMOVcc; //178
    instrument_functions[XED_ICLASS_CMOVZ] = &Instrument_CMOVcc; //179
    instrument_functions[XED_ICLASS_CMOVNZ] = &Instrument_CMOVcc; //180
    instrument_functions[XED_ICLASS_CMOVBE] = &Instrument_CMOVcc; //181
    instrument_functions[XED_ICLASS_CMOVNBE] = &Instrument_CMOVcc; //182

    //  instrument_functions[XED_ICLASS_EMMS] = &Instrument_EMMS; //216

    instrument_functions[XED_ICLASS_SETB] = &Instrument_SETcc; //222
    instrument_functions[XED_ICLASS_SETNB] = &Instrument_SETcc; //223
    instrument_functions[XED_ICLASS_SETZ] = &Instrument_SETcc; //224
    instrument_functions[XED_ICLASS_SETNZ] = &Instrument_SETcc; //225
    instrument_functions[XED_ICLASS_SETBE] = &Instrument_SETcc; //226
    instrument_functions[XED_ICLASS_SETNBE] = &Instrument_SETcc; //227
    //  instrument_functions[XED_ICLASS_CPUID] = &Instrument_CPUID; //228
    //  instrument_functions[XED_ICLASS_BT] = &Instrument_BT; //229
    instrument_functions[XED_ICLASS_SHLD] = &Instrument_SHLD; //230
    instrument_functions[XED_ICLASS_CMPXCHG] = &Instrument_CMPXCHG; //231

    //  instrument_functions[XED_ICLASS_BTR] = &Instrument_BTR; //233

    instrument_functions[XED_ICLASS_MOVZX] = &Instrument_MOVZX; //236
    instrument_functions[XED_ICLASS_XADD] = &Instrument_XADD; //237

    //  instrument_functions[XED_ICLASS_PSRLQ] = &Instrument_PSRLQ; //250  
    //  instrument_functions[XED_ICLASS_PADDQ] = &Instrument_PADDQ; //251  

    //  instrument_functions[XED_ICLASS_MOVQ] = &Instrument_MOVQ; //255  

    //  instrument_functions[XED_ICLASS_MOVQ2Q] = &Instrument_MOVDQ2Q; //258

    //  instrument_functions[XED_ICLASS_PSLLQ] = &Instrument_PSLLQ; //272
    //  instrument_functions[XED_ICLASS_PMULUDQ] = &Instrument_PMULUDQ; //273

    //  instrument_functions[XED_ICLASS_UD2] = &Instrument_UD2; //281

    instrument_functions[XED_ICLASS_CMOVS] = &Instrument_CMOVcc; //307
    instrument_functions[XED_ICLASS_CMOVNS] = &Instrument_CMOVcc; //308

    instrument_functions[XED_ICLASS_CMOVL] = &Instrument_CMOVcc; //311
    instrument_functions[XED_ICLASS_CMOVNL] = &Instrument_CMOVcc; //312
    instrument_functions[XED_ICLASS_CMOVLE] = &Instrument_CMOVcc; //313
    instrument_functions[XED_ICLASS_CMOVNLE] = &Instrument_CMOVcc; //314
    instrument_functions[XED_ICLASS_NOP] = &Instrument_NOP; // 345

    //  instrument_functions[XED_ICLASS_MOVD] = &Instrument_MOVD; //350
    //  instrument_functions[XED_ICLASS_MOVDQU] = &Instrument_MOVDQU; //351

    //  instrument_functions[XED_ICLASS_MOVDQA] = &Instrument_MOVDQA; //354

    instrument_functions[XED_ICLASS_SETS] = &Instrument_SETcc; //361

    instrument_functions[XED_ICLASS_SETL] = &Instrument_SETcc; //365
    instrument_functions[XED_ICLASS_SETNL] = &Instrument_SETcc; //366
    instrument_functions[XED_ICLASS_SETLE] = &Instrument_SETcc; //367
    instrument_functions[XED_ICLASS_SETNLE] = &Instrument_SETcc; //368

    //  instrument_functions[XED_ICLASS_BTS] = &Instrument_BTS; //370
    instrument_functions[XED_ICLASS_SHRD] = &Instrument_SHRD; //371

    //  instrument_functions[XED_ICLASS_BSF] = &Instrument_BSF; //376
    //  instrument_functions[XED_ICLASS_BSR] = &Instrument_BSR; //377
    instrument_functions[XED_ICLASS_MOVSX] = &Instrument_MOVSX; //378
    instrument_functions[XED_ICLASS_BSWAP] = &Instrument_BSWAP; //379

    //  instrument_functions[XED_ICLASS_PAND] = &Instrument_PAND; //383

    //  instrument_functions[XED_ICLASS_PSUBSW] = &Instrument_PSUBSW; //389

    //  instrument_functions[XED_ICLASS_POR] = &Instrument_POR; //391

    //  instrument_functions[XED_ICLASS_PXOR] = &Instrument_PXOR; //395

    instrument_functions[XED_ICLASS_ROL] = &Instrument_ROL; //472
    instrument_functions[XED_ICLASS_ROR] = &Instrument_ROR; //473
    //  instrument_functions[XED_ICLASS_RCL] = &Instrument_RCL; //474
    //  instrument_functions[XED_ICLASS_RCR] = &Instrument_RCR; //475
    instrument_functions[XED_ICLASS_SHL] = &Instrument_SHL; //476
    instrument_functions[XED_ICLASS_SHR] = &Instrument_SHR; //477
    instrument_functions[XED_ICLASS_SAR] = &Instrument_SAR; //478
    instrument_functions[XED_ICLASS_NOT] = &Instrument_NOT; //479
    instrument_functions[XED_ICLASS_NEG] = &Instrument_NEG; //480
    instrument_functions[XED_ICLASS_MUL] = &Instrument_MUL; //481
    instrument_functions[XED_ICLASS_DIV] = &Instrument_DIV; //482
    instrument_functions[XED_ICLASS_IDIV] = &Instrument_IDIV; //483

    instrument_functions[XED_ICLASS_LDMXCSR] = &Instrument_LDMXCSR; //507
    instrument_functions[XED_ICLASS_STMXCSR] = &Instrument_STMXCSR; //508

    instrument_functions[XED_ICLASS_FLDCW] = &Instrument_FLDCW; //527

    instrument_functions[XED_ICLASS_FNSTCW] = &Instrument_FNSTCW; //529

    /*
       SyscallMonitor takes care of the dirty work of handling system calls
       all that you need to do it give it the system call number of monitor
       and a callback that will be called after the system call and give the
       arguments and the return value.  See syscall_monitor.H for the system 
       call monitor and also syscall_functions.c for the call back functions.
       */

    SyscallMonitor* monitor = new SyscallMonitor();
    monitor->activate();
    // set a default observer that aborts when a program uses a system
    // call that we don't provide a handling function for.
    monitor->setDefaultObserver(UnimplementedSystemCall);

    monitor->addObserver(SYS_access, Handle_ACCESS, 0);
    monitor->addObserver(SYS_alarm, Handle_ALARM, 0);
    monitor->addObserver(SYS_brk, Handle_BRK, 0);
    monitor->addObserver(SYS_chmod, Handle_CHMOD, 0);
    monitor->addObserver(SYS_close, Handle_CLOSE, 0);
    monitor->addObserver(SYS_dup, Handle_DUP, 0);
    monitor->addObserver(SYS_fcntl, Handle_FSTAT64, 0);
    monitor->addObserver(SYS_flock, Handle_FLOCK, 0);
    monitor->addObserver(SYS_fstat, Handle_FSTAT64, 0);
    monitor->addObserver(SYS_fsync, Handle_FSYNC, 0);
    monitor->addObserver(SYS_ftruncate, Handle_FTRUNCATE, 0);
    monitor->addObserver(SYS_getdents64, Handle_GETDENTS64, 0);
    monitor->addObserver(SYS_getpid, Handle_GETPID, 0);
    monitor->addObserver(SYS_gettimeofday, Handle_GETTIMEOFDAY, 0);
    monitor->addObserver(SYS_getuid, Handle_GETUID32, 0);
    monitor->addObserver(SYS_ioctl, Handle_IOCTL, 0);
    monitor->addObserver(SYS_link, Handle_LINK, 0);
    //monitor->addObserver(SYS__llseek, Handle_LLSEEK, 0);
    monitor->addObserver(SYS_lseek, Handle_LSTAT64, 0);
    monitor->addObserver(SYS_lstat, Handle_LSTAT64, 0);
    monitor->addObserver(SYS_mmap, Handle_MMAP, 0);
    //monitor->addObserver(SYS_mmap2, Handle_MMAP2, 0);
    monitor->addObserver(SYS_mprotect, Handle_MPROTECT, 0);
    monitor->addObserver(SYS_munmap, Handle_MUNMAP, 0);

    /* Pin has problems instrumenting the nanosleep system call so we skip it */
    // monitor->addObserver(SYS_nanosleep, Handle_NANOSLEEP, 0);
    monitor->addObserver(SYS_open, Handle_OPEN, 0);
    monitor->addObserver(SYS_read, Handle_READ, 0);
    monitor->addObserver(SYS_readlink, Handle_READLINK, 0);
    monitor->addObserver(SYS_rename, Handle_RENAME, 0);
    monitor->addObserver(SYS_rt_sigaction, Handle_RT_SIGACTION, 0);
    monitor->addObserver(SYS_rt_sigprocmask, Handle_RT_SIGPROCMASK, 0);
    monitor->addObserver(SYS_set_thread_area, Handle_SET_THREAD_AREA, 0);
    monitor->addObserver(SYS_stat, Handle_STAT64, 0);
    //monitor->addObserver(SYS_socketcall, Handle_SOCKETCALL, 0);
    monitor->addObserver(SYS_time, Handle_TIME, 0);
    monitor->addObserver(SYS_uname, Handle_UNAME, 0);
    monitor->addObserver(SYS_unlink, Handle_UNLINK, 0);
    monitor->addObserver(SYS_utime, Handle_UTIME, 0);
    monitor->addObserver(SYS_write, Handle_WRITE, 0);
    monitor->addObserver(SYS_writev, Handle_WRITEV, 0);

    log = fopen("out.log", "w");
    taintAssignmentLog = fopen("taint-log.log", "w");

    // set up first output log
    fnum = 0;
    sprintf(fname, "%s%u.log", KnobOutputFile.Value().c_str(), fnum);
    fout = fopen(fname, "w");
    fnum++;

    // activate instruction counter
    tracing = false;    
    StartCount = KnobStartAt.Value() * 1000000; // early simpoint
    StopCount = KnobStopAt.Value() * 1000000; // run length
    icount.Activate();
    accs = 0;

    // Never returns
    PIN_StartProgram();
    
    fclose(log);
    fclose(taintAssignmentLog);
    return 1;
}
