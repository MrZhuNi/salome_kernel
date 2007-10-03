//  SALOME Container : implementation of container and engine for Kernel
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : Container_i.cxx
//  Author : Paul RASCLE, EDF - MARC TAJCHMAN, CEA 
//  Module : SALOME
//  $Header$

//#define private public
#include <string.h>
#include <stdio.h>
#include <time.h>
#ifndef WNT
#include <sys/time.h>
#include <dlfcn.h>
#include <unistd.h>
#else
#include <signal.h>
#include <process.h>
int SIGUSR1 = 1000;
#endif

#include "utilities.h"
#include <SALOMEconfig.h>
//#ifndef WNT
#include CORBA_SERVER_HEADER(SALOME_Component)
#include CORBA_SERVER_HEADER(SALOME_Exception)
//#else
//#include <SALOME_Component.hh>
//#endif
#include <pthread.h>  // must be before Python.h !
#include "SALOME_Container_i.hxx"
#include "SALOME_Component_i.hxx"
#include "SALOME_FileRef_i.hxx"
#include "SALOME_FileTransfer_i.hxx"
#include "Salome_file_i.hxx"
#include "SALOME_NamingService.hxx"
#include "OpUtil.hxx"

#include <Python.h>
#include "Container_init_python.hxx"

using namespace std;

bool _Sleeping = false ;

// // Needed by multi-threaded Python --- Supervision
int _ArgC ;
char ** _ArgV ;


// Containers with name FactoryServer are started via rsh in LifeCycleCORBA
// Other Containers are started via start_impl of FactoryServer

extern "C" {void ActSigIntHandler() ; }
#ifndef WNT
  extern "C" {void SigIntHandler(int, siginfo_t *, void *) ; }
#else
  extern "C" {void SigIntHandler( int ) ; }
#endif


map<std::string, int> Engines_Container_i::_cntInstances_map;
map<std::string, void *> Engines_Container_i::_library_map;
map<std::string, void *> Engines_Container_i::_toRemove_map;
omni_mutex Engines_Container_i::_numInstanceMutex ;

//=============================================================================
/*! 
 *  Default constructor, not for use
 */
//=============================================================================

Engines_Container_i::Engines_Container_i () :
  _numInstance(0)
{
}

//=============================================================================
/*! 
 *  Construtor to use
 */
//=============================================================================

