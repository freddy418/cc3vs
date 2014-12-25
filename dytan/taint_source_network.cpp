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

#include "taint_source_network.h"
#include <iostream>

void networkReadDefault(string, syscall_arguments, void *);
void networkReadCallbackPerByte(string, syscall_arguments, void *);
void networkReadCallbackPerRead(string, syscall_arguments, void *);

NetworkTaintSource::NetworkTaintSource()
{
	monitor = new NetworkMonitor();
	monitor->activate();
	monitor->registerAddressDefault(networkReadDefault, this);
}
	
NetworkTaintSource::~NetworkTaintSource()
{
	delete monitor;
}

void NetworkTaintSource::addNetworkSource(string networkAddr, TaintGenerator *gen,
	taint_range_t type)
{
	switch(type) {
		case PerByte: 
			monitor->observeAddress(networkAddr, networkReadCallbackPerByte, gen);
			break;
		case PerRead:
			monitor->observeAddress(networkAddr, networkReadCallbackPerRead, gen);
			break;
		default:
			cout << "Missing case!";
			abort();
	}
}

void networkReadCallbackPerByte(string networkAddr,
		syscall_arguments args,
		void *v) 
{
	TaintGenerator *gen = static_cast<TaintGenerator *>(v);
	
	printf("Tainting per byte of %s with %d\n", networkAddr.c_str(),
			gen->nextTaintMark());
	//taint each byte with a different mark
				
}

void networkReadCallbackPerRead(string networkAddr,
		syscall_arguments args,
		void *v) 
{
	TaintGenerator *gen = static_cast<TaintGenerator *>(v);
	
	printf("Tainting per read of %s with %d\n", networkAddr.c_str(),
			gen->nextTaintMark());
	//taint entire buffer with 1 mark
				
}

void networkReadDefault(string networkAddr,
		syscall_arguments args,
		void *v) 
{
	printf("Read from  %s\n ", networkAddr.c_str());
	//clear taint marks
				
}


		
	
