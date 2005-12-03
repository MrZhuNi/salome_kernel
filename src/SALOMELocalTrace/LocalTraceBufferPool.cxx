//  Copyright (C) 2004  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
// 
//  This library is free software; you can redistribute it and/or 
//  modify it under the terms of the GNU Lesser General Public 
//  License as published by the Free Software Foundation; either 
//  version 2.1 of the License. 
// 
//  This library is distributed in the hope that it will be useful, 
//  but WITHOUT ANY WARRANTY; without even the implied warranty of 
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
//  Lesser General Public License for more details. 
// 
//  You should have received a copy of the GNU Lesser General Public 
//  License along with this library; if not, write to the Free Software 
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
// 
//  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
//
//  Author : Paul RASCLE (EDF)
//  Module : KERNEL
//  $Header$
//
// Cf. C++ Users Journal, June 2004, Tracing Application Execution, Tomer Abramson
//

#include <iostream>
#include <limits.h>
#include <cassert>

#ifndef WNT
#include <dlfcn.h>
#else
#endif

//#define _DEVDEBUG_
#include "LocalTraceBufferPool.hxx"
#include "BaseTraceCollector.hxx"
#include "LocalTraceCollector.hxx"
#include "FileTraceCollector.hxx"
#include "BasicsGenericDestructor.hxx"
#include "utilities.h"

using namespace std;

// In case of truncated message, end of trace contains "...\n\0"

#define TRUNCATED_MESSAGE "...\n"
#define MAXMESS_LENGTH MAX_TRACE_LENGTH-5

// Class static attributes initialisation

LocalTraceBufferPool* LocalTraceBufferPool::_singleton = 0;
#ifndef WNT
pthread_mutex_t LocalTraceBufferPool::_singletonMutex;
#else
pthread_mutex_t LocalTraceBufferPool::_singletonMutex =
  PTHREAD_MUTEX_INITIALIZER;
#endif
BaseTraceCollector *LocalTraceBufferPool::_myThreadTrace = 0;

// ============================================================================
/*!
 *  Guarantees a unique object instance of the class (singleton thread safe).
 *  When the LocalTraceBufferPool instance is created, the trace collector is
 *  also created (singleton). Type of trace collector to create depends on 
 *  environment variable "SALOME_trace":
 *  - "local" implies standard err trace, LocalTraceCollector is launched.
 *  - "file" implies trace in /tmp/tracetest.log
 *  - "file:pathname" implies trace in file pathname
 *  - anything else like "other" : try to load dynamically a library named
 *    otherTraceCollector, and invoque C method instance() to start a singleton
 *    instance of the trace collector. Example: with_loggerTraceCollector, for
 *    CORBA Log.
 */
// ============================================================================

LocalTraceBufferPool* LocalTraceBufferPool::instance()
{
  if (_singleton == 0) // no need of lock when singleton already exists
    {
      int ret;
      ret = pthread_mutex_lock(&_singletonMutex); // acquire lock to be alone
      if (_singleton == 0)                     // another thread may have got
	{                                      // the lock after the first test
	  DEVTRACE("New buffer pool");
	  LocalTraceBufferPool* myInstance = new LocalTraceBufferPool(); 

  	  DESTRUCTOR_OF<LocalTraceBufferPool> *ptrDestroy =
  	    new DESTRUCTOR_OF<LocalTraceBufferPool> (*myInstance);
	  _singleton = myInstance;

	  // --- start a trace Collector

	  char* traceKind = getenv("SALOME_trace");
	  assert(traceKind);
	  //cerr<<"SALOME_trace="<<traceKind<<endl;

	  if (strcmp(traceKind,"local")==0)
	    {
	      _myThreadTrace = LocalTraceCollector::instance();
	    }
	  else if (strncmp(traceKind,"file",strlen("file"))==0)
	    {
	      char *fileName;
	      if (strlen(traceKind) > strlen("file"))
		fileName = &traceKind[strlen("file")+1];
	      else
		fileName = "/tmp/tracetest.log";
	      
	      _myThreadTrace = FileTraceCollector::instance(fileName);
	    }
	  else // --- try a dynamic library
	    {
	      void* handle;
#ifndef WNT
	      string impl_name = string ("lib") + traceKind 
		+ string("TraceCollector.so");
	      handle = dlopen( impl_name.c_str() , RTLD_LAZY ) ;
#else
	      string impl_name = string ("lib") + traceKind + string(".dll");
	      handle = dlopen( impl_name.c_str() , 0 ) ;
#endif
	      if ( handle )
		{
		  typedef BaseTraceCollector * (*FACTORY_FUNCTION) (void);
		  FACTORY_FUNCTION TraceCollectorFactory =
		    (FACTORY_FUNCTION) dlsym(handle, "SingletonInstance");
		  char *error ;
		  if ( (error = dlerror() ) != NULL)
		    {
		      cerr << "Can't resolve symbol: SingletonInstance" <<endl;
		      cerr << "dlerror: " << error << endl;
		      assert(error == NULL); // to give file and line
		      exit(1);               // in case assert is deactivated
		    }
		  _myThreadTrace = (TraceCollectorFactory) ();
		}
	      else
		{
		  cerr << "library: " << impl_name << " not found !" << endl;
		  assert(handle); // to give file and line
		  exit(1);        // in case assert is deactivated
		}	      
	    }
	  DEVTRACE("New buffer pool: end");
	}
      ret = pthread_mutex_unlock(&_singletonMutex); // release lock
    }
  return _singleton;
}

// ============================================================================
/*!
 *  Called by trace producers within their threads. The trace message is copied
 *  in specific buffer from a circular pool of buffers.
 *  Waits until there is a free buffer in the pool, gets the first available
 *  buffer, fills it with the message.
 *  Messages are printed in a separate thread (see retrieve method)
 */