Engines_Container_i::Engines_Container_i (CORBA::ORB_ptr orb, 
                                          PortableServer::POA_var poa,
                                          char *containerName ,
                                          int argc , char* argv[],
                                          bool activAndRegist,
                                          bool isServantAloneInProcess
                                          ) :
  _numInstance(0),_isServantAloneInProcess(isServantAloneInProcess)
{
  _pid = (long)getpid();

  if(activAndRegist)
    ActSigIntHandler() ;

  _argc = argc ;
  _argv = argv ;

  string hostname = GetHostname();
#ifndef WNT
  MESSAGE(hostname << " " << getpid() << 
         " Engines_Container_i starting argc " <<
   _argc << " Thread " << pthread_self() ) ;
#else
  MESSAGE(hostname << " " << _getpid() << 
         " Engines_Container_i starting argc " << _argc<< " Thread " << pthread_self().p ) ;
#endif

  int i = 0 ;
  while ( _argv[ i ] )
    {
      MESSAGE("           argv" << i << " " << _argv[ i ]) ;
      i++ ;
    }

  if ( argc < 2 )
    {
      INFOS("SALOME_Container usage : SALOME_Container ServerName");
      ASSERT(0) ;
    }
  SCRUTE(argv[1]);
  _isSupervContainer = false;
  if (strcmp(argv[1],"SuperVisionContainer") == 0) _isSupervContainer = true;

  if (_isSupervContainer)
    {
      _ArgC = argc ;
      _ArgV = argv ;
    }

  _orb = CORBA::ORB::_duplicate(orb) ;
  _poa = PortableServer::POA::_duplicate(poa) ;
  
  // Pour les containers paralleles: il ne faut pas enregistrer et activer
  // le container generique, mais le container specialise

  if(activAndRegist)
    {
      _id = _poa->activate_object(this);
      _NS = new SALOME_NamingService();
      _NS->init_orb( CORBA::ORB::_duplicate(_orb) ) ;
      CORBA::Object_var obj=_poa->id_to_reference(*_id);
      Engines::Container_var pCont 
        = Engines::Container::_narrow(obj);

      _containerName = _NS->BuildContainerNameForNS(containerName,
                                                    hostname.c_str());
      SCRUTE(_containerName);
      _NS->Register(pCont, _containerName.c_str());
      MESSAGE("Engines_Container_i::Engines_Container_i : Container name "
              << _containerName);

      // Python: 
      // import SALOME_Container
      // pycont = SALOME_Container.SALOME_Container_i(containerIORStr)
    
      CORBA::String_var sior =  _orb->object_to_string(pCont);
      string myCommand="pyCont = SALOME_Container.SALOME_Container_i('";
      myCommand += _containerName + "','";
      myCommand += sior;
      myCommand += "')\n";
      SCRUTE(myCommand);

      if (!_isSupervContainer)
        {
#ifdef WNT

          PyEval_AcquireLock();
          PyThreadState *myTstate = PyThreadState_New(KERNEL_PYTHON::_interp);
          PyThreadState *myoldTstate = PyThreadState_Swap(myTstate);
#else
          Py_ACQUIRE_NEW_THREAD;
#endif

#ifdef WNT
          // mpv: this is temporary solution: there is a unregular crash if not
          //Sleep(2000);
          //
    // first element is the path to Registry.dll, but it's wrong
          PyRun_SimpleString("import sys\n");
          PyRun_SimpleString("sys.path = sys.path[1:]\n");
#endif
          PyRun_SimpleString("import SALOME_Container\n");
          PyRun_SimpleString((char*)myCommand.c_str());
          Py_RELEASE_NEW_THREAD;
        }

      fileTransfer_i* aFileTransfer = new fileTransfer_i();
      _fileTransfer = Engines::fileTransfer::_narrow(aFileTransfer->_this());
    }
}

//=============================================================================
/*! 
 *  Destructor
 */
//=============================================================================

Engines_Container_i::~Engines_Container_i()
{
  MESSAGE("Container_i::~Container_i()");
  delete _id;
}

//=============================================================================
/*! 
 *  CORBA attribute: Container name (see constructor)
 */
//=============================================================================

char* Engines_Container_i::name()
{
   return CORBA::string_dup(_containerName.c_str()) ;
}

//=============================================================================
/*! 
 *  CORBA method: Get the hostName of the Container (without domain extensions)
 */
//=============================================================================

char* Engines_Container_i::getHostName()
{
  string s = GetHostname();
  //  MESSAGE("Engines_Container_i::getHostName " << s);
  return CORBA::string_dup(s.c_str()) ;
}

//=============================================================================
/*! 
 *  CORBA method: Get the PID (process identification) of the Container
 */
//=============================================================================

CORBA::Long Engines_Container_i::getPID()
{
  return (CORBA::Long)getpid();
}

//=============================================================================
/*! 
 *  CORBA method: check if servant is still alive
 */
//=============================================================================

void Engines_Container_i::ping()
{
  MESSAGE("Engines_Container_i::ping() pid "<< getpid());
}

//=============================================================================
/*! 
 *  CORBA method, oneway: Server shutdown. 
 *  - Container name removed from naming service,
 *  - servant deactivation,
 *  - orb shutdown if no other servants in the process 
 */
//=============================================================================

void Engines_Container_i::Shutdown()
{
  MESSAGE("Engines_Container_i::Shutdown()");

  /* For each component contained in this container
   * tell it to self-destroy
   */
  std::map<std::string, Engines::Component_var>::iterator itm;
  for (itm = _listInstances_map.begin(); itm != _listInstances_map.end(); itm++)
    itm->second->destroy();

  _NS->Destroy_FullDirectory(_containerName.c_str());
  _NS->Destroy_Name(_containerName.c_str());
  //_remove_ref();
  //_poa->deactivate_object(*_id);
  if(_isServantAloneInProcess)
    {
      MESSAGE("Effective Shutdown of container Begins...");
      LocalTraceBufferPool* bp1 = LocalTraceBufferPool::instance();
      bp1->deleteInstance(bp1);
      if(!CORBA::is_nil(_orb))
	_orb->shutdown(0);
    }
}


