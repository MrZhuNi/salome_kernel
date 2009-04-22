//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
#include "SALOME_ContainerManager.hxx"
#include "SALOME_NamingService.hxx"
#include "SALOME_ModuleCatalog.hh"
#include "Basics_Utils.hxx"
#include "Basics_DirUtils.hxx"
#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <vector>
#include "Utils_CorbaException.hxx"
#include "Batch_Date.hxx"
#include <sstream>

#ifdef WITH_PACO_PARALLEL
#include "PaCOPP.hxx"
#endif

#define TIME_OUT_TO_LAUNCH_CONT 61

using namespace std;

vector<Engines::Container_ptr> SALOME_ContainerManager::_batchLaunchedContainers;

vector<Engines::Container_ptr>::iterator SALOME_ContainerManager::_batchLaunchedContainersIter;

const char *SALOME_ContainerManager::_ContainerManagerNameInNS = 
  "/ContainerManager";

//=============================================================================
/*! 
 *  Constructor
 *  \param orb
 *  Define a CORBA single thread policy for the server, which avoid to deal
 *  with non thread-safe usage like Change_Directory in SALOME naming service
 */
//=============================================================================

SALOME_ContainerManager::SALOME_ContainerManager(CORBA::ORB_ptr orb, PortableServer::POA_var poa, SALOME_ResourcesManager *rm, SALOME_NamingService *ns)
{
  MESSAGE("constructor");
  _NS = ns;
  _ResManager = rm;

  PortableServer::POAManager_var pman = poa->the_POAManager();
  _orb = CORBA::ORB::_duplicate(orb) ;
  CORBA::PolicyList policies;
  policies.length(1);
  PortableServer::ThreadPolicy_var threadPol = 
    poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
  policies[0] = PortableServer::ThreadPolicy::_duplicate(threadPol);

  _poa = poa->create_POA("SThreadPOA",pman,policies);
  threadPol->destroy();
  PortableServer::ObjectId_var id = _poa->activate_object(this);
  CORBA::Object_var obj = _poa->id_to_reference(id);
  Engines::ContainerManager_var refContMan =
    Engines::ContainerManager::_narrow(obj);

  _NS->Register(refContMan,_ContainerManagerNameInNS);
  _MpiStarted = false;
  _isAppliSalomeDefined = (getenv("APPLI") != 0);
  MESSAGE("constructor end");
}

//=============================================================================
/*! 
 * destructor
 */
//=============================================================================

SALOME_ContainerManager::~SALOME_ContainerManager()
{
  MESSAGE("destructor");
}

//=============================================================================
/*! CORBA method:
 *  shutdown all the containers, then the ContainerManager servant
 */
//=============================================================================

void SALOME_ContainerManager::Shutdown()
{
  MESSAGE("Shutdown");
  ShutdownContainers();
  _NS->Destroy_Name(_ContainerManagerNameInNS);
  PortableServer::ObjectId_var oid = _poa->servant_to_id(this);
  _poa->deactivate_object(oid);
  //_remove_ref() has already been done at creation
  //_remove_ref();
}

//=============================================================================
/*! CORBA Method:
 *  Loop on all the containers listed in naming service, ask shutdown on each
 */
//=============================================================================

void SALOME_ContainerManager::ShutdownContainers()
{
  MESSAGE("ShutdownContainers");
  bool isOK;
  isOK = _NS->Change_Directory("/Containers");
  if( isOK ){
    vector<string> vec = _NS->list_directory_recurs();
    list<string> lstCont;
    for(vector<string>::iterator iter = vec.begin();iter!=vec.end();iter++)
      {
        SCRUTE((*iter));
        CORBA::Object_var obj=_NS->Resolve((*iter).c_str());
        try
          {
            Engines::Container_var cont=Engines::Container::_narrow(obj);
            if(!CORBA::is_nil(cont))
	      lstCont.push_back((*iter));
          }
        catch(const CORBA::Exception& e)
          {
            // ignore this entry and continue
          }
      }
    MESSAGE("Container list: ");
    for(list<string>::iterator iter=lstCont.begin();iter!=lstCont.end();iter++){
      SCRUTE((*iter));
    }
    for(list<string>::iterator iter=lstCont.begin();iter!=lstCont.end();iter++)
    {
      try
      {
	SCRUTE((*iter));
	CORBA::Object_var obj=_NS->Resolve((*iter).c_str());
	Engines::Container_var cont=Engines::Container::_narrow(obj);
	if(!CORBA::is_nil(cont))
	{
	  MESSAGE("ShutdownContainers: " << (*iter));
	  cont->Shutdown();
	}
	else 
	  MESSAGE("ShutdownContainers: no container ref for " << (*iter));
      }
      catch(CORBA::SystemException& e)
      {
	INFOS("CORBA::SystemException ignored : " << e);
      }
      catch(CORBA::Exception&)
      {
	INFOS("CORBA::Exception ignored.");
      }
      catch(...)
      {
	INFOS("Unknown exception ignored.");
      }
    }
  }
}

