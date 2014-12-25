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
#include "monitor_path.h"

void openCallback(INT32, syscall_arguments, void *);
void closeCallback(INT32, syscall_arguments, void *);
void readCallback(INT32, syscall_arguments, void *);

PathMonitor::PathMonitor()
{
  syscallMonitor = new SyscallMonitor();
  defaultObserver = NULL;
}

PathMonitor::~PathMonitor()
{
  delete syscallMonitor;
  if(NULL != defaultObserver) {
    delete defaultObserver;
  }
}

void PathMonitor::activate()
{
  syscallMonitor->activate();
  syscallMonitor->addObserver(SYS_open, openCallback, this);
  syscallMonitor->addObserver(SYS_close, closeCallback, this);
  syscallMonitor->addObserver(SYS_read, readCallback, this);
}

void PathMonitor::observePath(string pathname,
			 PathMonitorCallback callback,
			 void *v)
{
  observers[pathname].
    push_back(pair<PathMonitorCallback, void *>(callback, v));
}

void PathMonitor::registerDefault(PathMonitorCallback callback, void *v)
{
  if(NULL != defaultObserver) {
    delete defaultObserver;
  }
  defaultObserver = new pair<PathMonitorCallback, void *>(callback, v);
}

void PathMonitor::addActiveFileDescriptor(int fd, string pathname)
{
  activeFileDescriptors[fd] = pathname;
}

void PathMonitor::removeActiveFileDescriptor(int fd)
{
  activeFileDescriptors.erase(fd);
}

/**
 * called by the listener for read system call. checks if the
 * read has happened on the file that we are interested in (defaultObserver).
 * if that is the case, it calls the callback for PathTaint 
 */
void PathMonitor::notifyForRead(syscall_arguments args)
{
  int fd = (int) args.arg0;

  if(activeFileDescriptors.find(fd) == activeFileDescriptors.end()) {
    return;
  }

  string pathname = activeFileDescriptors[fd];

  if(observers.find(pathname) == observers.end()) {
    if(NULL != defaultObserver) {
      defaultObserver->first(pathname, args, defaultObserver->second);
    }
    return;
  }

  vector<pair<PathMonitorCallback, void *> > activeObservers =
    observers[pathname];

  for(vector<pair<PathMonitorCallback, void *> >::iterator iter = activeObservers.begin(); iter != activeObservers.end(); iter++) {
    (*iter).first(pathname, args, (*iter).second);
  }
  
}

/*********************************************/

// int open(const char *pathname, int flags, mode_t mode);
// friend function that listens for the open system call. This
// is called by the system call monitor. Adds the file descriptor
// of the file that is opened
void openCallback(INT32 syscall_num,
		  syscall_arguments args,
		  void *v)
{
  PathMonitor *pathMonitor = static_cast<PathMonitor *>(v);
 
  if(args.ret == -1) return;

  pathMonitor->addActiveFileDescriptor((int) args.ret,
					string((const char *)args.arg0));
}

// int close(int fd);
// friend function that listens for the close call. called by
// system call monitor. removes file descriptor of the file
// being close
void closeCallback(INT32 syscall_num,
		   syscall_arguments args,
		   void *v)
{
  PathMonitor *pathMonitor = static_cast<PathMonitor *>(v);
 
  if(args.ret == -1) return;

  pathMonitor->removeActiveFileDescriptor((int) args.arg0);
}

// ssize_t read(int fd, void *buf, size_t count);
// friend function that listens for the read call. called by
// system call monitor. 
void readCallback(INT32 syscall_num,
		  syscall_arguments args,
		  void *v)
{
  PathMonitor *pathMonitor = static_cast<PathMonitor *>(v);

  if(args.ret == -1) return;
  
  pathMonitor->notifyForRead(args);

}
