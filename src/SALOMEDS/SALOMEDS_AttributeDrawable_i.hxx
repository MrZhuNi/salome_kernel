// Copyright (C) 2007-2014  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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

//  File   : SALOMEDS_AttributeDrawable_i.hxx
//  Author : Sergey RUIN
//  Module : SALOME
//
#ifndef SALOMEDS_AttributeDrawable_i_HeaderFile
#define SALOMEDS_AttributeDrawable_i_HeaderFile

// IDL headers

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SALOMEDS_Attributes)

#include "SALOMEDS_GenericAttribute_i.hxx"
#include "SALOMEDSImpl_AttributeDrawable.hxx"

class SALOMEDS_AttributeDrawable_i: public virtual POA_SALOMEDS::AttributeDrawable,
                                    public virtual SALOMEDS_GenericAttribute_i 
{
public:
  
  SALOMEDS_AttributeDrawable_i(SALOMEDSImpl_AttributeDrawable* theAttr, CORBA::ORB_ptr orb)
    :SALOMEDS_GenericAttribute_i(theAttr, orb) {}; 

  virtual ~SALOMEDS_AttributeDrawable_i() {};
  CORBA::Boolean IsDrawable();
  void SetDrawable(CORBA::Boolean value);
};



#endif