//=============================================================================
/*! 
 *  CORBA method: load a new component class (Python or C++ implementation)
 *  \param componentName like COMPONENT
 *                          try to make a Python import of COMPONENT,
 *                          then a lib open of libCOMPONENTEngine.so
 *  \return true if dlopen successfull or already done, false otherwise
 */
//=============================================================================

bool
Engines_Container_i::load_component_Library(const char* componentName)
{

  string aCompName = componentName;

  // --- try dlopen C++ component

#ifndef WNT
  string impl_name = string ("lib") + aCompName + string("Engine.so");
#else
  string impl_name = aCompName + string("Engine.dll");
#endif
  SCRUTE(impl_name);
  
  _numInstanceMutex.lock(); // lock to be alone 
  // (see decInstanceCnt, finalize_removal))
  if (_toRemove_map[impl_name]) _toRemove_map.erase(impl_name);
  if (_library_map[impl_name])
    {
      MESSAGE("Library " << impl_name << " already loaded");
      _numInstanceMutex.unlock();
      return true;
    }
  
#ifndef WNT
  void* handle;
  handle = dlopen( impl_name.c_str() , RTLD_LAZY ) ;
#else
  HINSTANCE handle;
  handle = LoadLibrary( impl_name.c_str() );
#endif

  if ( handle )
  {
      _library_map[impl_name] = handle;
      _numInstanceMutex.unlock();
      return true;
  }
  else
  {
      INFOS( "Can't load shared library: " << impl_name );
#ifndef WNT
      INFOS("error dlopen: " << dlerror());
#endif
  }
  _numInstanceMutex.unlock();

  // --- try import Python component

  INFOS("try import Python component "<<componentName);
  if (_isSupervContainer)
    {
      INFOS("Supervision Container does not support Python Component Engines");
      return false;
    }
  if (_library_map[aCompName])
    {
      return true; // Python Component, already imported
    }
  else
    {
      Py_ACQUIRE_NEW_THREAD;
      PyObject *mainmod = PyImport_AddModule("__main__");
      PyObject *globals = PyModule_GetDict(mainmod);
      PyObject *pyCont = PyDict_GetItemString(globals, "pyCont");
      PyObject *result = PyObject_CallMethod(pyCont,
                                             "import_component",
                                             "s",componentName);
      int ret= PyInt_AsLong(result);
      SCRUTE(ret);
      Py_RELEASE_NEW_THREAD;
  
      if (ret) // import possible: Python component
        {
          _numInstanceMutex.lock() ; // lock to be alone (stl container write)
          _library_map[aCompName] = (void *)pyCont; // any non O value OK
          _numInstanceMutex.unlock() ;
          MESSAGE("import Python: "<<aCompName<<" OK");
          return true;
        }
    }
  return false;
}

//=============================================================================
/*! 
 *  CORBA method: Creates a new servant instance of a component.
 *  The servant registers itself to naming service and Registry.
 *  \param genericRegisterName  Name of the component instance to register
 *                         in Registry & Name Service (without _inst_n suffix)
 *  \param studyId         0 for multiStudy instance, 
 *                         study Id (>0) otherwise
 *  \return a loaded component
 */
//=============================================================================

