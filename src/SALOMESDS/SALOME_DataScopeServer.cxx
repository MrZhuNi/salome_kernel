// Copyright (C) 2007-2014  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
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
// Author : Anthony GEAY (EDF R&D)

#include "SALOMESDS_DataScopeServer.hxx"
#include "SALOMESDS_DataServerManager.hxx"
#include "SALOMESDS_Exception.hxx"

#include "SALOME_NamingService.hxx"

#include <string>
#include <sstream>

int main(int argc, char* argv[])
{
  std::string scopeName;
  if(argc<=1)
    throw SALOMESDS::Exception("In the main of SALOME_DataScopeServer.cxx !");
  scopeName=argv[1];
  CORBA::ORB_var orb(CORBA::ORB_init(argc,argv));
  CORBA::Object_var obj(orb->resolve_initial_references("RootPOA"));
  PortableServer::POA_var poa(PortableServer::POA::_narrow(obj));
  PortableServer::POAManager_var mgr(poa->the_POAManager());
  mgr->activate();
  SALOMESDS::DataScopeServer *server(new SALOMESDS::DataScopeServer(orb,scopeName));
  //
  CORBA::PolicyList policies;
  policies.length(1);
  PortableServer::ThreadPolicy_var threadPol(poa->create_thread_policy(PortableServer::SINGLE_THREAD_MODEL));
  policies[0]=PortableServer::ThreadPolicy::_duplicate(threadPol);
  PortableServer::POA_var poa2(poa->create_POA("SingleThPOA4SDS",mgr,policies));
  threadPol->destroy();
  PortableServer::ObjectId_var id(poa2->activate_object(server));
  obj=poa2->id_to_reference(id);
  SALOME::DataScopeServer_var serverPtr(SALOME::DataScopeServer::_narrow(obj));
  //
  std::string fullScopeName(SALOMESDS::DataServerManager::CreateAbsNameInNSFromScopeName(scopeName));
  SALOME_NamingService ns(orb);
  ns.Register(serverPtr,fullScopeName.c_str());
  //
  orb->run();
  server->decrRef();
  return 0;
}

