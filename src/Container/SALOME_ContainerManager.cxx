// Copyright (C) 2005  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
// 
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either 
// version 2.1 of the License.
// 
// This library is distributed in the hope that it will be useful 
// but WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public  
// License along with this library; if not, write to the Free Software 
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
#include "SALOME_ContainerManager.hxx"
#include "SALOME_NamingService.hxx"
#include "OpUtil.hxx"
#include <sys/types.h>
#ifndef WNT
#include <unistd.h>
#endif
#include <vector>
#include "Utils_CorbaException.hxx"

#ifdef WITH_PACO_PARALLEL
#include "PaCO++.h"
#endif

#define TIME_OUT_TO_LAUNCH_CONT 21

using namespace std;

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

SALOME_ContainerManager::SALOME_ContainerManager(CORBA::ORB_ptr orb)
{
  MESSAGE("constructor");
  _NS = new SALOME_NamingService(orb);
  _ResManager = new SALOME_ResourcesManager(orb);
  _id=0;
  PortableServer::POA_var root_poa = PortableServer::POA::_the_root_poa();
  PortableServer::POAManager_var pman = root_poa->the_POAManager();
  PortableServer::POA_var my_poa;

  CORBA::PolicyList policies;
  policies.length(1);
  PortableServer::ThreadPolicy_var threadPol = 
    root_poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL);
  policies[0] = PortableServer::ThreadPolicy::_duplicate(threadPol);

  my_poa = 
    root_poa->create_POA("SThreadPOA",pman,policies);
  threadPol->destroy();
  PortableServer::ObjectId_var id = my_poa->activate_object(this);
  CORBA::Object_var obj = my_poa->id_to_reference(id);
  Engines::ContainerManager_var refContMan =
    Engines::ContainerManager::_narrow(obj);

  _NS->Register(refContMan,_ContainerManagerNameInNS);
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
  delete _NS;
  delete _ResManager;
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
  PortableServer::ObjectId_var oid = _default_POA()->servant_to_id(this);
  _default_POA()->deactivate_object(oid);
  _remove_ref();
  
}

//=============================================================================
/*! CORBA Method:
 *  Loop on all the containers listed in naming service, ask shutdown on each
 */
//=============================================================================

void SALOME_ContainerManager::ShutdownContainers()
{
  MESSAGE("ShutdownContainers");
  _NS->Change_Directory("/Containers");
  vector<string> vec = _NS->list_directory_recurs();
  list<string> lstCont;
  for(vector<string>::iterator iter = vec.begin();iter!=vec.end();iter++)
    {
      SCRUTE((*iter));
      CORBA::Object_var obj=_NS->Resolve((*iter).c_str());
      Engines::Container_var cont=Engines::Container::_narrow(obj);
      if(!CORBA::is_nil(cont))
	{
	  lstCont.push_back((*iter));
	}
    }
  MESSAGE("Container list: ");
  for(list<string>::iterator iter=lstCont.begin();iter!=lstCont.end();iter++)
    {
      SCRUTE((*iter));
    }
  for(list<string>::iterator iter=lstCont.begin();iter!=lstCont.end();iter++)
    {
      SCRUTE((*iter));
      CORBA::Object_var obj=_NS->Resolve((*iter).c_str());
      Engines::Container_var cont=Engines::Container::_narrow(obj);
      if(!CORBA::is_nil(cont))
	{
	  MESSAGE("ShutdownContainers: " << (*iter));
	  cont->Shutdown();
	}
      else MESSAGE("ShutdownContainers: no container ref for " << (*iter));
    }
}

//=============================================================================
/*! CORBA Method:
 *  Find a suitable Container in a list of machines, or start one
 *  \param params            Machine Parameters required for the container
 *  \param possibleComputers list of machines usable for find or start
 */
//=============================================================================