//=============================================================================
//!  Find a suitable Container in a list of machines, or start one
/*! CORBA Method:
 *  \param params            Machine Parameters required for the container
 *  \param possibleComputers list of machines usable for find or start
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
FindOrStartContainer(const Engines::MachineParameters& params,
		     const Engines::MachineList& possibleComputers)
{
  Engines::Container_ptr ret = FindContainer(params,possibleComputers);
  if(!CORBA::is_nil(ret))
    return ret;
  MESSAGE("Container doesn't exist try to launch it ...");

  return StartContainer(params,possibleComputers,Engines::P_FIRST);

}

//=============================================================================
//! Start a suitable Container in a list of machines with constraints and a policy
/*! C++ Method:
 * Constraints are given by a machine parameters struct
 *  \param params            Machine Parameters required for the container
 *  \param possibleComputers list of machines usable for start
 *  \param policy        policy to use (first,cycl or best)
 *  \param container_exe specific container executable (default=SALOME_Container)
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
StartContainer(const Engines::MachineParameters& params,
	       const Engines::MachineList& possibleComputers,
	       Engines::ResPolicy policy,const std::string& container_exe)
{
#ifdef WITH_PACO_PARALLEL
  std::string parallelLib(params.parallelLib);
  if (parallelLib != "")
    return StartParallelContainer(params, policy, possibleComputers);
#endif
  string containerNameInNS;
  Engines::Container_ptr ret = Engines::Container::_nil();

  MESSAGE("SALOME_ContainerManager::StartContainer " <<
	  possibleComputers.length());

  vector<string> lm;
  for(unsigned int i=0;i<possibleComputers.length();i++)
    lm.push_back(string(possibleComputers[i]));

  string theMachine;
  try{
    switch(policy){
    case Engines::P_FIRST:
      theMachine=_ResManager->GetImpl()->FindFirst(lm);
      break;
    case Engines::P_CYCL:
      theMachine=_ResManager->GetImpl()->FindNext(lm);
      break;
    case Engines::P_BEST:
      theMachine=_ResManager->GetImpl()->FindBest(lm);
      break;
    }
  }
  catch( const SALOME_Exception &ex ){
    MESSAGE(ex.what());
    return Engines::Container::_nil();
  }

  //If the machine name is localhost use the real name
  if(theMachine == "localhost")
    theMachine=Kernel_Utils::GetHostname();

  MESSAGE("try to launch it on " << theMachine);

  string command;
  if(theMachine==""){
    MESSAGE("SALOME_ContainerManager::StartContainer : " <<
	    "no possible computer");
    return Engines::Container::_nil();
  }
  else if(theMachine==Kernel_Utils::GetHostname())
    command = BuildCommandToLaunchLocalContainer(params,container_exe);
  else
    command = BuildCommandToLaunchRemoteContainer(theMachine,params,container_exe);

  //check if an entry exists in Naming service
  if(params.isMPI)
    // A parallel container register on zero node in NS
    containerNameInNS = _NS->BuildContainerNameForNS(params,GetMPIZeroNode(theMachine).c_str());
  else
    containerNameInNS = _NS->BuildContainerNameForNS(params,theMachine.c_str());

  SCRUTE(containerNameInNS);
  CORBA::Object_var obj = _NS->Resolve(containerNameInNS.c_str());
  if ( !CORBA::is_nil(obj) )
    {
      try
        {
          // shutdown the registered container if it exists
          Engines::Container_var cont=Engines::Container::_narrow(obj);
          if(!CORBA::is_nil(cont))
            cont->Shutdown();
        }
      catch(CORBA::Exception&)
        {
          INFOS("CORBA::Exception ignored.");
        }
    }

  //redirect stdout and stderr in a file
  string logFilename="/tmp/"+_NS->ContainerName(params)+"_"+ theMachine +"_"+getenv( "USER" )+".log" ;
  command += " > " + logFilename + " 2>&1 &";

  // launch container with a system call
  int status=system(command.c_str());


  if (status == -1){
    MESSAGE("SALOME_LifeCycleCORBA::StartOrFindContainer rsh failed " <<
	    "(system command status -1)");
    RmTmpFile(_TmpFileName); // command file can be removed here
    return Engines::Container::_nil();
  }
  else if (status == 217){
    MESSAGE("SALOME_LifeCycleCORBA::StartOrFindContainer rsh failed " <<
	    "(system command status 217)");
    RmTmpFile(_TmpFileName); // command file can be removed here
    return Engines::Container::_nil();
  }
  else{
    int count=TIME_OUT_TO_LAUNCH_CONT;
    MESSAGE("count = "<<count);
    while ( CORBA::is_nil(ret) && count ){
#ifndef WIN32
      sleep( 1 ) ;
#else
      Sleep(1000);
#endif
      count-- ;
      if ( count != 10 )
	MESSAGE( count << ". Waiting for container on " << theMachine);

      CORBA::Object_var obj = _NS->Resolve(containerNameInNS.c_str());
      ret=Engines::Container::_narrow(obj);
    }
    
    if ( CORBA::is_nil(ret) )
      {
        MESSAGE("SALOME_LifeCycleCORBA::StartOrFindContainer rsh failed");
      }
    else
      {
        logFilename=":"+logFilename;
        logFilename="@"+Kernel_Utils::GetHostname()+logFilename;
        logFilename=getenv( "USER" )+logFilename;
        ret->logfilename(logFilename.c_str());
      }

    RmTmpFile(_TmpFileName); // command file can be removed here
    return ret;
  }
}

//=============================================================================
//! Start a suitable Container for a list of components with constraints and a policy
/*! CORBA Method:
 *  \param params            Machine Parameters required for the container
 *  \param policy        policy to use (first,cycl or best)
 *  \param componentList list of component to be loaded on this container
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
StartContainer(const Engines::MachineParameters& params,
	       Engines::ResPolicy policy,
	       const Engines::CompoList& componentList)
{
  Engines::MachineList_var possibleComputers = _ResManager->GetFittingResources(params,componentList);

  // Look into ModulCatalog if a specific container must be launched
  CORBA::String_var container_exe;
  int found=0;
  try
    {
      CORBA::Object_var obj = _NS->Resolve("/Kernel/ModulCatalog");
      SALOME_ModuleCatalog::ModuleCatalog_var Catalog = SALOME_ModuleCatalog::ModuleCatalog::_narrow(obj) ;
      if (CORBA::is_nil (Catalog))
        return Engines::Container::_nil();
      // Loop through component list
      for(unsigned int i=0;i<componentList.length();i++)
        {
          const char* compoi = componentList[i];
          SALOME_ModuleCatalog::Acomponent_var compoInfo = Catalog->GetComponent(compoi);
          if (CORBA::is_nil (compoInfo))
            {
              continue;
            }
          SALOME_ModuleCatalog::ImplType impl=compoInfo->implementation_type();
          container_exe=compoInfo->implementation_name();
          if(impl==SALOME_ModuleCatalog::CEXE)
            {
              if(found)
                {
                  INFOS("ContainerManager Error: you can't have 2 CEXE component in the same container" );
                  return Engines::Container::_nil();
                }
              found=1;
            }
        }
    }
  catch (ServiceUnreachable&)
    {
      INFOS("Caught exception: Naming Service Unreachable");
      return Engines::Container::_nil();
    }
  catch (...)
    {
      INFOS("Caught unknown exception.");
      return Engines::Container::_nil();
    }

  if(found)
    return StartContainer(params,possibleComputers,policy,container_exe.in());
  else
    return StartContainer(params,possibleComputers,policy);
}

#ifdef WITH_PACO_PARALLEL
//=============================================================================
/*! CORBA Method:
 *  Find or Start a suitable PaCO++ Parallel Container in a list of machines.
 *  \param params            Machine Parameters required for the container
 *  \param possibleComputers list of machines usable for find or start
 *
 *  \return CORBA container reference.
 */
