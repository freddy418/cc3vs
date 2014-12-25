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

#include "taint_source_path.h"

void pathSourceReadDefault(string, syscall_arguments, void *);
void pathSourceReadCallbackPerByte(string, syscall_arguments, void *);
void pathSourceReadCallbackPerRead(string, syscall_arguments, void *);

PathTaintSource::PathTaintSource()
{
  monitor = new PathMonitor();
  monitor->activate();

  monitor->registerDefault(pathSourceReadDefault, this);
}  

PathTaintSource::~PathTaintSource()
{
  delete monitor;
}

void PathTaintSource::addPathSource(string pathname, TaintGenerator *gen,
				    taint_range_t type)
{
  switch(type) {
  case PerByte: {
    monitor->observePath(pathname, pathSourceReadCallbackPerByte, gen);
    break;
  }
  case PerRead: {
    monitor->observePath(pathname, pathSourceReadCallbackPerRead, gen);
    break;
  }
  default:
    printf("Missing case\n");
    abort();
  }

}

/**************************************************************/

// ssize_t read(int fd, void *buf, size_t count);

void pathSourceReadCallbackPerByte(string pathname,
				   syscall_arguments args,
				   void *v)
{
  TaintGenerator *gen = static_cast<TaintGenerator *>(v);
  
  printf("Tainting per byte of %s with %d\n", pathname.c_str(),
	 gen->nextTaintMark());

  //taint each byte in buffer with different mark
}

void pathSourceReadCallbackPerRead(string pathname, 
				   syscall_arguments args,
				   void *v)
{
  TaintGenerator *gen = static_cast<TaintGenerator *>(v);
 
  printf("Tainting entire read of %s with %d\n", pathname.c_str(),
	 gen->nextTaintMark());
 
  // taint entire buffer with 1 taint mark
}

void pathSourceReadDefault(string pathname, 
			   syscall_arguments args,
			   void *v)
{
  printf("read from: %s\n", pathname.c_str());
  // clear taint marks for read area
}