// ============================================================================

int LocalTraceBufferPool::insert(int traceType, const char* msg)
{

  // get immediately a message number to control sequence (mutex protected)

  unsigned long myMessageNumber = lockedIncrement(_position);

  // wait until there is a free buffer in the pool

  int ret = sem_wait(&_freeBufferSemaphore);

  // get the next free buffer available (mutex protected) 

  unsigned long myInsertPos = lockedIncrement(_insertPos);

  // fill the buffer with message, thread id and type (normal or abort)

  strncpy(_myBuffer[myInsertPos%TRACE_BUFFER_SIZE].trace,
	  msg,
	  MAXMESS_LENGTH); // last chars always "...\n\0" if msg too long
  _myBuffer[myInsertPos%TRACE_BUFFER_SIZE].threadId =pthread_self();//thread id
  _myBuffer[myInsertPos%TRACE_BUFFER_SIZE].traceType = traceType;
  _myBuffer[myInsertPos%TRACE_BUFFER_SIZE].position = myMessageNumber;


  // increment the full buffer semaphore
  // (if previously 0, awake thread in charge of trace)

  ret = sem_post(&_fullBufferSemaphore);

  // returns the number of free buffers

  sem_getvalue(&_freeBufferSemaphore, &ret);
  return ret;  
}

// ============================================================================
/*!
 *  Called by the thread in charge of printing trace messages.
 *  Waits until there is a buffer with a message to print.
 *  Gets the first buffer to print, copies it int the provided buffer
 */
// ============================================================================

int LocalTraceBufferPool::retrieve(LocalTrace_TraceInfo& aTrace)
{

  // wait until there is a buffer in the pool, with a message to print

  int ret = sem_wait(&_fullBufferSemaphore);

  // get the next buffer to print

  unsigned long myRetrievePos = lockedIncrement(_retrievePos);

  // copy the buffer from the pool to the provided buffer

  memcpy((void*)&aTrace,
	 (void*)&_myBuffer[myRetrievePos%TRACE_BUFFER_SIZE],
	 sizeof(aTrace));

  // increment the free buffer semaphore
  // (if previously 0, awake one of the threads waiting to put a trace, if any)
  // there is no way to preserve the order of waiting threads if several
  // threads are waiting to put a trace: the waken up thread is not
  // necessarily the first thread to wait.

  ret = sem_post(&_freeBufferSemaphore);

  // returns the number of full buffers

  sem_getvalue(&_fullBufferSemaphore, &ret);
  return ret;
}

// ============================================================================
/*!
 *  Gives the number of buffers to print.
 *  Usage : when the thread in charge of messages print id to be stopped,
 *  check if there is still something to print, before stop.
 *  There is no need of mutex here, provided there is only one thread to
 *  retrieve and print the buffers.
 */
// ============================================================================

unsigned long LocalTraceBufferPool::toCollect()
{
  return _insertPos - _retrievePos;
}

// ============================================================================
/*!
 * Constructor : initialize pool of buffers, semaphores and mutex.
 */
// ============================================================================

LocalTraceBufferPool::LocalTraceBufferPool()
{
  //cerr << "LocalTraceBufferPool::LocalTraceBufferPool()" << endl;

  _insertPos   = ULONG_MAX;  // first increment will give 0
  _retrievePos = ULONG_MAX;
  _position=0;               // first message will have number = 1

  memset(_myBuffer, 0, sizeof(_myBuffer)); // to guarantee end of strings = 0
  for (int i=0; i<TRACE_BUFFER_SIZE; i++)
    strcpy(&(_myBuffer[i].trace[MAXMESS_LENGTH]),TRUNCATED_MESSAGE);
  int ret;
  ret=sem_init(&_freeBufferSemaphore, 0, TRACE_BUFFER_SIZE); // all buffer free
  if (ret!=0) IMMEDIATE_ABORT(ret);
  ret=sem_init(&_fullBufferSemaphore, 0, 0);                 // 0 buffer full
  if (ret!=0) IMMEDIATE_ABORT(ret);
  ret=pthread_mutex_init(&_incrementMutex,NULL); // default = fast mutex
  if (ret!=0) IMMEDIATE_ABORT(ret);

  //cerr << "LocalTraceBufferPool::LocalTraceBufferPool()-end" << endl;
}

// ============================================================================
/*!
 * Destructor : release memory associated with semaphores and mutex
 */
// ============================================================================

LocalTraceBufferPool::~LocalTraceBufferPool()
{
  int ret = pthread_mutex_lock(&_singletonMutex); // acquire lock to be alone
  if (_singleton)
    {
      DEVTRACE("LocalTraceBufferPool::~LocalTraceBufferPool()");
      delete (_myThreadTrace);
      _myThreadTrace = 0;
      int ret;
      ret=sem_destroy(&_freeBufferSemaphore);
      ret=sem_destroy(&_fullBufferSemaphore);
      ret=pthread_mutex_destroy(&_incrementMutex);
      DEVTRACE("LocalTraceBufferPool::~LocalTraceBufferPool()-end");
      _singleton = 0;
    }
  ret = pthread_mutex_unlock(&_singletonMutex); // release lock
}

// ============================================================================
/*!
 * pool counters are incremented under a mutex protection
 */
// ============================================================================

unsigned long LocalTraceBufferPool::lockedIncrement(unsigned long& pos)
{
  int ret;
  ret = pthread_mutex_lock(&_incrementMutex);   // lock access to counters
  unsigned long mypos = ++pos;
  ret = pthread_mutex_unlock(&_incrementMutex); // release lock
  return mypos;
}