//=============================================================================
Engines::Container_ptr
SALOME_ContainerManager::
StartParallelContainer(const Engines::MachineParameters& params_const,
			     Engines::ResPolicy policy,
			     const Engines::MachineList& possibleComputers)
{
  CORBA::Object_var obj;
  PaCO::InterfaceManager_var container_proxy;
  Engines::Container_ptr ret = Engines::Container::_nil();
  Engines::MachineParameters params(params_const);

  // Step 1 : Try to find a suitable container
  // Currently not as good as could be since
  // we have to verified the number of nodes of the container
  // if a user tell that.
  ret = FindContainer(params, possibleComputers);
  if(CORBA::is_nil(ret)) {
    // Step 2 : Starting a new parallel container !
    INFOS("[StartParallelContainer] Starting a PaCO++ parallel container");

    // Step 3 : Choose a computer
    std::string theMachine = _ResManager->FindFirst(possibleComputers);
    //If the machine name is localhost use the real name
    if(theMachine == "localhost")
      theMachine=Kernel_Utils::GetHostname();

    if(theMachine == "") {
      INFOS("[StartParallelContainer] !!!!!!!!!!!!!!!!!!!!!!!!!!");
      INFOS("[StartParallelContainer] No possible computer found");
      INFOS("[StartParallelContainer] !!!!!!!!!!!!!!!!!!!!!!!!!!");
      return ret;
    }
    INFOS("[StartParallelContainer] on machine : " << theMachine);
    params.hostname = CORBA::string_dup(theMachine.c_str());

    // Step 4 : starting parallel container proxy
    Engines::MachineParameters params_proxy(params);
    std::string command_proxy;
    try 
    {
      command_proxy = BuildCommandToLaunchLocalParallelContainer("SALOME_ParallelContainerProxy", params_proxy);
    }
    catch(const SALOME_Exception & ex)
    {
      INFOS("[StartParallelContainer] Exception in BuildCommandToLaunchLocalParallelContainer");
      INFOS(ex.what());
      return ret;
    }
    params_proxy.nb_component_nodes = 0; // LaunchParallelContainer uses this value to know if it launches the proxy or the nodes
    obj = LaunchParallelContainer(command_proxy, params_proxy, _NS->ContainerName(params_proxy));
    if (CORBA::is_nil(obj))
    {
      INFOS("[StartParallelContainer] LaunchParallelContainer for proxy returns NIL !");
      return ret;
    }
    try 
    {
      container_proxy = PaCO::InterfaceManager::_narrow(obj);
    }
    catch(CORBA::SystemException& e)
    {
      INFOS("[StartParallelContainer] Exception in _narrow after LaunchParallelContainer for proxy !");
      INFOS("CORBA::SystemException : " << e);
      return ret;
    }
    catch(CORBA::Exception& e)
    {
      INFOS("[StartParallelContainer] Exception in _narrow after LaunchParallelContainer for proxy !");
      INFOS("CORBA::Exception" << e);
      return ret;
    }
    catch(...)
    {
      INFOS("[StartParallelContainer] Exception in _narrow after LaunchParallelContainer for proxy !");
      INFOS("Unknown exception !");
      return ret;
    }
    if (CORBA::is_nil(container_proxy))
    {
      INFOS("[StartParallelContainer] PaCO::InterfaceManager::_narrow returns NIL !");
      return ret;
    }

    // Step 5 : starting parallel container nodes
    std::string command_nodes;
    Engines::MachineParameters params_nodes(params);
    command_nodes = BuildCommandToLaunchLocalParallelContainer("SALOME_ParallelContainerNode", params_nodes);
    std::string container_generic_node_name = _NS->ContainerName(params) + "Node";
    obj = LaunchParallelContainer(command_nodes, params_nodes, container_generic_node_name);
    if (CORBA::is_nil(obj))
    {
      INFOS("[StartParallelContainer] LaunchParallelContainer for nodes returns NIL !");
      // Il faut tuer le proxy
      try 
      {
	Engines::Container_var proxy = Engines::Container::_narrow(container_proxy);
	proxy->Shutdown();
      }
      catch (...)
      {
	INFOS("[StartParallelContainer] Exception catched from proxy Shutdown...");
      }
      return ret;
    }

    // Step 6 : connecting nodes and the proxy to actually create a parallel container
    for (int i = 0; i < params.nb_component_nodes; i++) 
    {
      std::ostringstream tmp;
      tmp << i;
      std::string proc_number = tmp.str();
      std::string container_node_name = container_generic_node_name + proc_number;

      std::string theNodeMachine(params_nodes.hostname);
      std::string containerNameInNS = _NS->BuildContainerNameForNS(container_node_name.c_str(), theNodeMachine.c_str());
      obj = _NS->Resolve(containerNameInNS.c_str());
      if (CORBA::is_nil(obj)) 
      {
	INFOS("[StartParallelContainer] CONNECTION FAILED From Naming Service !");
	INFOS("[StartParallelContainer] Container name is " << containerNameInNS);
	return ret;
      }
      try
      {
	MESSAGE("[StartParallelContainer] Deploying node : " << container_node_name);
	PaCO::InterfaceParallel_var node = PaCO::InterfaceParallel::_narrow(obj);
	node->deploy();
	MESSAGE("[StartParallelContainer] node " << container_node_name << " is deployed");
      }
      catch(CORBA::SystemException& e)
      {
	INFOS("[StartParallelContainer] Exception in deploying node : " << containerNameInNS);
	INFOS("CORBA::SystemException : " << e);
	return ret;
      }
      catch(CORBA::Exception& e)
      {
	INFOS("[StartParallelContainer] Exception in deploying node : " << containerNameInNS);
	INFOS("CORBA::Exception" << e);
	return ret;
      }
      catch(...)
      {
	INFOS("[StartParallelContainer] Exception in deploying node : " << containerNameInNS);
	INFOS("Unknown exception !");
	return ret;
      }
    }

    // Step 7 : starting parallel container
    try 
    {
      MESSAGE ("[StartParallelContainer] Starting parallel object");
      container_proxy->start();
      MESSAGE ("[StartParallelContainer] Parallel object is started");
      ret = Engines::Container::_narrow(container_proxy);
    }
    catch(CORBA::SystemException& e)
    {
      INFOS("Caught CORBA::SystemException. : " << e);
    }
    catch(PortableServer::POA::ServantAlreadyActive&)
    {
      INFOS("Caught CORBA::ServantAlreadyActiveException");
    }
    catch(CORBA::Exception&)
    {
      INFOS("Caught CORBA::Exception.");
    }
    catch(std::exception& exc)
    {
      INFOS("Caught std::exception - "<<exc.what()); 
    }
    catch(...)
    {
      INFOS("Caught unknown exception.");
    }
  }
  return ret;
}
#else
//=============================================================================
/*! CORBA Method:
 *  Find or Start a suitable PaCO++ Parallel Container in a list of machines.
 *  \param params            Machine Parameters required for the container
 *  \param possibleComputers list of machines usable for find or start
 *
 *  \return CORBA container reference.
 */
