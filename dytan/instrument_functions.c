/************************************************
 *      Copyright 2007                          *
 *      Georgia Tech Research Corporation       *
 *      Atlanta, GA  30332-0415                 *
 *      All Rights Reserved                     *
 ************************************************/

// Bitmasks for accessing specific parts of eflags
#define CF_MASK 1 << 1 // bit 1
#define PF_MASK 1 << 2 // bit 2
#define AF_MASK 1 << 4 // bit 4
#define ZF_MASK 1 << 6 // bit 6
#define SF_MASK 1 << 7 // bit 7
#define OF_MASK 1 << 11 // bit 11

static void Instrument_MUL(INS, void *);

void UnimplementedInstruction(INS ins, void * v)
{
  fprintf(log, "%s unimplemented[%d]\n", INS_Disassemble(ins).c_str(),INS_Opcode(ins));
  //printf("%s unimplemented[%d]\n", INS_Disassemble(ins).c_str(), INS_Opcode(ins));
  //exit(-1);
}

void Instrument_NOP(INS ins, void *v)
{
  // don't do anything, it's a nop
}


//CMOVcc check functions
ADDRINT CheckCMOVNB(ADDRINT eflags)
{ 
  return ((eflags & CF_MASK) == 0);
}

ADDRINT CheckCMOVB(ADDRINT eflags)
{ 
  return ((eflags & CF_MASK) == 1);
}

ADDRINT CheckCMOVBE(ADDRINT eflags)
{ 
  return (((eflags & CF_MASK) == 1) || ((eflags & ZF_MASK) == 1));
}

ADDRINT CheckCMOVNLE(ADDRINT eflags)
{ 
  return ((eflags & ZF_MASK) == 0) && ((eflags & SF_MASK) == 0);
}

ADDRINT CheckCMOVNL(ADDRINT eflags)
{ 
  return ((eflags & SF_MASK) == (eflags & OF_MASK));
}

ADDRINT CheckCMOVL(ADDRINT eflags)
{ 
  return ((eflags & SF_MASK) != (eflags & OF_MASK));
}

ADDRINT CheckCMOVLE(ADDRINT eflags)
{ 
  return (((eflags & ZF_MASK) == 1) || (eflags & SF_MASK != eflags & OF_MASK));
}

ADDRINT CheckCMOVNBE(ADDRINT eflags)
{ 
  return ((eflags & CF_MASK) == 0) && ((eflags & ZF_MASK) == 0);
}

ADDRINT CheckCMOVNZ(ADDRINT eflags)
{ 
  return ((eflags & ZF_MASK) == 0);
}

ADDRINT CheckCMOVNO(ADDRINT eflags)
{ 
  return ((eflags & OF_MASK) == 0);
}

ADDRINT CheckCMOVNP(ADDRINT eflags)
{ 
  return ((eflags & PF_MASK) == 0);
}

ADDRINT CheckCMOVNS(ADDRINT eflags)
{ 
  return ((eflags & SF_MASK) == 0);
}

ADDRINT CheckCMOVO(ADDRINT eflags)
{ 
  return ((eflags & OF_MASK) == 1);
}

ADDRINT CheckCMOVP(ADDRINT eflags)
{ 
  return ((eflags & PF_MASK) == 1);
}

ADDRINT CheckCMOVS(ADDRINT eflags)
{ 
  return ((eflags & SF_MASK) == 1);
}

ADDRINT CheckCMOVZ(ADDRINT eflags)
{ 
  return ((eflags & ZF_MASK) == 1);
}

//CMPXCHG check functions
ADDRINT CheckEqual_r_r(ADDRINT v1, ADDRINT v2)
{
  return v1 == v2;
}

ADDRINT CheckNotEqual_r_r(ADDRINT v1, ADDRINT v2)
{
  return v1 != v2;
}

ADDRINT CheckEqual_m_r(ADDRINT start, ADDRINT size, ADDRINT v2)
{
  ADDRINT v1 = *((ADDRINT *) start);
  return v1 == v2;
}

ADDRINT CheckNotEqual_m_r(ADDRINT start, ADDRINT size, ADDRINT v2)
{
  ADDRINT v1 = *((ADDRINT *) start);
  return v1 == v2;
}

static void Instrument_ADC(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //	 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //	 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // Insert calls that copy the taint marks associated with the
  // destination argument into the global storage of dest
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {

    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  // Insert calls that copy the taint marks associated with the
  // source argument into the global storage of src
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  // Insert calls that copy the taint marks associated with the
  // eflags into the global storage of eflags
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, eflags,
		 IARG_END);

  // Insert function call to propagate taint marks from dest, src, eflags
  // to dest.
  //dest <- dest, src, eflags
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 4,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_PTR, eflags,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_ADDRINT, 5,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_PTR, eflags,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  // Insert call to propagate taint marks from dest, src, eflags to eflags
  //eflags <- dest, src, eflags
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 4,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_PTR, eflags,
		 IARG_END);
}