Engines::Component_ptr
Engines_Container_i::create_component_instance(const char*genericRegisterName,
                                               CORBA::Long studyId)
{
  if (studyId < 0)
    {
      INFOS("studyId must be > 0 for mono study instance, =0 for multiStudy");
      return Engines::Component::_nil() ;
    }

  Engines::Component_var iobject = Engines::Component::_nil() ;

  string aCompName = genericRegisterName;
  if (_library_map[aCompName]) // Python component
    {
      if (_isSupervContainer)
        {
          INFOS("Supervision Container does not support Python Component Engines");
          return Engines::Component::_nil();
        }
      _numInstanceMutex.lock() ; // lock on the instance number
      _numInstance++ ;
      int numInstance = _numInstance ;
      _numInstanceMutex.unlock() ;

      char aNumI[12];
      sprintf( aNumI , "%d" , numInstance ) ;
      string instanceName = aCompName + "_inst_" + aNumI ;
      string component_registerName =
        _containerName + "/" + instanceName;

      Py_ACQUIRE_NEW_THREAD;
      PyObject *mainmod = PyImport_AddModule("__main__");
      PyObject *globals = PyModule_GetDict(mainmod);
      PyObject *pyCont = PyDict_GetItemString(globals, "pyCont");
      PyObject *result = PyObject_CallMethod(pyCont,
                                             "create_component_instance",
                                             "ssl",
                                             aCompName.c_str(),
                                             instanceName.c_str(),
                                             studyId);
      string iors = PyString_AsString(result);
      SCRUTE(iors);
      Py_RELEASE_NEW_THREAD;
  
      if( iors!="" )
      {
        CORBA::Object_var obj = _orb->string_to_object(iors.c_str());
        iobject = Engines::Component::_narrow( obj ) ;
      }
      return iobject._retn();
    }
  
  //--- try C++

#ifndef WNT
  string impl_name = string ("lib") + genericRegisterName +string("Engine.so");
#else
  string impl_name = genericRegisterName +string("Engine.dll");
#endif
  void* handle = _library_map[impl_name];
  if ( !handle )
    {
      INFOS("shared library " << impl_name <<"must be loaded before instance");
      return Engines::Component::_nil() ;
    }
  else
    {
      iobject = createInstance(genericRegisterName,
                               handle,
                               studyId);
      return iobject._retn();
    }
}

//=============================================================================
/*! 
 *  CORBA method: Finds a servant instance of a component
 *  \param registeredName  Name of the component in Registry or Name Service,
 *                         without instance suffix number
 *  \param studyId         0 if instance is not associated to a study, 
 *                         >0 otherwise (== study id)
 *  \return the first instance found with same studyId
 */
//=============================================================================

Engines::Component_ptr
Engines_Container_i::find_component_instance( const char* registeredName,
                                              CORBA::Long studyId)
{
  Engines::Component_var anEngine = Engines::Component::_nil();
  map<string,Engines::Component_var>::iterator itm =_listInstances_map.begin();
  while (itm != _listInstances_map.end())
    {
      string instance = (*itm).first;
      SCRUTE(instance);
      if (instance.find(registeredName) == 0)
        {
          anEngine = (*itm).second;
          if (studyId == anEngine->getStudyId())
            {
              return anEngine._retn();
            }
        }
      itm++;
    }
  return anEngine._retn();  
}

//=============================================================================
/*! 
 *  CORBA method: find or create an instance of the component (servant),
 *  load a new component class (dynamic library) if required,
 *  ---- FOR COMPATIBILITY WITH 2.2 ---- 
 *  ---- USE ONLY FOR MULTISTUDY INSTANCES ! --------
 *  The servant registers itself to naming service and Registry.
 *  \param genericRegisterName  Name of the component to register
 *                              in Registry & Name Service
 *  \param componentName       Name of the constructed library of the component
 *  \return a loaded component
 */
//=============================================================================

Engines::Component_ptr
Engines_Container_i::load_impl( const char* genericRegisterName,
                                const char* componentName )
{
  string impl_name = string ("lib") + genericRegisterName +string("Engine.so");
  Engines::Component_var iobject = Engines::Component::_nil() ;
  if (load_component_Library(genericRegisterName))
    iobject = find_or_create_instance(genericRegisterName, impl_name);
  return iobject._retn();
}
    

//=============================================================================
/*! 
 *  CORBA method: Stops the component servant, and deletes all related objects
 *  \param component_i     Component to be removed
 */
//=============================================================================

void Engines_Container_i::remove_impl(Engines::Component_ptr component_i)
{
  ASSERT(! CORBA::is_nil(component_i));
  string instanceName = component_i->instanceName() ;
  MESSAGE("unload component " << instanceName);
  _numInstanceMutex.lock() ; // lock to be alone (stl container write)
  _listInstances_map.erase(instanceName);
  _numInstanceMutex.unlock() ;
  component_i->destroy() ;
  _NS->Destroy_Name(instanceName.c_str());
}

//=============================================================================
/*! 
 *  CORBA method: Discharges unused libraries from the container.
 */
//=============================================================================