//=============================================================================
Engines::Container_ptr
SALOME_ContainerManager::
StartParallelContainer(const Engines::MachineParameters& params,
		       Engines::ResPolicy policy,
		       const Engines::MachineList& possibleComputers)
{
  Engines::Container_ptr ret = Engines::Container::_nil();
  INFOS("[StartParallelContainer] is disabled !");
  INFOS("[StartParallelContainer] recompile SALOME Kernel to enable parallel extension");
  return ret;
}
#endif

//=============================================================================
//! Give a suitable Container for a list of components with constraints and a policy
/*! CORBA Method:
 *  \param params            Machine Parameters required for the container
 *  \param policy        policy to use (first,cycl or best)
 *  \param componentList list of component to be loaded on this container
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
GiveContainer(const Engines::MachineParameters& params,
              Engines::ResPolicy policy,
              const Engines::CompoList& componentList)
{
  char *valenv=getenv("SALOME_BATCH");
  if(valenv)
    if (strcmp(valenv,"1")==0)
      {
	if(_batchLaunchedContainers.empty())
	  fillBatchLaunchedContainers();

	if (_batchLaunchedContainersIter == _batchLaunchedContainers.end())
	  _batchLaunchedContainersIter = _batchLaunchedContainers.begin();

	Engines::Container_ptr rtn = Engines::Container::_duplicate(*_batchLaunchedContainersIter);
	_batchLaunchedContainersIter++;
        return rtn;
      }
  return StartContainer(params,policy,componentList);
}

//=============================================================================
/*! 
 * 
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
FindContainer(const Engines::MachineParameters& params,
	      const char *theMachine)
{
  string containerNameInNS(_NS->BuildContainerNameForNS(params,theMachine));
  CORBA::Object_var obj = _NS->Resolve(containerNameInNS.c_str());
  if( !CORBA::is_nil(obj) )
    return Engines::Container::_narrow(obj);
  else
    return Engines::Container::_nil();
}

//=============================================================================
/*! 
 * 
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
FindContainer(const Engines::MachineParameters& params,
	      const Engines::MachineList& possibleComputers)
{
  MESSAGE("FindContainer "<<possibleComputers.length());
  for(unsigned int i=0;i<possibleComputers.length();i++)
    {
      MESSAGE("FindContainer possible " << possibleComputers[i]);
      Engines::Container_ptr cont = FindContainer(params,possibleComputers[i]);
      if( !CORBA::is_nil(cont) )
	return cont;
    }
  MESSAGE("FindContainer: not found");
  return Engines::Container::_nil();
}

//=============================================================================
/*! This method launches the parallel container.
 *  It will may be placed on the ressources manager.
 *
 * \param command to launch
 * \param container's parameters
 * \param name of the container
 *
 * \return CORBA container reference
 */
