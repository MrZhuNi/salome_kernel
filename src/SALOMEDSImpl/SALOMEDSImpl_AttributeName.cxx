//  File   : SALOMEDSImpl_AttributeName.cxx
//  Author : Sergey RUIN
//  Module : SALOME

using namespace std;
#include "SALOMEDSImpl_AttributeName.hxx"

IMPLEMENT_STANDARD_HANDLE( SALOMEDSImpl_AttributeName, SALOMEDSImpl_GenericAttribute )
IMPLEMENT_STANDARD_RTTIEXT( SALOMEDSImpl_AttributeName, SALOMEDSImpl_GenericAttribute )

//=======================================================================
//function : GetID
//purpose  :
//=======================================================================
const Standard_GUID& SALOMEDSImpl_AttributeName::GetID ()
{
  static Standard_GUID NameID ("8650000D-63A0-4651-B621-CC95C9308598");
  return NameID;
}   

Handle(SALOMEDSImpl_AttributeName) SALOMEDSImpl_AttributeName::Set (const TDF_Label& L, 
								    const TCollection_ExtendedString& Val) 
{
  Handle(SALOMEDSImpl_AttributeName) A;
  if (!L.FindAttribute(SALOMEDSImpl_AttributeName::GetID(), A)) {
    A = new  SALOMEDSImpl_AttributeName(); 
    L.AddAttribute(A);
  }

  A->Set(Val);   
  return A;
}

//=======================================================================
//function : Set
//purpose  :
//=======================================================================
void SALOMEDSImpl_AttributeName::Set (const TCollection_ExtendedString& S)
{
  CheckLocked();

  if(myString == S) return;

  Backup();

  myString = S;
}
           

//=======================================================================
//function : ID
//purpose  :
//=======================================================================
const Standard_GUID& SALOMEDSImpl_AttributeName::ID () const { return GetID(); } 

//=======================================================================
//function : NewEmpty
//purpose  : 
//=======================================================================
Handle(TDF_Attribute) SALOMEDSImpl_AttributeName::NewEmpty () const
{  
  return new SALOMEDSImpl_AttributeName(); 
}

//=======================================================================
//function : Restore
//purpose  : 
//=======================================================================
void SALOMEDSImpl_AttributeName::Restore(const Handle(TDF_Attribute)& with) 
{
  myString = Handle(SALOMEDSImpl_AttributeName)::DownCast (with)->Get ();
}

//=======================================================================
//function : Paste
//purpose  : 
//=======================================================================

void SALOMEDSImpl_AttributeName::Paste (const Handle(TDF_Attribute)& into,
					   const Handle(TDF_RelocationTable)& RT) const
{
  Handle(SALOMEDSImpl_AttributeName)::DownCast (into)->Set(myString);
}
