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
//
//
//  File   : LocalTraceCollector.cxx
//  Author : Paul RASCLE (EDF)
//  Module : KERNEL
//  $Header$

#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

using namespace std;

#include "LocalTraceCollector.hxx"

// ============================================================================
/*!
 *  This class is for use without CORBA, outside SALOME.
 *  SALOME uses SALOMETraceCollector, to allow trace collection via CORBA.
 *
 *  guarantees a unique object instance of the class (singleton thread safe)
 *  a separate thread for loop to print traces is launched.
 *  \param typeTrace 0=standard out, 1=file(/tmp/tracetest.log)
 *  If typeTrace=0, checks environment for "SALOME_trace". Test values in
 *  the following order:
 *  - "local"  standard out
 *  - anything else is kept as a file name
 */
// ============================================================================

BaseTraceCollector* LocalTraceCollector::instance()
{
  if (_singleton == 0) // no need of lock when singleton already exists
    {
      int ret;
      ret = pthread_mutex_lock(&_singletonMutex); // acquire lock to be alone
      if (_singleton == 0)                     // another thread may have got
	{                                      // the lock after the first test
	  _singleton = new LocalTraceCollector();

	  pthread_t traceThread;
	  int bid;
	  int re2 = pthread_create(&traceThread, NULL,
				   LocalTraceCollector::run, (void *)bid);
	}
      ret = pthread_mutex_unlock(&_singletonMutex); // release lock
    }
  return _singleton;
}

// ============================================================================
/*!
 *  In a separate thread, loop to print traces.
 *  Mutex garantees intialisation on instance method is done and only one run
 *  allowed (double check ...)
 *  Loop until there is no more buffer to print,
 *  and no ask for end from destructor.
 *  Get a buffer. If type = ABORT then exit application with message.
 */
// ============================================================================

void* LocalTraceCollector::run(void *bid)
{
  int isOKtoRun = 0;
  int ret = pthread_mutex_lock(&_singletonMutex); // acquire lock to be alone

  if (! _threadId)  // only one run
    {
      isOKtoRun = 1;
      if(_threadId == 0) {
	_threadId = new pthread_t;
      }
      *_threadId = pthread_self();
    }
  else cout << "----- Comment est-ce possible de passer la ? -------" <<endl;

  ret = pthread_mutex_unlock(&_singletonMutex); // release lock

  if (isOKtoRun)
    { 
      if(_threadId == 0) 
	{
	  _threadId = new pthread_t;
	}

      *_threadId = pthread_self();
      LocalTraceBufferPool* myTraceBuffer = LocalTraceBufferPool::instance();
      LocalTrace_TraceInfo myTrace;

      // --- Loop until there is no more buffer to print,
      //     and no ask for end from destructor.

      while ((!_threadToClose) || myTraceBuffer->toCollect() )
	{
	  int fullBuf = myTraceBuffer->retrieve(myTrace);
	  if (myTrace.traceType == ABORT_MESS)
	    {
	      cout << flush ;
#ifndef WNT
	      cerr << "INTERRUPTION from thread " << myTrace.threadId
		   << " : " <<  myTrace.trace;
#else
	      cerr << "INTERRUPTION from thread " << (void*)(&myTrace.threadId)
		   << " : " <<  myTrace.trace;
#endif
	      cerr << flush ; 
	      exit(1);     
	    }
	  else
	    {
	      cout << flush ;
#ifndef WNT
	      cerr << "th. " << myTrace.threadId << " " << myTrace.trace;
#else
	      cerr << "th. " << (void*)(&myTrace.threadId)
		   << " " << myTrace.trace;
#endif
	      cerr << flush ; 
	    }
	}
    }
  pthread_exit(NULL);
  return NULL;
}

// ============================================================================
/*!
 *  Destructor: wait until printing thread ends (LocalTraceCollector::run)
 */
// ============================================================================

LocalTraceCollector:: ~LocalTraceCollector()
{
  cerr << "LocalTraceCollector:: ~LocalTraceCollector()" << endl << flush;
}

// ============================================================================
/*!
 * Constructor: no need of LocalTraceBufferPool object initialization here,
 * thread safe singleton used in LocalTraceBufferPool::instance()
 */
// ============================================================================

LocalTraceCollector::LocalTraceCollector()
{
  _threadId=0;
}