//=============================================================================
CORBA::Object_ptr 
SALOME_ContainerManager::LaunchParallelContainer(const std::string& command, 
						 const Engines::MachineParameters& params,
						 const std::string& name)
{
  CORBA::Object_ptr obj = CORBA::Object::_nil();
  std::string containerNameInNS;
  std::string theMachine(params.hostname);
  int count = TIME_OUT_TO_LAUNCH_CONT;

  INFOS("[LaunchParallelContainer] Begin");
  int status = system(command.c_str());
  if (status == -1) {
    INFOS("[LaunchParallelContainer] failed : system command status -1");
    return obj;
  }
  else if (status == 217) {
    INFOS("[LaunchParallelContainer] failed : system command status 217");
    return obj;
  }

  if (params.nb_component_nodes == 0) 
  {
    // Proxy We have launch a proxy
    containerNameInNS = _NS->BuildContainerNameForNS((char*) name.c_str(), theMachine.c_str());
    INFOS("[LaunchParallelContainer]  Waiting for Parallel Container proxy " << containerNameInNS << " on " << theMachine);
    while (CORBA::is_nil(obj) && count) 
    {
#ifndef WIN32
      sleep(1) ;
#else
      Sleep(1000);
#endif
      count-- ;
      obj = _NS->Resolve(containerNameInNS.c_str());
    }
  }
  else 
  {
    INFOS("[LaunchParallelContainer] launching the nodes of the parallel container");
    // We are waiting all the nodes
    for (int i = 0; i < params.nb_component_nodes; i++) 
    {
      // Name of the node
      std::ostringstream tmp;
      tmp << i;
      std::string proc_number = tmp.str();
      std::string container_node_name = name + proc_number;
      containerNameInNS = _NS->BuildContainerNameForNS((char*) container_node_name.c_str(), theMachine.c_str());
      INFOS("[LaunchParallelContainer]  Waiting for Parallel Container node " << containerNameInNS << " on " << theMachine);
      while (CORBA::is_nil(obj) && count) {
#ifndef WIN32
	sleep(1) ;
#else
	Sleep(1000);
#endif
	count-- ;
	obj = _NS->Resolve(containerNameInNS.c_str());
      }
      if (CORBA::is_nil(obj))
      {
	INFOS("[LaunchParallelContainer] Launch of node failed (or not found) !");
	return obj;
      }
    }
  }
  if (CORBA::is_nil(obj)) 
    INFOS("[LaunchParallelContainer] failed");
  
  return obj;
}

void SALOME_ContainerManager::fillBatchLaunchedContainers()
{
  _batchLaunchedContainers.clear();
  _NS->Change_Directory("/Containers");
  vector<string> vec = _NS->list_directory_recurs();
  for(vector<string>::iterator iter = vec.begin();iter!=vec.end();iter++){
    CORBA::Object_var obj=_NS->Resolve((*iter).c_str());
    Engines::Container_ptr cont=Engines::Container::_narrow(obj);
    if(!CORBA::is_nil(cont)){
      _batchLaunchedContainers.push_back(cont);
    }
  }
  _batchLaunchedContainersIter=_batchLaunchedContainers.begin();
}

//=============================================================================
/*!
 *  This is no longer valid (C++ container are also python containers)
 */ 
//=============================================================================

bool isPythonContainer(const char* ContainerName)
{
  bool ret = false;
  int len = strlen(ContainerName);

  if (len >= 2)
    if (strcmp(ContainerName + len - 2, "Py") == 0)
      ret = true;

  return ret;
}

//=============================================================================
/*!
 *  Builds the script to be launched
 *
 *  If SALOME Application not defined ($APPLI),
 *  see BuildTempFileToLaunchRemoteContainer()
 *
 *  Else rely on distant configuration. Command is under the form (example):
 *  ssh user@machine distantPath/runRemote.sh hostNS portNS WORKINGDIR workingdir \
 *                   SALOME_Container containerName &"

 *  - where user is ommited if not specified in CatalogResources,
 *  - where distant path is always relative to user@machine $HOME, and
 *    equal to $APPLI if not specified in CatalogResources,
 *  - where hostNS is the hostname of CORBA naming server (set by scripts to
 *    use to launch SALOME and servers in $APPLI: runAppli.sh, runRemote.sh)
 *  - where portNS is the port used by CORBA naming server (set by scripts to
 *    use to launch SALOME and servers in $APPLI: runAppli.sh, runRemote.sh)
 *  - where workingdir is the requested working directory for the container.
 *    If WORKINGDIR (and workingdir) is not present the working dir will be $HOME
 */ 
//=============================================================================