// This is a good example of how instructions are modeled and taint is 
// propagated in dytan.

// To implement one of these functions the first thing to do is decide
// how taint marks should be propagated.

// In this instance, for ADD, the destination operand and the eflags register
// are tainted with the union of the taint marks associated with the 
// destination and source operands.

// dest, eflags <- union(dest, src)
 
// Then the general implementation goes like this
// 1. load the taint marks associated with the operands on the right hand
//    side of the equation (dest, src)
// 2. figure out the union
// 3. assign the union to the operands on the left hand side (dest, eflags)

static void Instrument_ADD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // Figure out what type (register, immediate, memory) the destination
  // operand is.  According to the IA-32 developer's manual the only 
  // possibilities are register or memory

  // INS_OperandIsReg is Pin's function to check if an operand is a register
  // for the add instruction we know that 0 (second operand) corresponds
  // to the destination operand.  This may not be true for later versions of
  // Pin which is why we require a specific version.
  if(INS_OperandIsReg(ins, 0)) {

    // If the operand is a register insert a call before the instruction
    // to load the taint marks currently associated with the register into
    // the dest region of memory. 

    // In dytan.cpp there are a few global memory regions that are used
    // to pass taint marks around at runtime.  The names correspond to
    // common operand names (dest, src, cnt, eflagsm ...)
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }

  // Similar to above but now we check if the destination is a memory reference
  else if(INS_OperandIsMemory(ins, 0)) {
    // Needs to have a second check here to account for cases
    // where Pin can not calculate memory addresses that use certain memory
    // selectors.  (i.e. Pin know that the operand is a memory reference
    // but can't figure out exactly what memory is being accessed.  In this
    // case we bail since we need to know what memory is read/written
    if(!INS_IsMemoryRead(ins)) return;
    
    // Load the taint marks currently associated with the read memory area 
    // into dest
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  // Do the same thing for the source operand as for the destination operand.
  // Here we have three possibilities, register, memory, or immediate
  
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    // If the operand is an immediate, by definition, it doesn't have taint
    // but since src is a shared global it may have a residual value so 
    // clear it just to make sure.
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  // Assign the union of dest and src to dest.
  // SetTaintForRegister and SetTaintForMemory take a NULL terminate list
  // of taint mark sets, unions them, and assigns the result to its first
  // argument

  //dest <- dest, src

  // Since the destination can be either a register or a memory location
  // handle each case appropriatly
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_ADDRINT, 4,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  // Assign the union to the eflags register.

  //eflags <- dest, src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_AND(INS ins, void *v)
{
  Instrument_ADD(ins, v);
}

static void Instrument_BSR(INS ins, void *v)
{
  Instrument_ADD(ins, v);
}

static void Instrument_BSWAP(INS ins, void *v)
{
  //pass
}

static void Instrument_FLDZ(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // loads +0.0. into register
  // doesn't do anything
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		 IARG_ADDRINT, INS_OperandReg(ins, 1),
		 IARG_END);
}

// the call near is an instance of a taint sink
static void Instrument_CALL_NEAR(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // 9-20-2008 - right now we're doing two checks on each call instruction
  // the first check is for the taintedness of the source of the call
  // target and the second check is for the taintedness of the actual
  // call target - is the second check redundant(?)

  // for the call instruction, only operand 1 is of interest
  // disallow calling injected addresses or functions  
  if(INS_OperandIsReg(ins, 0)) {
    //printf("call near instruction using registers\n");
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
    //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckControlPointer), 
    //		   IARG_REG_VALUE, INS_OperandReg(ins, 0),
		   //IARG_REG_VALUE, LEVEL_BASE::REG_SEG_CS,
    //		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {    
    //printf("call near instruction using memory\n");

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
    //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckControlPointer),
    //		   IARG_ADDRINT, IARG_MEMORY_VALUE,
		   //IARG_REG_VALUE, LEVEL_BASE::REG_SEG_CS,
    //		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 0)){ 
    //printf("call near instruction using immediate\n");
    // If the operand is an immediate or displacement, by definition, 
    // it doesn't have taint but since src is a shared global it may 
    // have a residual value so clear it just to make sure.
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
    
    //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckControlPointer),
    //		   IARG_ADDRINT, INS_OperandImmediate(ins, 0),
		   //IARG_REG_VALUE, LEVEL_BASE::REG_SEG_CS,
    //		   IARG_END);
  }
  else if(INS_OperandIsBranchDisplacement(ins, 0)) {
    //printf("Call near instruction using displacement\n");
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
    
    //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckControlPointer),
    //		   IARG_ADDRINT, INS_DirectBranchOrCallTargetAddress(ins),
		   //IARG_REG_VALUE, LEVEL_BASE::REG_SEG_CS,
    //		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
  //		 IARG_PTR, src,
  //		 IARG_ADDRINT, 0,
  //		 IARG_END);
  
  // afterwards, iterate through memory addresses and registers
  // updated by the call instruction and update their taint information
  /*cout << "Call instruction: ";
  for(uint i=0; i<INS_OperandCount(ins); i++){
    if(INS_OperandIsReg(ins, i)) {
      printf(" Operand %d is Register %s,", i, REG_StringShort(INS_OperandReg(ins, i)).c_str());
    }
    else if(INS_OperandIsMemory(ins, i)) {    
      printf(" Operand %d is Memory,", i);
    }
    else if(INS_OperandIsImmediate(ins, i)){
      printf(" Operand %d is Immediate,", i);
    }
    else if(INS_OperandIsBranchDisplacement(ins, i)) {
      printf(" Operand %d is branch displacement,", i);
    }
    else {
      printf(" Operand %d is unknown,", i);
    }
  }
  cout << endl;
  exit(-1);*/

  if(INS_IsMemoryWrite(ins)){

    if(INS_MemoryWriteSize(ins) > 8){
      printf("Why does call instruction write more than 4 bytes\n");
      exit(-1);
    }

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_END);
  }else{
    printf("Why does call instruction not write the return address\n");
    exit(-1);
  }
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_ESP,
		 IARG_END);
}

