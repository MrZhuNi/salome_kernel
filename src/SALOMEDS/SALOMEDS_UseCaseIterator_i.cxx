//  File   : SALOMEDS_UseCaseIterator_i.cxx
//  Author : Sergey RUIN
//  Module : SALOME

using namespace std;
#include "SALOMEDS_UseCaseIterator_i.hxx"
#include "SALOMEDS_SObject_i.hxx"
#include "SALOMEDS.hxx"

#include "SALOMEDSImpl_SObject.hxx"
#include "utilities.h"

//============================================================================
/*! Function : constructor
 *  Purpose  :
 */
//============================================================================
SALOMEDS_UseCaseIterator_i::SALOMEDS_UseCaseIterator_i(const Handle(SALOMEDSImpl_UseCaseIterator)& theImpl, 
						       CORBA::ORB_ptr orb)
{
  _orb = CORBA::ORB::_duplicate(orb);
  _impl = theImpl;
}

//============================================================================
/*! Function : destructor
 *  Purpose  :
 */
//============================================================================
SALOMEDS_UseCaseIterator_i::~SALOMEDS_UseCaseIterator_i()
{
}

//============================================================================
/*! Function :Init
 * 
 */
//============================================================================
void SALOMEDS_UseCaseIterator_i::Init(CORBA::Boolean allLevels)
{ 
  SALOMEDS::Locker lock;
  _impl->Init(allLevels);
}

//============================================================================
/*! Function : More
 * 
 */
//============================================================================
CORBA::Boolean SALOMEDS_UseCaseIterator_i::More()
{
  SALOMEDS::Locker lock;
  return _impl->More();
}

//============================================================================
/*! Function : Next
 * 
 */
//============================================================================
void SALOMEDS_UseCaseIterator_i::Next()
{
  SALOMEDS::Locker lock;
  _impl->Next();
}


//============================================================================
/*! Function :
 *  Purpose  :
 */
//============================================================================
SALOMEDS::SObject_ptr SALOMEDS_UseCaseIterator_i::Value()
{
  SALOMEDS::Locker lock;
  Handle(SALOMEDSImpl_SObject) aSO = _impl->Value();
  SALOMEDS_SObject_i *  so_servant = new SALOMEDS_SObject_i (aSO, _orb);
  SALOMEDS::SObject_var so = SALOMEDS::SObject::_narrow(so_servant->_this());
  return so._retn();
}