string
SALOME_ContainerManager::BuildCommandToLaunchRemoteContainer
(const string& machine,
 const Engines::MachineParameters& params, const std::string& container_exe)
{
  string command;
  int nbproc;
	  
  if ( ! _isAppliSalomeDefined )
    command = BuildTempFileToLaunchRemoteContainer(machine, params);

  else
    {
      const ParserResourcesType& resInfo = _ResManager->GetImpl()->GetResourcesList(machine);

      if (params.isMPI)
        {
          if ( (params.nb_node <= 0) && (params.nb_proc_per_node <= 0) )
            nbproc = 1;
          else if ( params.nb_node == 0 )
            nbproc = params.nb_proc_per_node;
          else if ( params.nb_proc_per_node == 0 )
            nbproc = params.nb_node;
          else
            nbproc = params.nb_node * params.nb_proc_per_node;
        }

      // "ssh user@machine distantPath/runRemote.sh hostNS portNS WORKINGDIR workingdir \
        //  SALOME_Container containerName &"

      if (resInfo.Protocol == rsh)
        command = "rsh ";
      else if (resInfo.Protocol == ssh)
        command = "ssh ";
      else
        throw SALOME_Exception("Unknown protocol");

      if (resInfo.UserName != "")
	{
	  command += resInfo.UserName;
	  command += "@";
	}

      command += machine;
      command += " ";

      if (resInfo.AppliPath != "")
	command += resInfo.AppliPath; // path relative to user@machine $HOME
      else
	{
	  ASSERT(getenv("APPLI"));
	  command += getenv("APPLI"); // path relative to user@machine $HOME
	}

      command += "/runRemote.sh ";

      ASSERT(getenv("NSHOST")); 
      command += getenv("NSHOST"); // hostname of CORBA name server

      command += " ";
      ASSERT(getenv("NSPORT"));
      command += getenv("NSPORT"); // port of CORBA name server

      std::string wdir=params.workingdir.in();
      if(wdir != "")
        {
          command += " WORKINGDIR ";
          command += " '";
          if(wdir == "$TEMPDIR")
            wdir="\\$TEMPDIR";
          command += wdir; // requested working directory
          command += "'"; 
        }

      if(params.isMPI)
	{
	  command += " mpirun -np ";
	  std::ostringstream o;
	  o << nbproc << " ";
	  command += o.str();
#ifdef WITHLAM
	  command += "-x PATH,LD_LIBRARY_PATH,OMNIORB_CONFIG,SALOME_trace ";
#elif defined(WITHOPENMPI)
	  if( getenv("OMPI_URI_FILE") == NULL )
	    command += "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace";
	  else{
	    command += "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace -ompi-server file:";
	    command += getenv("OMPI_URI_FILE");
	  }
#endif	
	  command += " SALOME_MPIContainer ";
	}
      else
        command += " " +container_exe+ " ";

      command += _NS->ContainerName(params);
      command += " -";
      AddOmninamesParams(command);

      MESSAGE("command =" << command);
    }

  return command;
}

//=============================================================================
/*!
 *  builds the command to be launched.
 */ 
//=============================================================================

string
SALOME_ContainerManager::BuildCommandToLaunchLocalContainer
(const Engines::MachineParameters& params, const std::string& container_exe)
{
  _TmpFileName = BuildTemporaryFileName();
  string command;
  int nbproc = 0;

  ofstream command_file( _TmpFileName.c_str() );

  if (params.isMPI)
    {
      //command = "mpirun -np ";
      command_file << "mpirun -np ";

      if ( (params.nb_node <= 0) && (params.nb_proc_per_node <= 0) )
        nbproc = 1;
      else if ( params.nb_node == 0 )
        nbproc = params.nb_proc_per_node;
      else if ( params.nb_proc_per_node == 0 )
        nbproc = params.nb_node;
      else
        nbproc = params.nb_node * params.nb_proc_per_node;

      //std::ostringstream o;

      //o << nbproc << " ";
      command_file << nbproc << " ";

      //command += o.str();
#ifdef WITHLAM
      //command += "-x PATH,LD_LIBRARY_PATH,OMNIORB_CONFIG,SALOME_trace ";
      command_file << "-x PATH,LD_LIBRARY_PATH,OMNIORB_CONFIG,SALOME_trace ";
#elif defined(WITHOPENMPI)
      //command += "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace ";
      if( getenv("OMPI_URI_FILE") == NULL )
	command_file << "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace";
      else
        {
          command_file << "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace -ompi-server file:";
          command_file << getenv("OMPI_URI_FILE");
        }
#endif

      if (isPythonContainer(params.container_name))
        //command += "pyMPI SALOME_ContainerPy.py ";
        command_file << " pyMPI SALOME_ContainerPy.py ";
      else
        //command += "SALOME_MPIContainer ";
        command_file << " SALOME_MPIContainer ";
    }

  else
    {
      //command="";
      std::string wdir=params.workingdir.in();
      if(wdir != "")
        {
          // a working directory is requested
          if(wdir == "$TEMPDIR")
            {
              // a new temporary directory is requested
              string dir = Kernel_Utils::GetTmpDir();
#ifdef WIN32
              //command += "cd /d "+ dir +";";
              command_file << "cd /d " << dir << endl;
#else
              //command = "cd "+ dir +";";
              command_file << "cd " << dir << ";";
#endif

            }
          else
            {
              // a permanent directory is requested use it or create it
#ifdef WIN32
              //command="mkdir " + wdir;
              command_file << "mkdir " + wdir << endl;
              command_file << "cd /D " + wdir << endl;
#else
              //command="mkdir -p " + wdir + " && cd " + wdir + ";";
              command_file << "mkdir -p " << wdir << " && cd " << wdir + ";";
#endif
            }
        }
      if (isPythonContainer(params.container_name))
        //command += "SALOME_ContainerPy.py ";
        command_file << "SALOME_ContainerPy.py ";
      else
        //command += container_exe + " ";
        command_file << container_exe + " ";

    }

  command_file << _NS->ContainerName(params);
  command_file << " -";
  AddOmninamesParams(command_file);
  command_file.close();

#ifndef WIN32
  chmod(_TmpFileName.c_str(), 0x1ED);
#endif
  command = _TmpFileName;

  MESSAGE("Command is file ... " << command);
  return command;
}


//=============================================================================
/*!
 *  removes the generated temporary file in case of a remote launch.
 */ 
//=============================================================================

void SALOME_ContainerManager::RmTmpFile(std::string& tmpFileName)
{
  int lenght = tmpFileName.size();
  if ( lenght  > 0)
    {
#ifdef WIN32
      string command = "del /F ";
#else
      string command = "rm ";      
#endif
      if ( lenght > 4 )
        command += tmpFileName.substr(0, lenght - 3 );
      else
        command += tmpFileName;
      command += '*';
      system(command.c_str());
      //if dir is empty - remove it
      string tmp_dir = Kernel_Utils::GetDirByPath( tmpFileName );
      if ( Kernel_Utils::IsEmptyDir( tmp_dir ) )
        {
#ifdef WIN32
          command = "del /F " + tmp_dir;
#else
          command = "rmdir " + tmp_dir;
#endif
          system(command.c_str());
        }
    }
}

