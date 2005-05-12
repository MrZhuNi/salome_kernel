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
//  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
//
//
//
//  File   : Container_i.cxx
//  Author : Paul RASCLE, EDF - MARC TAJCHMAN, CEA 
//  Module : SALOME
//  $Header$

//#define private public
#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SALOME_Component)
#include "SALOME_Container_i.hxx"
#include "SALOME_Component_i.hxx"
#include "SALOME_NamingService.hxx"
#include "OpUtil.hxx"
#include <string.h>
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>

#include "utilities.h"
using namespace std;

bool _Sleeping = false ;

// Needed by multi-threaded Python
int _ArgC ;
char ** _ArgV ;


// Containers with name FactoryServer are started via rsh in LifeCycleCORBA
// Other Containers are started via start_impl of FactoryServer

extern "C" {void ActSigIntHandler() ; }
extern "C" {void SigIntHandler(int, siginfo_t *, void *) ; }

const char *Engines_Container_i::_defaultContainerName="FactoryServer";
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
					  PortableServer::POA_ptr poa,
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

  _ArgC = argc ;
  _ArgV = argv ;

  _argc = argc ;
  _argv = argv ;
  int i = strlen( _argv[ 0 ] ) - 1 ;
  while ( i >= 0 )
    {
      if ( _argv[ 0 ][ i ] == '/' )
	{
	  _argv[ 0 ][ i+1 ] = '\0' ;
	  break ;
	}
      i -= 1 ;
    }
  string hostname = GetHostname();
  MESSAGE(hostname << " " << getpid() << " Engines_Container_i starting argc "
	  << _argc << " Thread " << pthread_self() ) ;
  i = 0 ;
  while ( _argv[ i ] )
    {
      MESSAGE("           argv" << i << " " << _argv[ i ]) ;
      i++ ;
    }
  if ( argc != 4 )
    {
      MESSAGE("SALOME_Container usage : SALOME_Container ServerName " <<
	      "-ORBInitRef NameService=corbaname::hostname:tcpipPortNumber") ;
      //    exit(0) ;
    }

  _containerName = BuildContainerNameForNS(containerName,hostname.c_str());
  
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
      SCRUTE(_containerName);
      _NS->Register(pCont, _containerName.c_str()); 
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
  MESSAGE("Engines_Container_i::getHostName " << s);
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
  _NS->Destroy_Name(_containerName.c_str());
  //_remove_ref();
  //_poa->deactivate_object(*_id);
  if(_isServantAloneInProcess)
    _orb->shutdown(0);
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
  string impl_name = string ("lib") + componentName + string("Engine.so");
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
  void* handle;
  handle = dlopen( impl_name.c_str() , RTLD_LAZY ) ;
  if ( !handle )
    {
      INFOS("Can't load shared library : " << impl_name);
      INFOS("error dlopen: " << dlerror());
      _numInstanceMutex.unlock();
      return false;
    }
  else
    {
      _library_map[impl_name] = handle;
      _numInstanceMutex.unlock();
      return true;
    }
  _numInstanceMutex.unlock();
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

  string impl_name = string ("lib") + genericRegisterName +string("Engine.so");
  void* handle = _library_map[impl_name];
  if ( !handle )
    {
      INFOS("shared library " << impl_name <<"must be loaded before instance");
      return Engines::Component::_nil() ;
    }
  else
    {
      Engines::Component_var iobject = Engines::Component::_nil() ;
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
  _listInstances_map.erase(instanceName);
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
// 	  dlclose(handle);                // SALOME unstable after ...
// 	  _library_map.erase(impl_name);
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

  FACTORY_FUNCTION Component_factory
    = (FACTORY_FUNCTION) dlsym(handle, factory_name.c_str());

  char *error ;
  if ( (error = dlerror() ) != NULL)
    {
      INFOS("Can't resolve symbol: " + factory_name);
      SCRUTE(error);
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

      // --- get reference & servant from id

      CORBA::Object_var obj = _poa->id_to_reference(*id);
      iobject = Engines::Component::_narrow( obj ) ;

      Engines_Component_i *servant =
	dynamic_cast<Engines_Component_i*>(_poa->reference_to_servant(iobject));
      ASSERT(servant);
      //SCRUTE(servant->pd_refCount);
      servant->_remove_ref(); // compensate previous id_to_reference 
      //SCRUTE(servant->pd_refCount);
      _listInstances_map[instanceName] = iobject;
      _cntInstances_map[aGenRegisterName] += 1;
      SCRUTE(aGenRegisterName);
      SCRUTE(_cntInstances_map[aGenRegisterName]);
      //SCRUTE(servant->pd_refCount);
      ASSERT(servant->setStudyId(studyId));

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
 *  Returns string = container path + name, to use in Naming service
 */
//=============================================================================

string Engines_Container_i::BuildContainerNameForNS(const char *ContainerName,
						    const char *hostname)
{
  string ret="/Containers/";
  ret += hostname;
  ret+="/";
  if (strlen(ContainerName)== 0)
    ret+=_defaultContainerName;
  else
    ret += ContainerName;
  return ret;
}

//=============================================================================
/*! 
 *  
 */
//=============================================================================

void ActSigIntHandler()
{
  struct sigaction SigIntAct ;
  SigIntAct.sa_sigaction = &SigIntHandler ;
  SigIntAct.sa_flags = SA_SIGINFO ;
  // DEBUG 03.02.2005 : the first parameter of sigaction is not a mask of signals (SIGINT | SIGUSR1) :
  // it must be only one signal ===> one call for SIGINT and an other one for SIGUSR1
  if ( sigaction( SIGINT , &SigIntAct, NULL ) ) {
    perror("SALOME_Container main ") ;
    exit(0) ;
  }
  if ( sigaction( SIGUSR1 , &SigIntAct, NULL ) ) {
    perror("SALOME_Container main ") ;
    exit(0) ;
  }
  INFOS(pthread_self() << "SigIntHandler activated") ;
}

void SetCpuUsed() ;

void SigIntHandler(int what , siginfo_t * siginfo ,
                                        void * toto )
{
  MESSAGE(pthread_self() << "SigIntHandler what     " << what << endl
          << "              si_signo " << siginfo->si_signo << endl
          << "              si_code  " << siginfo->si_code << endl
          << "              si_pid   " << siginfo->si_pid) ;
  if ( _Sleeping ) {
    _Sleeping = false ;
    MESSAGE("SigIntHandler END sleeping.") ;
    return ;
  }
  else {
    ActSigIntHandler() ;
    if ( siginfo->si_signo == SIGUSR1 ) {
      SetCpuUsed() ;
    }
    else {
      _Sleeping = true ;
      MESSAGE("SigIntHandler BEGIN sleeping.") ;
      int count = 0 ;
      while( _Sleeping ) {
        sleep( 1 ) ;
        count += 1 ;
      }
      MESSAGE("SigIntHandler LEAVE sleeping after " << count << " s.") ;
    }
    return ;
  }
}

//=============================================================================
/*! 
 *  CORBA method: Create one instance of componentName component 
 *  and register it as genericRegisterName in naming service
 */
//=============================================================================

// Engines::Component_ptr Engines_Container_i::instance( const char* genericRegisterName,
// 	                                              const char* componentName )
// {
//   _numInstanceMutex.lock() ; // lock on the instance number
//   BEGIN_OF( "Container_i::instance " << componentName ) ;

//   string _genericRegisterName = genericRegisterName;
//   string component_registerName = _containerName + "/" + _genericRegisterName;
  
//   Engines::Component_var iobject = Engines::Component::_nil() ;
  
//   try 
//     {
//       CORBA::Object_var obj = _NS->Resolve( component_registerName.c_str() ) ;
//       if (! CORBA::is_nil( obj ) )
// 	{
// 	  MESSAGE( "Container_i::instance " << component_registerName.c_str() << " already registered" ) ;
// 	  iobject = Engines::Component::_narrow( obj ) ;
// 	}
//       else
// 	{
// 	  string _compo_name = componentName;
// 	  string _impl_name = "lib" + _compo_name + "Engine.so";
// 	  SCRUTE(_impl_name);
      
// 	  void* handle;
// 	  handle = dlopen( _impl_name.c_str() , RTLD_LAZY ) ;
	  
// 	  if ( handle )
// 	    {
// 	      string factory_name = _compo_name + "Engine_factory";
// 	      SCRUTE(factory_name) ;
	      
// 	      typedef  PortableServer::ObjectId * (*FACTORY_FUNCTION)
// 		(CORBA::ORB_ptr,
// 		 PortableServer::POA_ptr, 
// 		 PortableServer::ObjectId *, 
// 		 const char *, 
// 		 const char *) ; 
// 	      FACTORY_FUNCTION Component_factory = (FACTORY_FUNCTION) dlsym(handle, factory_name.c_str());

// 	      char *error ;
// 	      if ( (error = dlerror() ) == NULL)
// 		{
// 		  // Instanciate required CORBA object
// 		  _numInstance++ ;
// 		  char _aNumI[12];
// 		  sprintf( _aNumI , "%d" , _numInstance ) ;
// 		  string instanceName = _compo_name + "_inst_" + _aNumI ;
// 		  SCRUTE(instanceName);
		  
// 		  PortableServer::ObjectId * id ;
// 		  id = (Component_factory) ( _orb, _poa, _id, instanceName.c_str() ,
// 					     _genericRegisterName.c_str() ) ;
// 		  // get reference from id
// 		  obj = _poa->id_to_reference(*id);
// 		  iobject = Engines::Component::_narrow( obj ) ;
		  
// 		  // register the engine under the name containerName.dir/genericRegisterName.object
// 		  _NS->Register( iobject , component_registerName.c_str() ) ;
// 		  MESSAGE( "Container_i::instance " << component_registerName.c_str() << " registered" ) ;
// 		  _handle_map[instanceName] = handle;
// 		}
// 	      else
// 		{
// 		  INFOS("Can't resolve symbol: " + factory_name);
// 		  SCRUTE(error);
// 		}  
// 	    }
// 	  else
// 	    {
// 	      INFOS("Can't load shared library : " << _impl_name);
// 	      INFOS("error dlopen: " << dlerror());
// 	    }      
// 	}
//     }
//   catch (...)
//     {
//       INFOS( "Container_i::instance exception caught" ) ;
//     }
//   END_OF("Container_i::instance");
//   _numInstanceMutex.unlock() ;
//   return Engines::Component::_duplicate(iobject);
// }

//=============================================================================
/*! 
 *  CORBA attribute: Machine Name (hostname without domain extensions)
 */
//=============================================================================

// char* Engines_Container_i::machineName()
// {
//   string s = GetHostname();
//   MESSAGE("Engines_Container_i::machineName " << s);
//    return CORBA::string_dup(s.c_str()) ;
// }
