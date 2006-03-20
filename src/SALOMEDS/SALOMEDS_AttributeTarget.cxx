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
// See http://www.salome-platform.org/
//
//  File   : SALOMEDS_AttributeTarget.cxx
//  Author : Sergey RUIN
//  Module : SALOME

#include "SALOMEDS_AttributeTarget.hxx"
#include "SALOMEDS.hxx"

#include <TColStd_HSequenceOfTransient.hxx>

#include "SALOMEDSImpl_SObject.hxx"
#include "SALOMEDS_SObject.hxx"


SALOMEDS_AttributeTarget::SALOMEDS_AttributeTarget(const Handle(SALOMEDSImpl_AttributeTarget)& theAttr)
:SALOMEDS_GenericAttribute(theAttr)
{}

SALOMEDS_AttributeTarget::SALOMEDS_AttributeTarget(SALOMEDS::AttributeTarget_ptr theAttr)
:SALOMEDS_GenericAttribute(theAttr)
{}

SALOMEDS_AttributeTarget::~SALOMEDS_AttributeTarget()
{}

void SALOMEDS_AttributeTarget::Add(const _PTR(SObject)& theObject)
{
  SALOMEDS_SObject* aSO = dynamic_cast<SALOMEDS_SObject*>(theObject.get());

  if (_isLocal) {
    SALOMEDS::Locker lock;
    Handle(SALOMEDSImpl_AttributeTarget)::DownCast(_local_impl)->Add(aSO->GetLocalImpl());
  }
  else SALOMEDS::AttributeTarget::_narrow(_corba_impl)->Add(aSO->GetCORBAImpl());
}

std::vector<_PTR(SObject)> SALOMEDS_AttributeTarget::Get()
{
  std::vector<_PTR(SObject)> aVector;
  int aLength, i;
  SALOMEDSClient_SObject* aSO = NULL;
  
  if (_isLocal) {
    SALOMEDS::Locker lock;
    Handle(TColStd_HSequenceOfTransient) aSeq =
      Handle(SALOMEDSImpl_AttributeTarget)::DownCast(_local_impl)->Get();
    aLength = aSeq->Length();
    for (i = 1; i <= aLength; i++) {
      aSO = new SALOMEDS_SObject(Handle(SALOMEDSImpl_SObject)::DownCast(aSeq->Value(i)));
      aVector.push_back(_PTR(SObject)(aSO));
    }
  }
  else {
    SALOMEDS::Study::ListOfSObject_var aSeq = SALOMEDS::AttributeTarget::_narrow(_corba_impl)->Get();
    aLength = aSeq->length();
	for (i = 0; i < aLength; i++) {
	  aSO = new SALOMEDS_SObject(aSeq[i].in());
	  aVector.push_back(_PTR(SObject)(aSO));
	}
  }

  return aVector;
}

void SALOMEDS_AttributeTarget::Remove(const _PTR(SObject)& theObject)
{
  SALOMEDS_SObject* aSO = dynamic_cast<SALOMEDS_SObject*>(theObject.get());

  if (_isLocal) {
    SALOMEDS::Locker lock;
    Handle(SALOMEDSImpl_AttributeTarget)::DownCast(_local_impl)->Remove(aSO->GetLocalImpl());
  }
  else SALOMEDS::AttributeTarget::_narrow(_corba_impl)->Remove(aSO->GetCORBAImpl());
}