//=============================================================================
/*!
 *   add to command all options relative to naming service.
 */ 
//=============================================================================

void SALOME_ContainerManager::AddOmninamesParams(string& command) const
{
  CORBA::String_var iorstr = _NS->getIORaddr();
  command += "ORBInitRef NameService=";
  command += iorstr;
}


//=============================================================================
/*!
 *  add to command all options relative to naming service.
 */ 
//=============================================================================

void SALOME_ContainerManager::AddOmninamesParams(ofstream& fileStream) const
{
  CORBA::String_var iorstr = _NS->getIORaddr();
  fileStream << "ORBInitRef NameService=";
  fileStream << iorstr;
}

//=============================================================================
/*!
 *  generate a file name in /tmp directory
 */ 
//=============================================================================

string SALOME_ContainerManager::BuildTemporaryFileName() const
{
  //build more complex file name to support multiple salome session
  string aFileName = Kernel_Utils::GetTmpFileName();
#ifndef WIN32
  aFileName += ".sh";
#else
  aFileName += ".bat";
#endif
  return aFileName;
}


//=============================================================================
/*!
 *  Builds in a temporary file the script to be launched.
 *  
 *  Used if SALOME Application ($APPLI) is not defined.
 *  The command is build with data from CatalogResources, in which every path
 *  used on remote computer must be defined.
 */ 
//=============================================================================

string
SALOME_ContainerManager::BuildTempFileToLaunchRemoteContainer
(const string& machine,
 const Engines::MachineParameters& params) throw(SALOME_Exception)
{
  int status;

  _TmpFileName = BuildTemporaryFileName();
  ofstream tempOutputFile;
  tempOutputFile.open(_TmpFileName.c_str(), ofstream::out );
  const ParserResourcesType& resInfo = _ResManager->GetImpl()->GetResourcesList(machine);
  tempOutputFile << "#! /bin/sh" << endl;

  // --- set env vars

  tempOutputFile << "export SALOME_trace=local" << endl; // mkr : 27.11.2006 : PAL13967 - Distributed supervision graphs - Problem with "SALOME_trace"
  //tempOutputFile << "source " << resInfo.PreReqFilePath << endl;

  // ! env vars

  if (params.isMPI)
    {
      tempOutputFile << "mpirun -np ";
      int nbproc;

      if ( (params.nb_node <= 0) && (params.nb_proc_per_node <= 0) )
        nbproc = 1;
      else if ( params.nb_node == 0 )
        nbproc = params.nb_proc_per_node;
      else if ( params.nb_proc_per_node == 0 )
        nbproc = params.nb_node;
      else
        nbproc = params.nb_node * params.nb_proc_per_node;

      std::ostringstream o;

      tempOutputFile << nbproc << " ";
#ifdef WITHLAM
      tempOutputFile << "-x PATH,LD_LIBRARY_PATH,OMNIORB_CONFIG,SALOME_trace ";
#elif defined(WITHOPENMPI)
      if( getenv("OMPI_URI_FILE") == NULL )
	tempOutputFile << "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace";
      else{
	tempOutputFile << "-x PATH -x LD_LIBRARY_PATH -x OMNIORB_CONFIG -x SALOME_trace -ompi-server file:";
	tempOutputFile << getenv("OMPI_URI_FILE");
      }
#endif
    }

  tempOutputFile << getenv("KERNEL_ROOT_DIR") << "/bin/salome/";

  if (params.isMPI)
    {
      if (isPythonContainer(params.container_name))
        tempOutputFile << " pyMPI SALOME_ContainerPy.py ";
      else
        tempOutputFile << " SALOME_MPIContainer ";
    }

  else
    {
      if (isPythonContainer(params.container_name))
        tempOutputFile << "SALOME_ContainerPy.py ";
      else
        tempOutputFile << "SALOME_Container ";
    }

  tempOutputFile << _NS->ContainerName(params) << " -";
  AddOmninamesParams(tempOutputFile);
  tempOutputFile << " &" << endl;
  tempOutputFile.flush();
  tempOutputFile.close();
#ifndef WIN32
  chmod(_TmpFileName.c_str(), 0x1ED);
#endif

  // --- Build command

  string command;

  if (resInfo.Protocol == rsh)
    {
      command = "rsh ";
      string commandRcp = "rcp ";
      commandRcp += _TmpFileName;
      commandRcp += " ";
      commandRcp += machine;
      commandRcp += ":";
      commandRcp += _TmpFileName;
      status = system(commandRcp.c_str());
    }

  else if (resInfo.Protocol == ssh)
    {
      command = "ssh ";
      string commandRcp = "scp ";
      commandRcp += _TmpFileName;
      commandRcp += " ";
      commandRcp += machine;
      commandRcp += ":";
      commandRcp += _TmpFileName;
      status = system(commandRcp.c_str());
    }
  else
    throw SALOME_Exception("Unknown protocol");

  if(status)
    throw SALOME_Exception("Error of connection on remote host");    

  command += machine;
  _CommandForRemAccess = command;
  command += " ";
  command += _TmpFileName;

  SCRUTE(command);

  return command;

}

//=============================================================================
/*! Creates a command line that the container manager uses to launch
 * a parallel container.
 */ 