static void Instrument_CDQ(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  UINT32 operandWidth = INS_OperandWidth(ins, 0);
  if(32 == operandWidth) {

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_END);
    // edx <- eax
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		   IARG_PTR, eax,
		   IARG_END);
    
  }
  else {
    printf("Unhandled operand size: %s\n", INS_Disassemble(ins).c_str());
    //exit(-1);
  }
}

static void Instrument_CLD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //eflags <- clear
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_END);
}

static void Instrument_CMOVcc(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  xed_iclass_enum_t opcode = (xed_iclass_enum_t) INS_Opcode(ins);

  // CF == 0
  if(XED_ICLASS_CMOVNB == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNB),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // CF == 1
  else if(XED_ICLASS_CMOVB == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVB),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // CF == 1 || ZF == 1
  else if(XED_ICLASS_CMOVBE == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVBE),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // ZF == 0 && SF == OF
  else if(XED_ICLASS_CMOVNLE == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNLE),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // SF == OF
  else if(XED_ICLASS_CMOVNL == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNL),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // SF != OF
  else if(XED_ICLASS_CMOVL == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVL),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // ZF == 1 || SF != OF
  else if(XED_ICLASS_CMOVLE == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVLE),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // CF == 0 && ZF == 0
  else if(XED_ICLASS_CMOVNBE == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNBE),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // ZF == 0
  else if(XED_ICLASS_CMOVNZ == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNZ),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // OF == 0
  else if(XED_ICLASS_CMOVNO == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNO),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // PF == 0
  else if(XED_ICLASS_CMOVNP == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNP),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // SF == 0
  else if(XED_ICLASS_CMOVNS == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVNS),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // OF == 1
  else if(XED_ICLASS_CMOVO == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVO),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // PF == 1
  else if(XED_ICLASS_CMOVP == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVP),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }

  // SF == 1
  else if(XED_ICLASS_CMOVS == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVS),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  
  // ZF == 1
  else if(XED_ICLASS_CMOVZ == opcode) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckCMOVZ),
		     IARG_REG_VALUE, LEVEL_BASE::REG_EFLAGS,
		     IARG_END);
  }
  else {
    printf("Unhandled cmov type: %s\n", INS_Disassemble(ins).c_str());
    //exit(-1);
  }

  //dest <- src
  INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 2,
		 IARG_ADDRINT, INS_OperandReg(ins, 0),
		 IARG_PTR, src,
		 IARG_END);

}

