//  File   : SALOMEDS_GenericAttribute.cxx
//  Author : Sergey RUIN
//  Module : SALOME

using namespace std; 

#include <TCollection_AsciiString.hxx>

#include "SALOMEDS_GenericAttribute.hxx"
#include "SALOMEDSImpl_SObject.hxx"
#include "SALOMEDS_SObject.hxx"
#include "SALOMEDS_ClientAttributes.hxx"

#ifdef WNT
#include <process.h>
#else
#include <sys/types.h>
#include <unistd.h>
#endif

#include "OpUtil.hxx"

SALOMEDS_GenericAttribute::SALOMEDS_GenericAttribute(const Handle(SALOMEDSImpl_GenericAttribute)& theGA)
{
  _isLocal = true;
  _local_impl = theGA;
  _corba_impl = SALOMEDS::GenericAttribute::_nil();
}

SALOMEDS_GenericAttribute::SALOMEDS_GenericAttribute(SALOMEDS::GenericAttribute_ptr theGA)
{
#ifdef WNT
  long pid =  (long)_getpid();
#else
  long pid =  (long)getpid();
#endif  

  long addr = theGA->GetLocalImpl(GetHostname().c_str(), pid, _isLocal);
  if(_isLocal) {
    _local_impl = ((SALOMEDSImpl_GenericAttribute*)(addr));
    _corba_impl = SALOMEDS::GenericAttribute::_nil();
  }
  else {
    _local_impl = NULL;
    _corba_impl = SALOMEDS::GenericAttribute::_duplicate(theGA);
  }
}

SALOMEDS_GenericAttribute::~SALOMEDS_GenericAttribute() 
{
  if(!_isLocal) CORBA::release(_corba_impl);
}

void SALOMEDS_GenericAttribute::CheckLocked() 
{
  if(_isLocal) {
    try {
      _local_impl->CheckLocked();
    }
    catch(...) {
      throw SALOMEDS::GenericAttribute::LockProtection();
    }
  }
  else {
    _corba_impl->CheckLocked();
  }
}

char* SALOMEDS_GenericAttribute::Type()
{
  TCollection_AsciiString aType;
  if(_isLocal) {
    aType = _local_impl->Type();
  }
  else {
    aType = _corba_impl->Type();
  }
  return aType.ToCString();
}

char* SALOMEDS_GenericAttribute::GetClassType()
{
  TCollection_AsciiString aType;
  if(_isLocal) {
    aType = _local_impl->GetClassType();
  }
  else {
    aType = _corba_impl->GetClassType();
  }
  return aType.ToCString();
}

SALOMEDSClient_SObject* SALOMEDS_GenericAttribute::GetSObject()
{
  SALOMEDSClient_SObject* aSO = NULL;
  if(_isLocal) {
    aSO = new SALOMEDS_SObject(_local_impl->GetSObject());
  }
  else {
    aSO = new SALOMEDS_SObject(_corba_impl->GetSObject());
  }

  return aSO;
}


SALOMEDS_GenericAttribute* SALOMEDS_GenericAttribute::CreateAttribute(const Handle(SALOMEDSImpl_GenericAttribute)& theGA)
{
  SALOMEDS_GenericAttribute* aGA = NULL;
  char* aTypeOfAttribute = theGA->GetClassType().ToCString();
  __CreateGenericClientAttributeLocal
  return aGA;  
}

SALOMEDS_GenericAttribute* SALOMEDS_GenericAttribute::CreateAttribute(SALOMEDS::GenericAttribute_ptr theGA)
{
  SALOMEDS_GenericAttribute* aGA = NULL;
  char* aTypeOfAttribute = theGA->GetClassType();
  __CreateGenericClientAttributeCORBA
  return aGA;  
}