//=============================================================================
string 
SALOME_ContainerManager::BuildCommandToLaunchLocalParallelContainer(const std::string& exe_name,
								    const Engines::MachineParameters& params)
{
  // This method knows the differences between the proxy and the nodes.
  // nb_component_nodes is not used in the same way if it is a proxy or 
  // a node.
  
  //command = "gdb --args ";
  //command = "valgrind --tool=memcheck --log-file=val_log ";
  //command += real_exe_name;

  std::string parallelLib(CORBA::string_dup(params.parallelLib));
  std::string real_exe_name  = exe_name + parallelLib;

  std::string hostname(CORBA::string_dup(params.hostname));

  std::ostringstream tmp_string;
  CORBA::Long nb_nodes = params.nb_component_nodes;
  tmp_string << nb_nodes;
  std::string nbproc = tmp_string.str();
  
  bool is_a_proxy = false; 
  std::string::size_type loc_proxy = exe_name.find("Proxy");
  if( loc_proxy != string::npos ) {
    is_a_proxy = true;
  } 

  Engines::MachineParameters_var rtn = new Engines::MachineParameters();
  rtn->container_name = params.container_name;
  rtn->hostname = params.hostname;
  rtn->OS = params.OS;
  rtn->mem_mb = params.mem_mb;
  rtn->cpu_clock = params.cpu_clock;
  rtn->nb_proc_per_node = params.nb_proc_per_node;
  rtn->nb_node = params.nb_node;
  rtn->isMPI = params.isMPI;

  std::string log_env("");
  char * get_val = getenv("PARALLEL_LOG");
  if (get_val)
    log_env = get_val;
  std::string command_begin("");
  std::string command_end("");
  if(log_env == "xterm")
  {
    command_begin = "/usr/X11R6/bin/xterm -e \"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH; export PATH=$PATH;";
    command_end   = "\"&";
  }
  else if(log_env == "xterm_debug")
  {
    command_begin = "/usr/X11R6/bin/xterm -e \"export LD_LIBRARY_PATH=$LD_LIBRARY_PATH; export PATH=$PATH;";
    command_end   = "; cat \" &";
  }
  else
  {
    // default into a file...
    std::string logFilename = "/tmp/" + _NS->ContainerName(params) + "_" + hostname;
    if (is_a_proxy)
      logFilename += "_Proxy_";
    else
      logFilename += "_Node_";
    logFilename += std::string(getenv("USER")) + ".log";
    command_end = " > " + logFilename + " 2>&1 & ";
  }

  std::string command("");
  if (parallelLib == "Dummy")
  {
    if (is_a_proxy)
    {
      command =  real_exe_name;
      command += " " + _NS->ContainerName(rtn);
      command += " " + parallelLib;
      command += " " + hostname;
      command += " " + nbproc;
      command += " -";
      AddOmninamesParams(command);

      command = command_begin + command + command_end;
    }
    else
    {
      for (int i= 0; i < nb_nodes; i++)
      {
	std::ostringstream tmp;
	tmp << i;
	std::string proc_number = tmp.str();

	std::string command_tmp("");
	command_tmp += real_exe_name;
	command_tmp += " " + _NS->ContainerName(rtn);
	command_tmp += " " + parallelLib;
	command_tmp += " " + hostname;
	command_tmp += " " + proc_number;
	command_tmp += " -";
	AddOmninamesParams(command_tmp);

	// On change _Node_ par _Nodex_ pour avoir chaque noeud
	// sur un fichier
	std::string command_end_tmp = command_end;
	std::string::size_type loc_node = command_end_tmp.find("_Node_");
	if (loc_node != std::string::npos)
	  command_end_tmp.insert(loc_node+5, proc_number);
	command += command_begin + command_tmp + command_end_tmp;
      }
    }
  }
  else if (parallelLib == "Mpi")
  {
    // Step 1 : check if MPI is started
    // Required for lam -> lamboot
    if (_MpiStarted == false)
    {
      startMPI();
    }

    if (is_a_proxy)
    {
      command = "mpirun -np 1 ";
      command += real_exe_name;
      command += " " + _NS->ContainerName(rtn);
      command += " " + nbproc;
      command += " " + parallelLib;
      command += " " + hostname;
      command += " -";
      AddOmninamesParams(command);

      command = command_begin + command + command_end;
    }
    else                                          
    {
      command = "mpirun -np " + nbproc + " ";
      command += real_exe_name;
      command += " " + _NS->ContainerName(rtn);
      command += " " + parallelLib;
      command += " " + hostname;
      command += " -";
      AddOmninamesParams(command);

      command = command_begin + command + command_end;
    }
  }
  else
  {
    std::string message("Unknown parallelLib : " + parallelLib);
    throw SALOME_Exception(message.c_str());
  }

  MESSAGE("Parallel launch is: " << command);
  return command;
}

void SALOME_ContainerManager::startMPI()
{
  cerr << "----------------------------------------------" << endl;
  cerr << "----------------------------------------------" << endl;
  cerr << "----------------------------------------------" << endl;
  cerr << "-Only Lam on Localhost is currently supported-" << endl;
  cerr << "----------------------------------------------" << endl;
  cerr << "----------------------------------------------" << endl;
  cerr << "----------------------------------------------" << endl;

  int status = system("lamboot");
  if (status == -1)
  {
    INFOS("lamboot failed : system command status -1");
    _MpiStarted = true;
  }
  else if (status == 217)
  {
    INFOS("lamboot failed : system command status 217");
    _MpiStarted = true;
  }
  else
    {
      _MpiStarted = true;
    }
}

string SALOME_ContainerManager::GetMPIZeroNode(string machine)
{
  int status;
  string zeronode;
  string cmd;
  string tmpFile = BuildTemporaryFileName();

  cmd = "ssh " + machine + " mpirun -np 1 hostname > " + tmpFile;

  status = system(cmd.c_str());
  if( status == 0 ){
    ifstream fp(tmpFile.c_str(),ios::in);
    fp >> zeronode;
  }

  RmTmpFile(tmpFile);

  return zeronode;
}
