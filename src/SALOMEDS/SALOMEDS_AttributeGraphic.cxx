//  Copyright (C) 2007-2011  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  File   : SALOMEDS_AttributeGraphic.cxx
//  Author : Sergey RUIN
//  Module : SALOME
//
#include "SALOMEDS_AttributeGraphic.hxx"
#include "SALOMEDS.hxx"

SALOMEDS_AttributeGraphic::SALOMEDS_AttributeGraphic(SALOMEDSImpl_AttributeGraphic* theAttr)
:SALOMEDS_GenericAttribute(theAttr)
{}

SALOMEDS_AttributeGraphic::SALOMEDS_AttributeGraphic(SALOMEDS::AttributeGraphic_ptr theAttr)
:SALOMEDS_GenericAttribute(theAttr)
{}

SALOMEDS_AttributeGraphic::~SALOMEDS_AttributeGraphic()
{}


bool SALOMEDS_AttributeGraphic::GetVisibility(int theViewId)
{
  bool aValue;
  if (_isLocal) {
    SALOMEDS::Locker lock;
    aValue = (bool)dynamic_cast<SALOMEDSImpl_AttributeGraphic*>(_local_impl)->GetVisibility(theViewId);
  }
  else aValue = SALOMEDS::AttributeGraphic::_narrow(_corba_impl)->GetVisibility(theViewId);
  return aValue;
}

void SALOMEDS_AttributeGraphic::SetVisibility(int theViewId, bool theValue)
{
  if (_isLocal) {
    SALOMEDS::Locker lock;
    dynamic_cast<SALOMEDSImpl_AttributeGraphic*>(_local_impl)->SetVisibility(theViewId, theValue);
  }
  else SALOMEDS::AttributeGraphic::_narrow(_corba_impl)->SetVisibility(theViewId, theValue);
}