Engines::Container_ptr
SALOME_ContainerManager::
FindOrStartContainer(const Engines::MachineParameters& params,
		     const Engines::MachineList& possibleComputers)
{
  long id;
  string containerNameInNS;
  char idc[3*sizeof(long)];

  Engines::Container_ptr ret = FindContainer(params,possibleComputers);
  if(!CORBA::is_nil(ret))
    return ret;
  MESSAGE("Container doesn't exist try to launch it ...");
  MESSAGE("SALOME_ContainerManager::FindOrStartContainer " <<
	  possibleComputers.length());
  //vector<string> vector;
  string theMachine=_ResManager->FindBest(possibleComputers);
  MESSAGE("try to launch it on " << theMachine);

  // Get Id for container: a parallel container registers in Naming Service
  // on the machine where is process 0. ContainerManager does'nt know the name
  // of this machine before the launch of the parallel container. So to get
  // the IOR of the parallel container in Naming Service, ContainerManager
  // gives a unique Id. The parallel container registers his name under
  // /ContainerManager/Id directory in NamingService

  id = GetIdForContainer();

  string command;
  if(theMachine=="")
    {
      MESSAGE("SALOME_ContainerManager::FindOrStartContainer : " <<
	      "no possible computer");
      return Engines::Container::_nil();
    }
  else if(theMachine==GetHostname())
    {
      command=_ResManager->BuildCommandToLaunchLocalContainer(params,id);
    }
  else
    command =
      _ResManager->BuildCommandToLaunchRemoteContainer(theMachine,params,id);

  _ResManager->RmTmpFile();
  int status=system(command.c_str());
  if (status == -1)
    {
      MESSAGE("SALOME_LifeCycleCORBA::StartOrFindContainer rsh failed " <<
	      "(system command status -1)");
      return Engines::Container::_nil();
    }
  else if (status == 217)
    {
      MESSAGE("SALOME_LifeCycleCORBA::StartOrFindContainer rsh failed " <<
	      "(system command status 217)");
      return Engines::Container::_nil();
    }
  else
    {
      int count=TIME_OUT_TO_LAUNCH_CONT;
      while ( CORBA::is_nil(ret) && count )
	{
#ifndef WNT
	  sleep( 1 ) ;
#else
	  Sleep(1000);
#endif
	  count-- ;
	  if ( count != 10 )
	    MESSAGE( count << ". Waiting for FactoryServer on " << theMachine);
	  if(params.isMPI)
	    {
	      containerNameInNS = "/ContainerManager/id";
	      sprintf(idc,"%ld",id);
	      containerNameInNS += idc;
	    }
	  else
	    containerNameInNS =
	      _NS->BuildContainerNameForNS(params,theMachine.c_str());
	  SCRUTE(containerNameInNS);
	  CORBA::Object_var obj = _NS->Resolve(containerNameInNS.c_str());
	  ret=Engines::Container::_narrow(obj);
	}
      if ( CORBA::is_nil(ret) )
	{
	  MESSAGE("SALOME_LifeCycleCORBA::StartOrFindContainer rsh failed");
	}
      return ret;
    }
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
FindOrStartParallelContainer(const Engines::MachineParameters& params_const,
			     const Engines::MachineList& possibleComputers)
{
  CORBA::Object_var obj;
  Engines::Container_ptr ret = Engines::Container::_nil();
  Engines::MachineParameters params(params_const);

  // Step 1 : Try to find a suitable container
  // Currently not as good as could be since
  // we have to verified the number of nodes of the container
  // if a user tell that.
  ret = FindContainer(params, possibleComputers);

  if(CORBA::is_nil(ret)) {
    // Step 2 : Starting a new parallel container
    INFOS("[FindOrStartParallelContainer] Starting a parallel container");
    
    // Step 2.1 : Choose a computer
    string theMachine = _ResManager->FindBest(possibleComputers);
    if(theMachine == "") {
      INFOS("[FindOrStartParallelContainer] !!!!!!!!!!!!!!!!!!!!!!!!!!");
      INFOS("[FindOrStartParallelContainer] No possible computer found");
      INFOS("[FindOrStartParallelContainer] !!!!!!!!!!!!!!!!!!!!!!!!!!");
    }
    else {
      INFOS("[FindOrStartParallelContainer] on machine : " << theMachine);
      string command;
      if(theMachine == GetHostname()) {
	// Step 3 : starting parallel container proxy
	params.hostname = CORBA::string_dup(theMachine.c_str());
	Engines::MachineParameters params_proxy(params);
	command = _ResManager->BuildCommandToLaunchLocalParallelContainer("SALOME_ParallelContainerProxy", params_proxy, "xterm");
	// LaunchParallelContainer uses this value to know if it launches the proxy or the nodes
	params_proxy.nb_component_nodes = 0;
	obj = LaunchParallelContainer(command, params_proxy, _NS->ContainerName(params));
	ret = Engines::Container::_narrow(obj);

	// Step 4 : starting parallel container nodes
	command = _ResManager->BuildCommandToLaunchLocalParallelContainer("SALOME_ParallelContainerNode", params, "xterm");
	string name = _NS->ContainerName(params) + "Node";
	LaunchParallelContainer(command, params, name);

	// Step 5 : connecting nodes and the proxy to actually create a parallel container
	try {
	for (int i = 0; i < params.nb_component_nodes; i++) {

	char buffer [5];
	snprintf(buffer,5,"%d",i);
	string name_cont = name + string(buffer);

	string theNodeMachine(CORBA::string_dup(params.hostname));
	string containerNameInNS = _NS->BuildContainerNameForNS(name_cont.c_str(),theNodeMachine.c_str());
	int count = TIME_OUT_TO_LAUNCH_CONT;
	obj = _NS->Resolve(containerNameInNS.c_str());
	while (CORBA::is_nil(obj) && count) {
	  INFOS("[FindOrStartParallelContainer] CONNECTION FAILED !!!!!!!!!!!!!!!!!!!!!!!!");
#ifndef WNT
	  sleep(1) ;
#else
	  Sleep(1000);
#endif
	  count-- ;
	  obj = _NS->Resolve(containerNameInNS.c_str());
	}

	PaCO::InterfaceParallel_var node = PaCO::InterfaceParallel::_narrow(obj);
	MESSAGE("[FindOrStartParallelContainer] Deploying node : " << name);
	node->deploy(i);
	}
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
	INFOS("[FindOrStartParallelContainer] node " << name << " deployed");
      }

      else {
	INFOS("[FindOrStartParallelContainer] Currently parallel containers are launched only on the local host");
      }
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
FindOrStartParallelContainer(const Engines::MachineParameters& params,
			     const Engines::MachineList& possibleComputers)
{
  Engines::Container_ptr ret = Engines::Container::_nil();
  INFOS("[FindOrStartParallelContainer] is disabled !");
  INFOS("[FindOrStartParallelContainer] recompile SALOME Kernel to enable parallel extension");
  return ref;
}
#endif

//=============================================================================
/*! 
 * 
 */
//=============================================================================

Engines::MachineList *
SALOME_ContainerManager::
GetFittingResources(const Engines::MachineParameters& params,
		    const char *componentName)
{
  MESSAGE("SALOME_ContainerManager::GetFittingResources");
  Engines::MachineList *ret=new Engines::MachineList;
  vector<string> vec;
  try
    {
      vec = _ResManager->GetFittingResources(params,componentName);
    }
  catch(const SALOME_Exception &ex)
    {
      INFOS("Caught exception.");
      THROW_SALOME_CORBA_EXCEPTION(ex.what(),SALOME::BAD_PARAM);
      //return ret;
    }

  //  MESSAGE("Machine list length "<<vec.size());
  ret->length(vec.size());
  for(unsigned int i=0;i<vec.size();i++)
    {
      (*ret)[i]=(vec[i]).c_str();
    }
  return ret;
}

//=============================================================================
/*! 
 * 
 */
//=============================================================================

char*
SALOME_ContainerManager::
FindBest(const Engines::MachineList& possibleComputers)
{
  string theMachine=_ResManager->FindBest(possibleComputers);
  return CORBA::string_dup(theMachine.c_str());
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
  string containerNameInNS;

  if (params.nb_component_nodes == 0) {
    INFOS("[LaunchParallelContainer] launching the proxy of the parallel container");
    int status = system(command.c_str());
    if (status == -1) {
      INFOS("[LaunchParallelContainer] failed : system command status -1");
    }
    else if (status == 217) {
      INFOS("[LaunchParallelContainer] failed : system command status 217");
    }

    int count = TIME_OUT_TO_LAUNCH_CONT;
    string theMachine(CORBA::string_dup(params.hostname));
    containerNameInNS = _NS->BuildContainerNameForNS((char*) name.c_str(),theMachine.c_str());

    INFOS("[LaunchContainer]  Waiting for Parallel Container proxy on " << theMachine);
    while (CORBA::is_nil(obj) && count) {
#ifndef WNT
      sleep(1) ;
#else
      Sleep(1000);
#endif
      count-- ;
      obj = _NS->Resolve(containerNameInNS.c_str());
    }
  }
  else {
    INFOS("[LaunchParallelContainer] launching the nodes of the parallel container");
    int status = system(command.c_str());
    if (status == -1) {
      INFOS("[LaunchParallelContainer] failed : system command status -1");
    }
    else if (status == 217) {
      INFOS("[LaunchParallelContainer] failed : system command status 217");
    }
    // We are waiting all the nodes
    for (int i = 0; i < params.nb_component_nodes; i++) {
      obj = CORBA::Object::_nil();
      int count = TIME_OUT_TO_LAUNCH_CONT;

      // Name of the node
      char buffer [5];
      snprintf(buffer,5,"%d",i);
      string name_cont = name + string(buffer);

      // I don't like this...
      string theMachine(CORBA::string_dup(params.hostname));
      containerNameInNS = _NS->BuildContainerNameForNS((char*) name_cont.c_str(),theMachine.c_str());
      cerr << "[LaunchContainer]  Waiting for Parllel Container node " << containerNameInNS << " on " << theMachine << endl;
      while (CORBA::is_nil(obj) && count) {
#ifndef WNT
	sleep(1) ;
#else
	Sleep(1000);
#endif
	count-- ;
	obj = _NS->Resolve(containerNameInNS.c_str());
      }
    }
  }

  if ( CORBA::is_nil(obj) ) {
    INFOS("[LaunchParallelContainer] failed");
  }
  return obj;
}

//=============================================================================
/*! 
 * Get Id for container: a parallel container registers in Naming Service
 * on the machine where is process 0. ContainerManager does'nt know the name
 * of this machine before the launch of the parallel container. So to get
 * the IOR of the parallel container in Naming Service, ContainerManager
 * gives a unique Id. The parallel container registers his name under
 * /ContainerManager/Id directory in NamingService
 */
//=============================================================================


long SALOME_ContainerManager::GetIdForContainer(void)
{
  _id++;
  return _id;
}