static void Instrument_CMP(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());    
	exit(-1);
  }
  
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
  //		 IARG_PTR, src,
  //		 IARG_ADDRINT, 2,
  //		 IARG_END);  
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
  //		 IARG_PTR, dest,
  //		 IARG_ADDRINT, 2,
  //		 IARG_END);

  //eflags <- dest, src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_CMPSB(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //Memory reference [edi]
  if(!INS_IsMemoryRead(ins)) return;  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		 IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		 IARG_ADDRINT, LEVEL_BASE::REG_EDI,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_PTR, dest,
		 IARG_END);
  
  //Memory reference [esi]
  if(!INS_IsMemoryRead(ins)) return;  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		 IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE,
		 IARG_ADDRINT, LEVEL_BASE::REG_ESI,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_PTR, src,
		 IARG_END);

  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
  //		 IARG_PTR, src,
  //		 IARG_ADDRINT, 2,
  //		 IARG_END);  
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
  //		 IARG_PTR, dest,
  //		 IARG_ADDRINT, 2,
  //		 IARG_END);

  //eflags <- dest, src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_CMPXCHG(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  UINT32 operandWidth = INS_OperandWidth(ins, 0);
  if(operandWidth < 65) {
    if(INS_OperandIsReg(ins, 0)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, dest,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 0)) {
      if(!INS_IsMemoryRead(ins)) return;
      
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		     IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		     IARG_PTR, dest,
		     IARG_END);
    }
    else {
      printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 2),
		   IARG_PTR, eax,
		   IARG_END);
  }
  else {
    printf("Unhanded operand width: %d for %s\n", operandWidth,
	    INS_Disassemble(ins).c_str());
    exit(-1);
  }

  /*
    if(eax == dest) {
      dest = src
    }
    else {
      eax = src
    }
  */
  
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckEqual_r_r),
		     IARG_REG_VALUE, INS_OperandReg(ins, 0),
		     IARG_REG_VALUE, INS_OperandReg(ins, 2),
		     IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		       IARG_UINT32, 2,
		       IARG_ADDRINT, INS_OperandReg(ins, 0),
		       IARG_PTR, src,
		       IARG_END);

    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckNotEqual_r_r),
		     IARG_REG_VALUE, INS_OperandReg(ins, 0),
		     IARG_REG_VALUE, INS_OperandReg(ins, 2),
		     IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		       IARG_UINT32, 2,
		       IARG_ADDRINT, INS_OperandReg(ins, 2),
		       IARG_PTR, src,
		       IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckEqual_m_r),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_REG_VALUE, INS_OperandReg(ins, 2),
		     IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),		
		   IARG_ADDRINT, 3,  
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, src,
		   IARG_END);
    
    INS_InsertIfCall(ins, IPOINT_BEFORE, AFUNPTR(CheckNotEqual_m_r),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_REG_VALUE, INS_OperandReg(ins, 2),
		     IARG_END);
    INS_InsertThenCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		       IARG_UINT32, 2,
		       IARG_ADDRINT, INS_OperandReg(ins, 2),
		       IARG_PTR, src,
		       IARG_END);

  }
}

static void Instrument_CWDE(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AX,
		 IARG_PTR, dest,
		 IARG_END);
  
  // eax <- ax
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 2,
		 IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		 IARG_PTR, dest,
		 IARG_END);
}

static void Instrument_DEC(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  //eflags <- dest
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 2,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_END);
}

static void Instrument_DIV(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  UINT32 operandWidth = INS_OperandWidth(ins, 0);

  if(32 == operandWidth) {
    if(INS_OperandIsReg(ins, 0)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 0)) {
      if(!INS_IsMemoryRead(ins)) return;
      
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		     IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else {
      printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_END);
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		   IARG_PTR, edx,
		   IARG_END);
    
    //eax <- eax, edx, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 4,
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_PTR, edx,
		   IARG_PTR, src,
		   IARG_END);
    
    //edx <- eax, edx, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 4,
		   IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		   IARG_PTR, eax,
		   IARG_PTR, edx,
		   IARG_PTR, src,
		   IARG_END);

    //eflags <- clear
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		   IARG_END);
  }
  else {
    printf("Unhandled operand size: %s\n", INS_Disassemble(ins).c_str());
    //exit(-1);
  }

}

static void Instrument_FLDCW(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //pass
}

static void Instrument_FNSTCW(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_END);
}

static void Instrument_HLT(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //pass
}

static void Instrument_IDIV(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  UINT32 operandWidth = INS_OperandWidth(ins, 0);

  if(operandWidth < 65) {
    
    if(INS_OperandIsReg(ins, 0)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 0)) {
      if(!INS_IsMemoryRead(ins)) return;
      
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		     IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else {
      printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		   IARG_PTR, edx,
		   IARG_END);
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_END);

    // eax <- edx, eax, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 4,
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_PTR, edx,
		   IARG_PTR, src,
		   IARG_END);

    // edx <- edx, eax, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 4,
		   IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		   IARG_PTR, eax,
		   IARG_PTR, edx,
		   IARG_PTR, src,
		   IARG_END);

    //eflags <- clear
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		   IARG_END);
  }
  else {
    printf("Unhanded operand width: %d for %s\n", operandWidth,
	    INS_Disassemble(ins).c_str());
    exit(-1);
  }
}