void Engines_Container_i::finalize_removal()
{
  MESSAGE("finalize unload : dlclose");
  _numInstanceMutex.lock(); // lock to be alone
                            // (see decInstanceCnt, load_component_Library)
  map<string, void *>::iterator ith;
  for (ith = _toRemove_map.begin(); ith != _toRemove_map.end(); ith++)
    {
      void *handle = (*ith).second;
      string impl_name= (*ith).first;
      if (handle)
        {
          SCRUTE(handle);
          SCRUTE(impl_name);
//        dlclose(handle);                // SALOME unstable after ...
//        _library_map.erase(impl_name);
        }
    }
  _toRemove_map.clear();
  _numInstanceMutex.unlock();
}

//=============================================================================
/*! 
 *  CORBA method: Kill the container process with exit(0).
 *  To remove :  never returns !
 */
//=============================================================================

bool Engines_Container_i::Kill_impl()
{
  MESSAGE("Engines_Container_i::Kill() pid "<< getpid() << " containerName "
          << _containerName.c_str() << " machineName "
          << GetHostname().c_str());
  INFOS("===============================================================");
  INFOS("= REMOVE calls to Kill_impl in C++ container                  =");
  INFOS("===============================================================");
  //exit( 0 ) ;
  ASSERT(0);
  return false;
}

//=============================================================================
/*! 
 *  CORBA method: get or create a fileRef object associated to a local file
 *  (a file on the computer on which runs the container server), which stores
 *  a list of (machine, localFileName) corresponding to copies already done.
 * 
 *  \param  origFileName absolute path for a local file to copy on other
 *          computers
 *  \return a fileRef object associated to the file.
 */
//=============================================================================

Engines::fileRef_ptr
Engines_Container_i::createFileRef(const char* origFileName)
{
  string origName(origFileName);
  Engines::fileRef_var theFileRef = Engines::fileRef::_nil();

  if (origName[0] != '/')
    {
      INFOS("path of file to copy must be an absolute path begining with '/'");
      return Engines::fileRef::_nil();
    }

  if (CORBA::is_nil(_fileRef_map[origName]))
    {
      CORBA::Object_var obj=_poa->id_to_reference(*_id);
      Engines::Container_var pCont = Engines::Container::_narrow(obj);
      fileRef_i* aFileRef = new fileRef_i(pCont, origFileName);
      theFileRef = Engines::fileRef::_narrow(aFileRef->_this());
      _numInstanceMutex.lock() ; // lock to be alone (stl container write)
      _fileRef_map[origName] = theFileRef;
      _numInstanceMutex.unlock() ;
    }
  
  theFileRef =  Engines::fileRef::_duplicate(_fileRef_map[origName]);
  ASSERT(! CORBA::is_nil(theFileRef));
  return theFileRef._retn();
}

//=============================================================================
/*! 
 *  CORBA method:
 *  \return a reference to the fileTransfer object
 */
//=============================================================================

Engines::fileTransfer_ptr
Engines_Container_i::getFileTransfer()
{
  Engines::fileTransfer_var aFileTransfer
    = Engines::fileTransfer::_duplicate(_fileTransfer);
  return aFileTransfer._retn();
}


