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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  File   : SALOMEDSImpl_AttributeSequenceOfInteger.cxx
//  Author : Sergey RUIN
//  Module : SALOME

#include "SALOMEDSImpl_AttributeSequenceOfInteger.hxx"

using namespace std;

//=======================================================================
//function : GetID
//purpose  : 
//=======================================================================

const std::string& SALOMEDSImpl_AttributeSequenceOfInteger::GetID () 
{
  static std::string SALOMEDSImpl_AttributeSequenceOfIntegerID ("12837182-8F52-11d6-A8A3-0001021E8C7F");
  return SALOMEDSImpl_AttributeSequenceOfIntegerID;
}



//=======================================================================
//function : Set
//purpose  : 
//=======================================================================

SALOMEDSImpl_AttributeSequenceOfInteger* SALOMEDSImpl_AttributeSequenceOfInteger::Set (const DF_Label& L) 
{
  SALOMEDSImpl_AttributeSequenceOfInteger* A = NULL;
  if (!(A = (SALOMEDSImpl_AttributeSequenceOfInteger*)L.FindAttribute(SALOMEDSImpl_AttributeSequenceOfInteger::GetID()))) {
    A = new  SALOMEDSImpl_AttributeSequenceOfInteger(); 
    L.AddAttribute(A);
  }
  return A;
}


//=======================================================================
//function : constructor
//purpose  : 
//=======================================================================
SALOMEDSImpl_AttributeSequenceOfInteger::SALOMEDSImpl_AttributeSequenceOfInteger()
:SALOMEDSImpl_GenericAttribute("AttributeSequenceOfInteger")
{}

//=======================================================================
//function : ID
//purpose  : 
//=======================================================================

const std::string& SALOMEDSImpl_AttributeSequenceOfInteger::ID () const { return GetID(); }


//=======================================================================
//function : NewEmpty
//purpose  : 
//=======================================================================

DF_Attribute* SALOMEDSImpl_AttributeSequenceOfInteger::NewEmpty () const
{  
  return new SALOMEDSImpl_AttributeSequenceOfInteger(); 
}

//=======================================================================
//function : Restore
//purpose  : 
//=======================================================================

void SALOMEDSImpl_AttributeSequenceOfInteger::Restore(DF_Attribute* with) 
{
  SALOMEDSImpl_AttributeSequenceOfInteger* anSeq = dynamic_cast<SALOMEDSImpl_AttributeSequenceOfInteger*>(with);
  myValue.clear();
  for(int i = 0, len = anSeq->Length(); i<len; i++)
    myValue.push_back(anSeq->myValue[i]);    
}

//=======================================================================
//function : Paste
//purpose  : 
//=======================================================================

void SALOMEDSImpl_AttributeSequenceOfInteger::Paste (DF_Attribute* into)
{
  dynamic_cast<SALOMEDSImpl_AttributeSequenceOfInteger*>(into)->Assign(myValue);
}

void SALOMEDSImpl_AttributeSequenceOfInteger::Assign(const vector<int>& other) 
{
  CheckLocked();
  Backup();

  myValue = other;
  
  SetModifyFlag(); //SRN: Mark the study as being modified, so it could be saved 
}

void SALOMEDSImpl_AttributeSequenceOfInteger::ChangeValue(const int Index,const int Value) 
{
  CheckLocked();  
  Backup();

  if(Index <= 0 || Index > myValue.size()) throw DFexception("Out of range");

  myValue[Index-1] = Value;
  
  SetModifyFlag(); //SRN: Mark the study as being modified, so it could be saved 
}

void SALOMEDSImpl_AttributeSequenceOfInteger::Add(const int Value) 
{
  CheckLocked();  
  Backup();
  myValue.push_back(Value);
  
  SetModifyFlag(); //SRN: Mark the study as being modified, so it could be saved 
}

void SALOMEDSImpl_AttributeSequenceOfInteger::Remove(const int Index) 
{
  CheckLocked();  
  Backup();

  if(Index <= 0 || Index > myValue.size()) throw DFexception("Out of range");

  typedef vector<int>::iterator VI;
  int i = 1;    
  for(VI p = myValue.begin(); p!=myValue.end(); p++, i++) {
    if(i == Index) {
      myValue.erase(p);
      break;
    }     
  }

  SetModifyFlag(); //SRN: Mark the study as being modified, so it could be saved 
}

int SALOMEDSImpl_AttributeSequenceOfInteger::Length() 
{
  return myValue.size();
}
int SALOMEDSImpl_AttributeSequenceOfInteger::Value(const int Index) 
{
  if(Index <= 0 || Index > myValue.size()) throw DFexception("Out of range");

  return myValue[Index-1];
}



string SALOMEDSImpl_AttributeSequenceOfInteger::Save() 
{
  int aLength = Length();
  char* aResult = new char[aLength * 25];
  aResult[0] = 0;
  int aPosition = 0;
  for (int i = 1; i <= aLength; i++) {
    sprintf(aResult + aPosition , "%d ", Value(i));
    aPosition += strlen(aResult + aPosition);
  }
  string ret(aResult);
  delete aResult;
  
  return ret;
}
			
void SALOMEDSImpl_AttributeSequenceOfInteger::Load(const string& value) 
{
  char* aCopy = (char*)value.c_str();
  char* adr = strtok(aCopy, " ");
  while (adr) {
    int l =  atol(adr);
    Add(l);
    adr = strtok(NULL, " ");
  }
}