static void Instrument_IMUL(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  int operand_count = 0;
  for(unsigned int i = 0; i < INS_OperandCount(ins); i++) {
    if(!INS_OperandIsImplicit(ins, i)) operand_count++;
  }

  if(1 == operand_count) {
    Instrument_MUL(ins, v);
  }
  else if(2 == operand_count) {

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
    
    if(INS_OperandIsReg(ins, 1)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 1)) {
      if(!INS_IsMemoryRead(ins)) return;
      
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		     IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsImmediate(ins, 1)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		     IARG_PTR, src,
		     IARG_END);
    }
    else {
      printf("Unknown operand type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }

    //dest <- dest, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);

    //eflags <- dest, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(3 == operand_count) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
    
    if(INS_OperandIsReg(ins, 1)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 1)) {
      if(!INS_IsMemoryRead(ins)) return;
      
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		     IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		     IARG_PTR, src,
		     IARG_END);
    }
    else {
      printf("Unknown operand type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }

    //dest <- dest, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);

    //eflags <- dest, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);
    
  }
  else {
    printf("\tStrange IMUL %s %d\n", 
	   INS_Disassemble(ins).c_str(), operand_count);
    exit(-1);
  }
}

static void Instrument_INC(INS ins, void *v)
{
  Instrument_DEC(ins, v);
}

static void Instrument_INT(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //pass
}

static void Instrument_Jcc(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // Conditional branches
  // TODO - implement the taint sink
  // for the jcc instruction, only operand 1 is of interest
  // disallow return to injected addresses or functions
  
  /*
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 0) || 
	  INS_OperandIsBranchDisplacement(ins, 0)) {
    // If the operand is an immediate or displacement, by definition, 
    // it doesn't have taint but since src is a shared global it may 
    //have a residual value so clear it just to make sure.
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
		 IARG_PTR, src,
		 IARG_END);
  */
}

static void Instrument_JMP(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // unconditional jumps - check indirect addresses
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 0) ||
	  INS_OperandIsBranchDisplacement(ins, 0)){
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, dest,
		   IARG_END);
  }  
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  /*INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
		 IARG_PTR, dest,
		 IARG_ADDRINT, 2,
		 IARG_END);  */
}

static void Instrument_LEA(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		 IARG_PTR, base,
		 IARG_END);
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		 IARG_PTR, idx,
		 IARG_END);

  // dest <- base, idx
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, INS_OperandReg(ins, 0),
		 IARG_PTR, base,
		 IARG_PTR, idx,
		 IARG_END);
}

static void Instrument_LEAVE(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //TODO
}

static void Instrument_LDMXCSR(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //pass
}

static void Instrument_MOV(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 IARG_PTR, (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1) || !(INS_OperandIsImmediate(ins, 1))) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
	//cerr << "Unknown operand(1) type: " << INS_Disassemble(ins) << endl;
    //abort();
	exit(-1);
  }
  
  //dest <- src
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
#ifdef IMPLICIT
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_PTR, base,
		   IARG_END);
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, idx,
		   IARG_END);
#endif    
    if(!INS_IsMemoryWrite(ins)) return;
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),	
#ifdef IMPLICIT
		   IARG_UINT32, 5,
#else
		   IARG_UINT32, 3,
#endif	
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, src,
#ifdef IMPLICIT
		   IARG_PTR, base,
		   IARG_PTR, idx,
#endif
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
	//cerr << "Unknown operand(1) type: " << INS_Disassemble(ins) << endl;
    //abort();
	exit(-1);
  }
}

static void Instrument_MOVSB(INS ins, void *v) 
{
  if(INS_RepPrefix(ins)){
    //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
    //		   IARG_ADDRINT, IARG_INST_PTR,
    //		   (INS_Disassemble(ins)).c_str(),
    //		   IARG_END);		 

    // CheckLoadOrStore(ins, v);

    //printf("REP MOVSB called\n");
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRepMov),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_END);
  }
  else{
    Instrument_MOV(ins, v);
  }
}

static void Instrument_MOVSD(INS ins, void *v)
{
  if(INS_RepPrefix(ins)){
  //  printf("REP MOVSD called\n");
    Instrument_MOVSB(ins, v);
  }
  else{
    Instrument_MOV(ins, v);
  }
}

static void Instrument_MOVSW(INS ins, void *v)
{
  if(INS_RepPrefix(ins)){
  //  printf("REP MOVSW called\n");
    Instrument_MOVSB(ins, v);
  }
  else{
    Instrument_MOV(ins, v);
  }
}

static void Instrument_MOVSX(INS ins, void *v)
{
  if(INS_RepPrefix(ins)){
  //  printf("REP MOVSX called\n");
    Instrument_MOVSB(ins, v);
  }
  else{
    Instrument_MOV(ins, v);
  }
}

static void Instrument_MOVZX(INS ins, void *v)
{
  if(INS_RepPrefix(ins)){
  //  printf("REP MOVZX called\n");
    Instrument_MOVSB(ins, v);
  }
  else{
    Instrument_MOV(ins, v);
  }
}

