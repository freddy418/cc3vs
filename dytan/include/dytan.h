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

#ifndef _DYTAN_H
#define _DYTAN_H

#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <typeinfo>
#include <map>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include "pin.H"
#include "instlib.H"

#include <signal.h>
#include <linux/net.h>
#include <time.h>
#include <sys/time.h>
#include <syscall.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include "bitset.h"

#define TRACE 1
#define replacechecks 0

// using namespace XED;  //obsolete

extern FILE *log;
extern FILE *taintAssignmentLog;

/* global storage to hold taint marks */
extern bitset2 *dest;
extern bitset2 *src;
extern bitset2 *eax;
extern bitset2 *edx;
extern bitset2 *base;
extern bitset2 *idx;
extern bitset2 *eflags;
extern bitset2 *cnt;

/* finer grained control for when logging should happen */
extern bool tracing;

/* the maximum allowable number of taint mark */
/* currently the assignment of taint marks will wrap around */
#define NUMBER_OF_TAINT_MARKS 512
extern int currentTaintMark;

/* controls if detailed logging is used */
#define TRACE 1

/*
   controls if registers are considered to propagate taint if they're
   used to access memory.  For example: load [%eax], %eax will propagate
   taint if IMPLICIT is defined and it won't if IMPLICIT is not defined
   */
#define IMPLICIT 0

/*
   map that stores taint marks for registers
   */
extern map<REG, bitset2 *> regTaintMap;

/*
   map that stores taint marks for memory address, currently this is
   per byte
   */
extern map<ADDRINT, bitset2 *> memTaintMap;

/*
 * map that stores taint marks active due to control flow
 */
extern map<ADDRINT, bitset2 *>controlTaintMap;

typedef void (*InstrumentFunction)(INS ins, void *v);

extern InstrumentFunction instrument_functions[XED_ICLASS_LAST];

extern void SetNewTaintForMemory(ADDRINT, ADDRINT, int taint_mark = -1);

/*
 * Some functions for converting string to a type like int
 */
class BadConversion : public std::runtime_error {
    public:
        BadConversion(const std::string& s)
            : std::runtime_error(s)
        { }
};
 
    template<typename T>
inline void convert(const std::string& s, T& x,
        bool failIfLeftoverChars = true)
{
    std::istringstream i(s);
    char c;
    if (!(i >> x) || (failIfLeftoverChars && i.get(c)))
        throw BadConversion(s);
}

    template<typename T>
inline T convertTo(const std::string& s,
        bool failIfLeftoverChars = true)
{
    T x;
    convert(s, x, failIfLeftoverChars);
    return x;
}


#endif
