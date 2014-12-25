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

#include "replace_functions.h"
#include "dytan.h"
/*
  This file sets of convience functions that can be called from an
  application to interact with the tool


  Current we can assigned taint marks, dump taint marks, and control
  if logging is enabled
 */


void SetTrace(int trace)
{
  tracing = trace;
}

map<string, int> tagMap;
void AssignTagToByteRange(ADDRINT start, size_t size, char * name)
{
  int taintMark;

  if(tagMap.end() == tagMap.find(string(name))) {
    tagMap[string(name)] = currentTaintMark;
    currentTaintMark++;
    //if(currentTaintMark == NUMBER_OF_TAINT_MARKS) currentTaintMark = 0;
  }
  
  bitset2 *s = bitset_init(NUMBER_OF_TAINT_MARKS);
  bitset_set_bit(s, tagMap[string(name)]);
  
  const char *sep = "";
  printf("assigning location %#x-%#x taint marks [", start, start + size - 1);
  printf("%s]\n", bitset_str(s));

  /*for(int i = 0; i < s->nbits; i++) {
    if(bitset_test_bit(s, i)) {
      printf("%s%d", sep, i);
      sep = ", ";
    }
  }
  printf("]\n");*/

  for(ADDRINT addr = start; addr < start + size; addr++) {
    memTaintMap[addr] = bitset_copy(s);
  }
  bitset_free(s);
}

void DisplayTagsForByteRange(ADDRINT start, size_t size, char *fmt, ...)
{
  bitset2 *tmp = bitset_init(NUMBER_OF_TAINT_MARKS);
  for(ADDRINT addr = start; addr < start + size; addr++) {
    map<ADDRINT, bitset2 *>::iterator iter = memTaintMap.find(addr);
    if(memTaintMap.end() != iter) {
      bitset_union(tmp, iter->second);  
    }
  }

  bitset2 *controlTaint = bitset_init(NUMBER_OF_TAINT_MARKS);
  for(map<ADDRINT, bitset2 *>::iterator iter = controlTaintMap.begin();
      iter != controlTaintMap.end(); iter++) {
    bitset_union(controlTaint, iter->second);
  }
  bitset_union(tmp, controlTaint);

  va_list ap;
  va_start(ap, fmt);
  
  vprintf(fmt, ap);
  va_end(ap);

  const char *sep = "";
  printf(" at a location %#x-%#x has taint marks: [", start, start + size - 1);
  printf("%s] with active control flow taint marks[", bitset_str(tmp));

  /*for(int i = 0; i < tmp->nbits; i++) {
    if(bitset_test_bit(tmp, i)) {
      printf("%s%d", sep, i);
      sep = ", ";
    }
  }
  printf("] with active control flow taint marks[");*/
  printf("%s]\n", bitset_str(controlTaint));

  /*sep = "";
  for(int i = 0; i < controlTaint->nbits; i++) {
    if(bitset_test_bit(controlTaint, i)) {
      printf("%s%d", sep, i);
      sep = ", ";
    }
  }
  printf("]\n");*/
 
  bitset_free(tmp);
  bitset_free(controlTaint);
}

void * nc_ptr_plus_int_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
							void *pointer, int offset,
							size_t size, const char *filename, int line)
{
	if(replacechecks){
		
	}else{
		return (char *) pointer + (int) size *offset; // -> performance evaluation	
	}
}

void * nc_ptr_minus_int_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
							void *pointer, int offset,
							size_t size, const char *filename, int line)
{
	if(replacechecks){
		
	}else{
		return (char *) pointer - (int) size *offset; // -> performance evaluation	
	}
}

void * nc_component_reference_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
							void *pointer, int offset,
							int size, const char *filename, int line)
{	
	if(replacechecks){
		
	}else{
		return pointer;
	}
}

int nc_array_reference_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
							void *pointer, int offset,
                       		size_t size, size_t array_size,
                       		const char *filename, int line){
	if(replacechecks){
	
	}else{
		return offset;
	}
}

void * nc_reference_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
					void *pointer, size_t size,
                  	const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer;
	}
}

bool nc_ptr_ne_ptr(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer1 != pointer2;
	}
}

bool nc_ptr_eq_ptr(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer1 == pointer2;
	}
}

int nc_ptr_diff_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2, size_t size,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return (int) ((char *) pointer1 - (char *) pointer2) / (int) size; // performance evaluation
	}
}

bool nc_ptr_true(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer, const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer != NULL; // performance eval
	}
}

bool nc_ptr_false(CONTEXT * ctxt, AFUNPTR origFuncptr,
					void *pointer, const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer == NULL; // performance eval
	}
}

bool nc_ptr_gt_ptr_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer1 > pointer2;  // performance evaluation
	}
}

bool nc_ptr_ge_ptr_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer1 >= pointer2;  // performance evaluation	
	}
}

bool nc_ptr_lt_ptr_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer1 < pointer2;  // performance evaluation
	}
}

bool nc_ptr_le_ptr_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
				void *pointer1, void *pointer2,
				const char *filename, int line){
	if(replacechecks){
	
	}else{
		return pointer1 <= pointer2;  // performance evaluation
	}
}

void * nc_ptr_preinc_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
		return (*(char **) ptr_to_ptr) += inc;  // performance eval
	}
}

void * nc_ptr_postinc_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
	
	}
}

void * nc_ptr_predec_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
	
	}
}

void * nc_ptr_postdec_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
	
	}
}

void * nc_ptr_preinc_ref_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc, size_t size,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
		return (*(char **) ptr_to_ptr) += inc; // performance eval
	}
}

void * nc_ptr_postinc_ref_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc, size_t size,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
	
	}
}

void * nc_ptr_predec_ref_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc, size_t size,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
	
	}
}

void * nc_ptr_postdec_ref_obj(CONTEXT * ctxt, AFUNPTR origFuncptr,
						void **ptr_to_ptr, int inc, size_t size,
						const char *filename, int line){
	if(replacechecks){
	
	}else{
	
	}
}


void ReplaceUserFunctions(IMG img, void *v)
{
  const char *func_names[] = {
    "DYTAN_set_trace",
    "DYTAN_display",
    "DYTAN_tag",
  };
  
  AFUNPTR functions[] = {
    AFUNPTR(SetTrace),
    AFUNPTR(DisplayTagsForByteRange),
    AFUNPTR(AssignTagToByteRange),
  };
  
  if(IMG_TYPE_SHAREDLIB == IMG_Type(img)) return;
  
  RTN rtn;
  
  for(int i = 0; i < 3; i++) {
    rtn = RTN_FindByName(img, func_names[i]);
    if(RTN_Valid(rtn)) {
      RTN_Replace(rtn, functions[i]);
      printf("**dytan** - Replaced %s in %s\n", func_names[i], IMG_Name(img).c_str());
    }
    else {
      printf("**dytan** - Could not replace %s in %s\n", func_names[i], IMG_Name(img).c_str());
    }
  }

	
}