static void Instrument_MUL(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  UINT32 operandWidth = INS_OperandWidth(ins, 0);

  if(65 > operandWidth) {
    if(INS_OperandIsReg(ins, 0)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 0)) {
      if(!INS_IsMemoryRead(ins)) return;

      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		     IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		     IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		     IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		     IARG_PTR, src,
		     IARG_END);
    }
    else {
      printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_END);
    
    
    //eax <- eax, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		   IARG_PTR, eax,
		   IARG_PTR, src,
		   IARG_END);
    
    //edx <- eax, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		   IARG_PTR, eax,
		   IARG_PTR, src,
		   IARG_END);
    
    //eflags <- eax, src
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		   IARG_PTR, eax,
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unhanded operand width: %d for %s\n", operandWidth,
	    INS_Disassemble(ins).c_str());
    fflush(log);
    exit(-1);
  }
}

static void Instrument_NEG(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  //eflags <- dest
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 2,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_END);
}

static void Instrument_NOT(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //pass
}

static void Instrument_OR(INS ins, void *v)
{
  Instrument_AND(ins, v);
}

static void Instrument_PAUSE(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //pass
}

static void Instrument_POP(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(!INS_IsMemoryRead(ins)) return;
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		 IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_PTR, src,
		 IARG_END);

  //dest <- src
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
#ifdef IMPLICIT
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_PTR, base,
		   IARG_END);

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, idx,
		   IARG_END);
#endif

    if(!INS_IsMemoryWrite(ins)) return;
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
#ifdef IMPLICIT
		   IARG_UINT32, 5,
#else
		   IARG_UINT32, 3,
#endif	
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, src,
#ifdef IMPLICIT
		   IARG_PTR, base,
		   IARG_PTR, idx,
#endif
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
}

static void Instrument_POPFD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(!INS_IsMemoryRead(ins)) return;
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		 IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- top of stack

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_PUSH(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  } 
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  if(!INS_IsMemoryWrite(ins)) return;
  // dest <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		 IARG_UINT32, 3,
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_PUSHFD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		 IARG_ADDRINT, 3,
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_PTR, src,
		 IARG_END);
  
}

static void Instrument_RDTSC(INS ins, void *v)
{
  
  //  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);
  
  // eax <- clear
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EAX,
		 IARG_END);

  // edx <- clear
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EDX,
		 IARG_END);
}

static void Instrument_BTS(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // memory location <- clear
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_END);
}

// return instruction is an instance of a taint sink
static void Instrument_RET_NEAR(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  // read return address from stack to EIP (check return address for taint)
  // load ESP from stack (propagate taint from stack to esp)
  // optionally subtract imm from ESP

  if(INS_IsMemoryRead(ins)){
    
    if(INS_MemoryReadSize(ins) > 4){
      printf("Why does ret instruction read more than 4 bytes\n");
      exit(-1);
    }
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		   IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		   IARG_PTR, src,
		   IARG_END);
    //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckControlPointer),
    //	   IARG_ADDRINT, IARG_MEMORY_VALUE,
		   //IARG_REG_VALUE, LEVEL_BASE::REG_SEG_CS,
    //	   IARG_END);

  }else{
    printf("Is this a return instruction? %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckTaint),
		 //IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
  //		 IARG_PTR, src, 
  //		 IARG_ADDRINT, 1,
  //		 IARG_END);
}

static void Instrument_SAR(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else if(strstr(INS_Disassemble(ins).c_str(), "one") != NULL){
    // for some reason one is used as an immediate in some
    // encodings
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //dest <- dest, count
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_UINT32, 4,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, dest,
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //eflags <- dest, count
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, cnt,
		 IARG_END);
}

static void Instrument_SBB(INS ins, void *v)
{
  Instrument_ADC(ins, v);
}

static void Instrument_SCASB(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  //Memory reference [edi]
  if(!INS_IsMemoryRead(ins)) return;
    
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		 IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		 IARG_ADDRINT, LEVEL_BASE::REG_EDI,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_PTR, src,
		 IARG_END);
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, eax,
		 IARG_END);

  //eflags <- src, al
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_PTR, eax,
		 IARG_END);

  if(INS_RepPrefix(ins) || INS_RepnePrefix(ins)) { 
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, LEVEL_BASE::REG_ECX,
		   IARG_PTR, src,
		   IARG_PTR, eax,
		   IARG_END);
  }

}

static void Instrument_SETcc(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);

  //dest <- src
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;

#ifdef IMPLICIT
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_PTR, base,
		   IARG_END);
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, idx,
		   IARG_END);
#endif

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
#ifdef IMPLICIT
		   IARG_UINT32, 5,
#else
		   IARG_UINT32, 3,
#endif
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, base,
#ifdef IMPLICIT
		   IARG_PTR, idx,
		   IARG_PTR, src,
#endif		   
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
}

static void Instrument_SHL(INS ins, void *v)
{
  Instrument_SAR(ins, v);
}

