// Copyright (C) 2021  CEA/DEN, EDF R&D
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

%module KernelServices

%include "std_string.i"

%{
#include "KernelServices.hxx"
%}

%inline
{
    void RegisterCompoInternal(const std::string& compoName, const std::string& compoIOR);
    std::string RetrieveCompoInternal(const std::string& compoName);
}

%pythoncode %{
def RegisterCompo(compoName,compoRef):
  import CORBA
  orb=CORBA.ORB_init([''])
  RegisterCompoInternal(compoName,orb.object_to_string(compoRef))

def RetrieveCompo(compoName):
  import CORBA
  orb=CORBA.ORB_init([''])
  return orb.string_to_object(RetrieveCompoInternal(compoName))
%}
