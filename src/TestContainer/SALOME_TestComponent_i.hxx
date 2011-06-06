// Copyright (C) 2007-2011  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
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

//  SALOME TestContainer : test of container creation and its life cycle
//  File   : SALOME_TestComponent_i.hxx
//  Author : Paul RASCLE, EDF - MARC TAJCHMAN, CEA
//  Module : SALOME
//  $Header$
//
#ifndef _SALOME_TESTCOMPONENT_I_HXX_
#define _SALOME_TESTCOMPONENT_I_HXX_

#include "SALOME_TestComponent.hxx"

#include <iostream>

#include "SALOME_Component_i.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SALOME_TestComponent)

class TESTCONTAINER_EXPORT Engines_TestComponent_i: 
  public POA_Engines::TestComponent,
  public Engines_Component_i 
{
public:
  Engines_TestComponent_i();
  Engines_TestComponent_i(CORBA::ORB_ptr orb,
                          PortableServer::POA_ptr poa,
                          PortableServer::ObjectId * contId, 
                          const char *instanceName, 
                          const char *interfaceName);

  virtual ~Engines_TestComponent_i();

  char* Coucou(CORBA::Long L);
  void Setenv();
  
private:

};

#endif
