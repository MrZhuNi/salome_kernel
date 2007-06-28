//  SALOME ParallelContainerNodeMpi : Launch mpi PaCO++ object nodes
//
//  Copyright (C) 2007 OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  File   : SALOME_ParallelContainerNodeMpi.cxx
//  Author : Andr� Ribes, EDF
//  Module : SALOME PARALLEL

#include <iostream>
#include <string>
#include <stdio.h>

#ifndef WNT
#include <unistd.h>
#else
#include <process.h>
#endif

#include "SALOME_ParallelContainer_i.hxx"

// PaCO++ include
#include <paco_omni.h>
#include <paco_mpi.h>

#include <mpi.h>

#include "SALOME_NamingService.hxx"

#include "utilities.h"
#include "Utils_ORB_INIT.hxx"
#include "Utils_SINGLETON.hxx"
#include "SALOMETraceCollector.hxx"
#include "OpUtil.hxx"

using namespace std;

#ifdef _DEBUG_
#include <signal.h>

void test(int sigval) {
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	cerr << "SIGSEGV in :" << getpid() << endl;
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	while (1) {}
}

void handler(int t) {
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	cerr << "SIGSEGV in :" << getpid() << endl;
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	cerr << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
	while (1) {}
}
#endif

int main(int argc, char* argv[])
{
	INFOS("Launching a parallel Mpi container node");

#ifdef _DEBUG_
	struct sigaction action;
	action.sa_handler = &test;
	sigaction(SIGSEGV, &action, NULL);
#endif
	
	// MPI Init
	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE ,&provided);

#ifdef _DEBUG_
	cerr << "Level MPI_THREAD_SINGLE : " << MPI_THREAD_SINGLE << endl;
	cerr << "Level MPI_THREAD_SERIALIZED : " << MPI_THREAD_SERIALIZED << endl;
	cerr << "Level MPI_THREAD_FUNNELED : " << MPI_THREAD_FUNNELED << endl;
	cerr << "Level MPI_THREAD_MULTIPLE : " << MPI_THREAD_MULTIPLE << endl;
	cerr << "Level provided : " << provided << endl;
#endif
	// Initialise the ORB.
	ORB_INIT &init = *SINGLETON_<ORB_INIT>::Instance();
	ASSERT(SINGLETON_<ORB_INIT>::IsAlreadyExisting());
	CORBA::ORB_var orb = init(0, 0);
	//CORBA::ORB_var orb = CORBA::ORB_init(argc, argv);

	// Code pour choisir le reseau infiniband .....
/*	string hostname_temp = GetHostname();
	hostent * t = gethostbyname(hostname_temp.c_str());
	cerr << " AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA " << t->h_addr << " " << hostname_temp << endl;
	cerr << t->h_addr << endl;
	in_addr * address=(in_addr * ) t->h_addr;
	cerr << inet_ntoa(* address) << endl;
	string ip = inet_ntoa(* address);
	cerr << " AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA " << endl;
	string com = "giop:tcp:" + ip + ":";
	const char* options[][2] = { { "endPoint", com.c_str() }, { 0, 0 } };
	CORBA::ORB_var orb = CORBA::ORB_init(argc, argv, "omniORB4", options);
*/
	char * containerName = "";
	if(argc > 1) {
		containerName = argv[1];
	}

	char * hostname = "";
	if(argc > 3) {
		hostname = argv[3];
	}

	try {  
		CORBA::Object_var obj = orb->resolve_initial_references("RootPOA");
		PortableServer::POA_var root_poa = PortableServer::POA::_narrow(obj);
		PortableServer::POAManager_var pman = root_poa->the_POAManager();

#ifndef WNT
		// add this container to the kill list
		char aCommand[100];
		sprintf(aCommand, "addToKillList.py %d SALOME_ParallelContainerNodeMpi", getpid());
		system(aCommand);
#endif

		SALOME_NamingService * ns = new SALOME_NamingService(CORBA::ORB::_duplicate(orb));
		// On r�cup�re le proxy 
		string proxyNameInNS = ns->BuildContainerNameForNS(containerName, hostname);
		obj = ns->Resolve(proxyNameInNS.c_str());
		char * proxy_ior = orb->object_to_string(obj);

		// Node creation
		string name(containerName);
		string node_name = name + "Node";
		Engines_Parallel_Container_i * servant = new Engines_Parallel_Container_i(CORBA::ORB::_duplicate(orb), proxy_ior,
																																							root_poa,
																																							(char*) node_name.c_str(),
																																							argc, argv);
		// PaCO++ init
		paco_fabrique_manager * pfm = paco_getFabriqueManager();
		pfm->register_com("mpi", new paco_mpi_fabrique());
		pfm->register_thread("omni", new paco_omni_fabrique());

		// Global context
		PaCO_operation * global_ptr = servant->getContext("global_paco_context");
		MPI_Comm group = MPI_COMM_WORLD;
		global_ptr->setLibCom("mpi", &group);
		global_ptr->setLibThread("omni");

		// Activation
		PortableServer::ObjectId * _id = root_poa->activate_object(servant);
		servant->set_id(_id);
		obj = root_poa->id_to_reference(*_id);

		// In the NamingService
		string hostname = GetHostname();

		int myid;
		MPI_Comm_rank(MPI_COMM_WORLD, &myid);
		char buffer [5];
		snprintf(buffer, 5, "%d", myid);
		node_name = node_name + buffer;
		string _containerName = ns->BuildContainerNameForNS((char*) node_name.c_str(),
																												hostname.c_str());
		cerr << "---------" << _containerName << "----------" << endl;
		ns->Register(obj, _containerName.c_str());
		pman->activate();
		orb->run();
	}
	catch(CORBA::SystemException&)
	{
		INFOS("Caught CORBA::SystemException.");
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

	MPI_Finalize();

	return 0 ;
}