Engines::Salome_file_ptr 
Engines_Container_i::createSalome_file(const char* origFileName) 
{
  string origName(origFileName);
  if (CORBA::is_nil(_Salome_file_map[origName]))
    {
      Salome_file_i* aSalome_file = new Salome_file_i();
      aSalome_file->setContainer(Engines::Container::_duplicate(this->_this()));
      try 
      {
        aSalome_file->setLocalFile(origFileName);
        aSalome_file->recvFiles();
      }
      catch (const SALOME::SALOME_Exception& e)
      {
        return Engines::Salome_file::_nil();
      }

      Engines::Salome_file_var theSalome_file = Engines::Salome_file::_nil();
      theSalome_file = Engines::Salome_file::_narrow(aSalome_file->_this());
      _numInstanceMutex.lock() ; // lock to be alone (stl container write)
      _Salome_file_map[origName] = theSalome_file;
      _numInstanceMutex.unlock() ;
    }
  
  Engines::Salome_file_ptr theSalome_file =  
    Engines::Salome_file::_duplicate(_Salome_file_map[origName]);
  ASSERT(!CORBA::is_nil(theSalome_file));
  return theSalome_file;
}
//=============================================================================
/*! 
 *  C++ method: Finds an already existing servant instance of a component, or
 *              create an instance.
 *  ---- USE ONLY FOR MULTISTUDY INSTANCES ! --------
 *  \param genericRegisterName    Name of the component instance to register
 *                                in Registry & Name Service,
 *                                (without _inst_n suffix, like "COMPONENT")
 *  \param componentLibraryName   like "libCOMPONENTEngine.so"
 *  \return a loaded component
 * 
 *  example with names:
 *  aGenRegisterName = COMPONENT (= first argument)
 *  impl_name = libCOMPONENTEngine.so (= second argument)
 *  _containerName = /Containers/cli76ce/FactoryServer
 *  factoryName = COMPONENTEngine_factory
 *  component_registerBase = /Containers/cli76ce/FactoryServer/COMPONENT
 *
 *  instanceName = COMPONENT_inst_1
 *  component_registerName = /Containers/cli76ce/FactoryServer/COMPONENT_inst_1
 */
//=============================================================================

Engines::Component_ptr
Engines_Container_i::find_or_create_instance(string genericRegisterName,
                                             string componentLibraryName)
{
  string aGenRegisterName = genericRegisterName;
  string impl_name = componentLibraryName;
  void* handle = _library_map[impl_name];
  if ( !handle )
    {
      INFOS("shared library " << impl_name <<"must be loaded before instance");
      return Engines::Component::_nil() ;
    }
  else
    {
      // --- find a registered instance in naming service, or create

      string component_registerBase =
        _containerName + "/" + aGenRegisterName;
      Engines::Component_var iobject = Engines::Component::_nil() ;
      try
        {
          CORBA::Object_var obj =
            _NS->ResolveFirst( component_registerBase.c_str());
          if ( CORBA::is_nil( obj ) )
            {
              iobject = createInstance(genericRegisterName,
                                       handle,
                                       0); // force multiStudy instance here !
            }
          else
            { 
              iobject = Engines::Component::_narrow( obj ) ;
              Engines_Component_i *servant =
                dynamic_cast<Engines_Component_i*>
                (_poa->reference_to_servant(iobject));
              ASSERT(servant)
              int studyId = servant->getStudyId();
              ASSERT (studyId >= 0);
              if (studyId == 0) // multiStudy instance, OK
                {
                  // No ReBind !
                  MESSAGE(component_registerBase.c_str()<<" already bound");
                }
              else // monoStudy instance: NOK
                {
                  iobject = Engines::Component::_nil();
                  INFOS("load_impl & find_component_instance methods "
                        << "NOT SUITABLE for mono study components");
                }
            }
        }
      catch (...)
        {
          INFOS( "Container_i::load_impl catched" ) ;
        }
      return iobject._retn();
    }
}

//=============================================================================
/*! 
 *  C++ method: create a servant instance of a component.
 *  \param genericRegisterName    Name of the component instance to register
 *                                in Registry & Name Service,
 *                                (without _inst_n suffix, like "COMPONENT")
 *  \param handle                 loaded library handle
 *  \param studyId                0 for multiStudy instance, 
 *                                study Id (>0) otherwise
 *  \return a loaded component
 * 
 *  example with names:
 *  aGenRegisterName = COMPONENT (= first argument)
 *  _containerName = /Containers/cli76ce/FactoryServer
 *  factoryName = COMPONENTEngine_factory
 *  component_registerBase = /Containers/cli76ce/FactoryServer/COMPONENT
 *  instanceName = COMPONENT_inst_1
 *  component_registerName = /Containers/cli76ce/FactoryServer/COMPONENT_inst_1
 */
//=============================================================================

