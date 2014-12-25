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

#include <syscall.h>
#include <sys/socket.h>
#include <linux/net.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "monitor_network.h"

/**
 * This class uses the Observer design pattern. It is a participant in two 
 * instances of the pattern. According to the design pattern,
 * this class is a subject for TaintNetworkSource, while it is an
 * observer for the SyscallMonitor class.
 */

void socketcallNetworkCallback(INT32 num, syscall_arguments args, void *v);
void readNetworkCallback(INT32 num, syscall_arguments args, void *v);

NetworkMonitor::NetworkMonitor()
{
  syscallMonitor = new SyscallMonitor();

  defaultAddressObserver = NULL;
}

NetworkMonitor::~NetworkMonitor()
{
  delete syscallMonitor;
  if(NULL != defaultAddressObserver) {
    delete defaultAddressObserver;
  }
}

void NetworkMonitor::activate()
{
  syscallMonitor->activate();

  //syscallMonitor->addObserver(SYS_socketcall, socketcallNetworkCallback, this);
  syscallMonitor->addObserver(SYS_read, readNetworkCallback, this);
}

/**
 * Registers the default callback function
 */
void NetworkMonitor::registerAddressDefault(NetworkMonitorCallback callback,
					 void *v)
{
  if(NULL != defaultAddressObserver) {
    delete defaultAddressObserver;
  }

  defaultAddressObserver = new pair<NetworkMonitorCallback, void *>(callback, v);
}

/**
 * Adds a callback for the address addr
 */
void NetworkMonitor::observeAddress(string addr,
				    NetworkMonitorCallback callback,
				    void *v)
{
  addressObservers[addr].
    push_back(pair<NetworkMonitorCallback, void *>(callback, v));
}

/**
 * Checks if the read system call has taken place for the 
 * network address that we are monitoring. If yes, then
 * notify the TaintNetworkSource object
 */
void NetworkMonitor::notifyForRead(syscall_arguments args)
{
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(struct sockaddr_in);
  int err;

  err = getpeername((int) args.arg0, (struct sockaddr*)&addr, &addr_len);

  if(-1 == err) return;

  string address = string(inet_ntoa(addr.sin_addr));
  
  if(addressObservers.find(address) == addressObservers.end()) {
    if(NULL != defaultAddressObserver) {
      defaultAddressObserver->first(address,
				    args,
				    defaultAddressObserver->second);
    }
    return;
  }

  vector<pair<NetworkMonitorCallback, void *> > activeObservers =
    addressObservers[address];

  for(vector<pair<NetworkMonitorCallback, void *> >::iterator iter = activeObservers.begin(); iter != activeObservers.end(); iter++) {
    (*iter).first(address, args, (*iter).second);
  }
  

}


/***********************************************************/

void socketcallNetworkCallback(INT32 num, syscall_arguments args, void *v)
{
  NetworkMonitor *networkMonitor = static_cast<NetworkMonitor *>(v);

  // ssize_t recv(int s, void *buf, size_t len, int flags);
  if(SYS_RECV == args.arg0) {
    
  }
}

// ssize_t read(int fd, void *buf, size_t count);
void readNetworkCallback(INT32 num, syscall_arguments args, void *v)
{
  NetworkMonitor *networkMonitor = static_cast<NetworkMonitor *>(v);

  if(-1 == args.ret) return;

  networkMonitor->notifyForRead(args);
  
}