static void Instrument_SHLD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, INS_OperandReg(ins, 1),
		 IARG_PTR, cnt,
		 IARG_END);
  
  if(INS_OperandIsReg(ins, 2)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 2)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //dest <- dest, src, count
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 4,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_UINT32, 5,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_PTR, cnt,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //eflags <- dest, src, count
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 4,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_PTR, cnt,
		 IARG_END);
}


static void Instrument_SHR(INS ins, void *v)
{
  Instrument_SAR(ins, v);
}

static void Instrument_SHRD(INS ins, void *v)
{
  Instrument_SHLD(ins, v);
}

static void Instrument_STD(INS ins, void *v) 
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_END);
}

static void Instrument_STMXCSR(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_END);
}

static void Instrument_STOSB(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, eax,
		 IARG_END);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
	 IARG_ADDRINT, LEVEL_BASE::REG_EDI,
		 IARG_PTR, src,
		 IARG_END);
  
  if(!INS_IsMemoryWrite(ins)) return;

  //dest <- edi, al
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
#ifdef IMPLICIT
		 IARG_UINT32, 4,
#else
		 IARG_UINT32, 3,
#endif
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_PTR, eax,
#ifdef IMPLICIT
		 IARG_PTR, src,
#endif
		 IARG_END);
}

static void Instrument_STOSD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AX,
		 IARG_PTR, eax,
		 IARG_END);
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EDI,
		 IARG_PTR, src,
		 IARG_END);
  
  //dest <- edi, al
  if(!INS_IsMemoryWrite(ins)) return;
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
#ifdef IMPLICIT
		 IARG_UINT32, 4,
#else
		 IARG_UINT32, 3,
#endif
		 IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		 IARG_PTR, eax,
#ifdef IMPLICIT
		 IARG_PTR, src,
#endif
		 IARG_END);
}

static void Instrument_SUB(INS ins, void *v)
{
  Instrument_ADD(ins, v);
}

static void Instrument_TEST(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    fflush(log);
    exit(-1);
  }
  
  //eflags <- dest, src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_XADD(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
	
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //dest <- dest, src
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 3,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_ADDRINT, 4,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, dest,
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  //src <- dest
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 2,
		 IARG_ADDRINT, INS_OperandReg(ins, 1),
		 IARG_PTR, dest,
		 IARG_END);

  //eflags <- dest, src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_END);
}

static void Instrument_XCHG(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryRead(ins)) return;
	
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }

  //dest <- src
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryWrite(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_ADDRINT, 3,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  //src <- dest
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryWrite(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
		   IARG_ADDRINT, 3,
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
}

static void Instrument_XOR(INS ins, void *v)
{
  //INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(CheckInstructionTaint),
  //		 IARG_ADDRINT, IARG_INST_PTR,
  //		 (INS_Disassemble(ins)).c_str(),
  //		 IARG_END);		 

  // CheckLoadOrStore(ins, v);

  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, dest,
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  if(INS_OperandIsReg(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 1)) {
    if(!INS_IsMemoryRead(ins)) return;
    
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		   IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 1),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 1),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsImmediate(ins, 1)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintSet),
		   IARG_PTR, src,
		   IARG_END);
  }
  else {
    printf("Unknown operand(1) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
  
  if(INS_OperandIsReg(ins, 0) && INS_OperandIsReg(ins, 1)
     && INS_OperandReg(ins, 0) == INS_OperandReg(ins, 1)) {
    
    //dest <- clear
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0) && INS_OperandIsMemory(ins, 1)
	 && (INS_OperandMemoryDisplacement(ins, 0) + INS_OperandMemoryBaseReg(ins, 0) +
	 INS_OperandMemoryIndexReg(ins, 0) * INS_OperandMemoryScale(ins, 0))  
	 == (INS_OperandMemoryDisplacement(ins, 1) + INS_OperandMemoryBaseReg(ins, 1) +
	 INS_OperandMemoryIndexReg(ins, 1) * INS_OperandMemoryScale(ins, 1))) {
		 
    //dest <- clear    
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_END);
  }
  else {
    //dest <- dest, src
    if(INS_OperandIsReg(ins, 0)) {
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		     IARG_UINT32, 3,
		     IARG_ADDRINT, INS_OperandReg(ins, 0),
		     IARG_PTR, dest,
		     IARG_PTR, src,
		     IARG_END);
    }
    else if(INS_OperandIsMemory(ins, 0)) {
      if(!INS_IsMemoryWrite(ins)) return;
    
      INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
			 IARG_ADDRINT, 4,
		     IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		     IARG_PTR, dest,
		     IARG_PTR, src,
		     IARG_END);
    }
    else {
      printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
      exit(-1);
    }
  }
  
  //eflags <- dest, src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 3,
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, dest,
		 IARG_PTR, src,
		 IARG_END);

}