Engines::Component_ptr
Engines_Container_i::createInstance(string genericRegisterName,
                                    void *handle,
                                    int studyId)
{
  // --- find the factory

  string aGenRegisterName = genericRegisterName;
  string factory_name = aGenRegisterName + string("Engine_factory");
  SCRUTE(factory_name) ;

  typedef  PortableServer::ObjectId * (*FACTORY_FUNCTION)
    (CORBA::ORB_ptr,
     PortableServer::POA_ptr, 
     PortableServer::ObjectId *, 
     const char *, 
     const char *) ;

#ifndef WNT
  FACTORY_FUNCTION Component_factory = (FACTORY_FUNCTION)dlsym( handle, factory_name.c_str() );
#else
  FACTORY_FUNCTION Component_factory = (FACTORY_FUNCTION)GetProcAddress( (HINSTANCE)handle, factory_name.c_str() );
#endif

  if ( !Component_factory )
  {
      INFOS( "Can't resolve symbol: " + factory_name );
#ifndef WNT
      SCRUTE( dlerror() );
#endif
      return Engines::Component::_nil() ;
  }

  // --- create instance

  Engines::Component_var iobject = Engines::Component::_nil() ;

  try
    {
      _numInstanceMutex.lock() ; // lock on the instance number
      _numInstance++ ;
      int numInstance = _numInstance ;
      _numInstanceMutex.unlock() ;

      char aNumI[12];
      sprintf( aNumI , "%d" , numInstance ) ;
      string instanceName = aGenRegisterName + "_inst_" + aNumI ;
      string component_registerName =
        _containerName + "/" + instanceName;

      // --- Instanciate required CORBA object

      PortableServer::ObjectId *id ; //not owner, do not delete (nore use var)
      id = (Component_factory) ( _orb, _poa, _id, instanceName.c_str(),
                                 aGenRegisterName.c_str() ) ;
      if (id == NULL)
        return iobject._retn();
      
      // --- get reference & servant from id

      CORBA::Object_var obj = _poa->id_to_reference(*id);
      iobject = Engines::Component::_narrow( obj ) ;

      Engines_Component_i *servant =
        dynamic_cast<Engines_Component_i*>(_poa->reference_to_servant(iobject));
      ASSERT(servant);
      //SCRUTE(servant->pd_refCount);
      servant->_remove_ref(); // compensate previous id_to_reference 
      //SCRUTE(servant->pd_refCount);
      _numInstanceMutex.lock() ; // lock to be alone (stl container write)
      _listInstances_map[instanceName] = iobject;
      _cntInstances_map[aGenRegisterName] += 1;
      _numInstanceMutex.unlock() ;
      SCRUTE(aGenRegisterName);
      SCRUTE(_cntInstances_map[aGenRegisterName]);
      //SCRUTE(servant->pd_refCount);
      bool ret_studyId = servant->setStudyId(studyId);
      ASSERT(ret_studyId);

      // --- register the engine under the name
      //     containerName(.dir)/instanceName(.object)

      _NS->Register( iobject , component_registerName.c_str() ) ;
      MESSAGE( component_registerName.c_str() << " bound" ) ;
    }
  catch (...)
    {
      INFOS( "Container_i::createInstance exception catched" ) ;
    }
  return iobject._retn();
}

//=============================================================================
/*! 
 *
 */
//=============================================================================

void Engines_Container_i::decInstanceCnt(string genericRegisterName)
{
  string aGenRegisterName =genericRegisterName;
  MESSAGE("Engines_Container_i::decInstanceCnt " << aGenRegisterName);
  ASSERT(_cntInstances_map[aGenRegisterName] > 0); 
  _numInstanceMutex.lock(); // lock to be alone
                            // (see finalize_removal, load_component_Library)
  _cntInstances_map[aGenRegisterName] -= 1;
  SCRUTE(_cntInstances_map[aGenRegisterName]);
  if (_cntInstances_map[aGenRegisterName] == 0)
    {
      string impl_name =
        Engines_Component_i::GetDynLibraryName(aGenRegisterName.c_str());
      SCRUTE(impl_name);
      void* handle = _library_map[impl_name];
      ASSERT(handle);
      _toRemove_map[impl_name] = handle;
    }
  _numInstanceMutex.unlock();
}

//=============================================================================
/*! 
 *  Retrieves only with container naming convention if it is a python container
 */
//=============================================================================

bool Engines_Container_i::isPythonContainer(const char* ContainerName)
{
  bool ret=false;
  int len=strlen(ContainerName);
  if(len>=2)
    if(strcmp(ContainerName+len-2,"Py")==0)
      ret=true;
  return ret;
}