/*
static void Instrument_DAA(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
  
}

static void Instrument_DAS(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
  
}

static void Instrument_AAA(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
  
}

static void Instrument_AAS(INS ins, void *v)
{
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AL,
		 IARG_PTR, src,
		 IARG_END);

  //eflags <- src
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
  
}
*/

static void Instrument_ROL(INS ins, void *v)
{
  Instrument_SAR(ins, v);
}

static void Instrument_ROR(INS ins, void *v)
{
  //Instrument_SAR(ins, v);
  // CheckLoadOrStore(ins, v);
}

static void Instrument_BSF(INS ins, void *v)
{
  // bit scan forward
  // Searches the source operand (second operand) for the least significant set bit (1 bit).
  // If a least significant 1 bit is found, its bit index is stored in the destination operand
  // (first operand). The source operand can be a register or a memory location; the
  // destination operand is a register. The bit index is an unsigned offset from bit 0 of the
  // source operand. If the content of the source operand is 0, the content of the destination
  // operand is undefined.
  
  // doesn't fit description of a mathematical operation, so nothing here
  
  // CheckLoadOrStore(ins, v);
}

static void Instrument_BT(INS ins, void *v)
{
  // bit test
  // CheckLoadOrStore(ins, v);
}

static void Instrument_BTR(INS ins, void *v)
{
  // bit test
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FLD(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FXAM(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FCOMP(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FYL2X(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FUCOMI(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FADD(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FWAIT(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FRNDINT(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FUCOMP(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FSUBR(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FYL2XP1(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FABS(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FSUB(INS ins, void *v)
{
  // floating point load
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FCHS(INS ins, void *v)
{
  // change sign  
  // CheckLoadOrStore(ins, v);
}

static void Instrument_FSTP(INS ins, void *v)
{
  // store floating point value and pop
  Instrument_MOV(ins, v);
  
  // Instrument_POP(ins, v);
  // entire instruction inlined here because this instruction
  // should access memory only once
  
  if(!INS_IsMemoryRead(ins)) return;
  
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForMemory),
		 IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_ADDRINT, LEVEL_BASE::REG_INVALID,
		 IARG_PTR, src,
		 IARG_END);

  //dest <- src
  if(INS_OperandIsReg(ins, 0)) {
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		   IARG_UINT32, 2,
		   IARG_ADDRINT, INS_OperandReg(ins, 0),
		   IARG_PTR, src,
		   IARG_END);
  }
  else if(INS_OperandIsMemory(ins, 0)) {
#ifdef IMPLICIT
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryBaseReg(ins, 0),
		   IARG_PTR, base,
		   IARG_END);

    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		   IARG_ADDRINT, INS_OperandMemoryIndexReg(ins, 0),
		   IARG_PTR, idx,
		   IARG_END);
#endif

    if(!INS_IsMemoryWrite(ins)) return;
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForMemory),
#ifdef IMPLICIT
		   IARG_ADDRINT, 5,
#else
		   IARG_ADDRINT, 3,
#endif
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_PTR, src,
#ifdef IMPLICIT
		   IARG_PTR, base,
		   IARG_PTR, idx,
#endif
		   IARG_END);
  }
  else {
    printf("Unknown operand(0) type: %s\n", INS_Disassemble(ins).c_str());
    exit(-1);
  }
}

static void Instrument_CPUID(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);

  for(unsigned int i = 0; i < INS_OperandCount(ins); i++) {
	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForRegister),
	IARG_ADDRINT, INS_OperandReg(ins, i),
	IARG_END);
  }
}

static void Instrument_SAHF(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);

  // Loads the SF, ZF, AF, PF, and CF flags of the EFLAGS register
  // with values from the corresponding bits in the AH register
  // (bits 7, 6, 4, 2, and 0). Bits 1, 3, and 5 of register AH are
  // ignored

  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(TaintForRegister),
		 IARG_ADDRINT, LEVEL_BASE::REG_AH,
		 IARG_PTR, src,
		 IARG_END);
  INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SetTaintForRegister),
		 IARG_UINT32, 2,
		 IARG_ADDRINT,  LEVEL_BASE::REG_EFLAGS,
		 IARG_PTR, src,
		 IARG_END);
}

/* ===================================================================== */
// floating point barrier
/* ===================================================================== */

static void Instrument_FILD(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FLD1(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FLDL2E(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FDIV(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FXSAVE(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  if(INS_IsMemoryWrite(ins)){
    INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(ClearTaintForMemory),
		   IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE,
		   IARG_END);
  }
}

static void Instrument_FDIVP(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FDIVRP(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FLDLN2(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FADDP(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FMUL(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FMULP(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FST(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FUCOM(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FISTP(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FXCH(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FUCOMPP(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FNSTSW(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FIADD(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FIMUL(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}

static void Instrument_FDIVR(INS ins, void *v)
{
  // CheckLoadOrStore(ins, v);
  
  // anything else?
}