//=============================================================================
/*! 
 *  
 */
//=============================================================================

void ActSigIntHandler()
{
#ifndef WNT
  struct sigaction SigIntAct ;
  SigIntAct.sa_sigaction = &SigIntHandler ;
  SigIntAct.sa_flags = SA_SIGINFO ;
#endif

// DEBUG 03.02.2005 : the first parameter of sigaction is not a mask of signals
// (SIGINT | SIGUSR1) :
// it must be only one signal ===> one call for SIGINT 
// and an other one for SIGUSR1

#ifndef WNT
  if ( sigaction( SIGINT , &SigIntAct, NULL ) ) 
    {
      perror("SALOME_Container main ") ;
      exit(0) ;
    }
  if ( sigaction( SIGUSR1 , &SigIntAct, NULL ) )
    {
      perror("SALOME_Container main ") ;
      exit(0) ;
    }
  if ( sigaction( SIGUSR2 , &SigIntAct, NULL ) )
    {
      perror("SALOME_Container main ") ;
      exit(0) ;
    }

  //PAL9042 JR : during the execution of a Signal Handler (and of methods called through Signal Handlers)
  //             use of streams (and so on) should never be used because :
  //             streams of C++ are naturally thread-safe and use pthread_mutex_lock ===>
  //             A stream operation may be interrupted by a signal and if the Handler use stream we
  //             may have a "Dead-Lock" ===HangUp
  //==INFOS is commented
  //  INFOS(pthread_self() << "SigIntHandler activated") ;

#else  
  signal( SIGINT, SigIntHandler );
  signal( SIGUSR1, SigIntHandler );
#endif

}

void SetCpuUsed() ;
void CallCancelThread() ;

#ifndef WNT
void SigIntHandler(int what ,
                   siginfo_t * siginfo ,
                   void * toto ) 
{
  //PAL9042 JR : during the execution of a Signal Handler (and of methods called through Signal Handlers)
  //             use of streams (and so on) should never be used because :
  //             streams of C++ are naturally thread-safe and use pthread_mutex_lock ===>
  //             A stream operation may be interrupted by a signal and if the Handler use stream we
  //             may have a "Dead-Lock" ===HangUp
  //==MESSAGE is commented
  //  MESSAGE(pthread_self() << "SigIntHandler what     " << what << endl
  //          << "              si_signo " << siginfo->si_signo << endl
  //          << "              si_code  " << siginfo->si_code << endl
  //          << "              si_pid   " << siginfo->si_pid) ;

  if ( _Sleeping )
    {
      _Sleeping = false ;
      //     MESSAGE("SigIntHandler END sleeping.") ;
      return ;
    }
  else
    {
      ActSigIntHandler() ;
      if ( siginfo->si_signo == SIGUSR1 )
        {
          SetCpuUsed() ;
        }
      else if ( siginfo->si_signo == SIGUSR2 )
        {
          CallCancelThread() ;
        }
      else 
        {
          _Sleeping = true ;
          //      MESSAGE("SigIntHandler BEGIN sleeping.") ;
          int count = 0 ;
          while( _Sleeping )
            {
              sleep( 1 ) ;
              count += 1 ;
            }
          //      MESSAGE("SigIntHandler LEAVE sleeping after " << count << " s.") ;
        }
      return ;
    }
}
#else // Case WNT
void SigIntHandler( int what )
{
#ifndef WNT
  MESSAGE( pthread_self() << "SigIntHandler what     " << what << endl );
#else
  MESSAGE( "SigIntHandler what     " << what << endl );
#endif
  if ( _Sleeping )
    {
      _Sleeping = false ;
      MESSAGE("SigIntHandler END sleeping.") ;
      return ;
    }
  else
    {
      ActSigIntHandler() ;
      if ( what == SIGUSR1 )
        {
          SetCpuUsed() ;
        }
      else
        {
          _Sleeping = true ;
          MESSAGE("SigIntHandler BEGIN sleeping.") ;
          int count = 0 ;
          while( _Sleeping ) 
            {
              Sleep( 1000 ) ;
              count += 1 ;
            }
          MESSAGE("SigIntHandler LEAVE sleeping after " << count << " s.") ;
        }
      return ;
    }
}
#endif
