//  File   : SALOMEDS_AttributeTableOfString.cxx
//  Author : Sergey RUIN
//  Module : SALOME

#include "SALOMEDS_AttributeTableOfString.hxx"

#include <TCollection_AsciiString.hxx>
#include <TCollection_ExtendedString.hxx>
#include <TColStd_HSequenceOfInteger.hxx>
#include <TColStd_HSequenceOfReal.hxx>
#include <TColStd_HSequenceOfExtendedString.hxx>
 
SALOMEDS_AttributeTableOfString
::SALOMEDS_AttributeTableOfString(const Handle(SALOMEDSImpl_AttributeTableOfString)& theAttr)
:SALOMEDS_GenericAttribute(theAttr)
{}

SALOMEDS_AttributeTableOfString::SALOMEDS_AttributeTableOfString(SALOMEDS::AttributeTableOfString_ptr theAttr)
:SALOMEDS_GenericAttribute(theAttr)
{}

SALOMEDS_AttributeTableOfString::~SALOMEDS_AttributeTableOfString()
{}


void SALOMEDS_AttributeTableOfString::SetTitle(const char* theTitle)
{
  CheckLocked();
  TCollection_AsciiString aStr((char*)theTitle);
  if(_isLocal) Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetTitle(aStr);
  else SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetTitle(aStr.ToCString());
}

char* SALOMEDS_AttributeTableOfString::GetTitle()
{
  TCollection_AsciiString aStr;
  if(_isLocal) aStr = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetTitle();
  else aStr = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetTitle();
  return aStr.ToCString();
}

void SALOMEDS_AttributeTableOfString::SetRowTitle(int theIndex, const char* theTitle)
{
  CheckLocked();
  TCollection_AsciiString aTitle((char*)theTitle);
  if(_isLocal) Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetRowTitle(theIndex, aTitle);
  else SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetRowTitle(theIndex, aTitle.ToCString());
}

void SALOMEDS_AttributeTableOfString::SetRowTitles(const std::vector<std::string>& theTitles)
{
  CheckLocked();
  int aLength = theTitles.size(), i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aSeq = new TColStd_HSequenceOfExtendedString;
    for(i = 0; i < aLength; i++) aSeq->Append((char*)theTitles[i].c_str());
    Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetRowTitles(aSeq);
  }
  else {
    SALOMEDS::StringSeq_var aSeq = new SALOMEDS::StringSeq();
    aSeq->length(aLength);
    for(i = 0; i < aLength; i++) aSeq[i] = TCollection_AsciiString((char*)theTitles[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetRowTitles(aSeq);
  }
  
}

std::vector<std::string> SALOMEDS_AttributeTableOfString::GetRowTitles()
{
  std::vector<std::string> aVector;
  int aLength, i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aSeq;
    aSeq = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetRowTitles();
    aLength = aSeq->Length();
    for(i = 1; i<= aLength; i++) aVector.push_back(TCollection_AsciiString(aSeq->Value(i)).ToCString());
  }
  else {
    SALOMEDS::StringSeq_var aSeq = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetRowTitles();
    aLength = aSeq->length();
    for(i = 0; i<aLength; i++) aVector.push_back(TCollection_AsciiString((char*)aSeq[i].in()).ToCString());
  }
  return aVector;
}

void SALOMEDS_AttributeTableOfString::SetColumnTitle(int theIndex, const char* theTitle)
{
  CheckLocked();
  TCollection_AsciiString aStr((char*)theTitle);
  if(_isLocal) Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetColumnTitle(theIndex, aStr);
  else SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetColumnTitle(theIndex, aStr.ToCString());
}

void SALOMEDS_AttributeTableOfString::SetColumnTitles(const std::vector<std::string>& theTitles)
{
  CheckLocked();
  int aLength = theTitles.size(), i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aSeq = new TColStd_HSequenceOfExtendedString;
    for(i = 0; i < aLength; i++) aSeq->Append((char*)theTitles[i].c_str());
    Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetColumnTitles(aSeq);
  }
  else {
    SALOMEDS::StringSeq_var aSeq = new SALOMEDS::StringSeq();
    aSeq->length(aLength);
    for(i = 0; i < aLength; i++) aSeq[i] = TCollection_AsciiString((char*)theTitles[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetColumnTitles(aSeq);
  }
}

std::vector<std::string> SALOMEDS_AttributeTableOfString::GetColumnTitles()
{
  std::vector<std::string> aVector;
  int aLength, i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aSeq;
    aSeq = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetColumnTitles();
    aLength = aSeq->Length();
    for(i = 1; i<= aLength; i++) aVector.push_back(TCollection_AsciiString(aSeq->Value(i)).ToCString());
  }
  else {
    SALOMEDS::StringSeq_var aSeq = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetColumnTitles();
    aLength = aSeq->length();
    for(i = 0; i<aLength; i++) aVector.push_back(TCollection_AsciiString((char*)aSeq[i].in()).ToCString());
  }
  return aVector;
}

void SALOMEDS_AttributeTableOfString::SetRowUnit(int theIndex, const char* theUnit)
{
  CheckLocked();
  TCollection_AsciiString aUnit((char*)theUnit);
  if(_isLocal) Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetRowUnit(theIndex, aUnit);
  else SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetRowUnit(theIndex, aUnit.ToCString());
}

void SALOMEDS_AttributeTableOfString::SetRowUnits(const std::vector<std::string>& theUnits)
{
  CheckLocked();
  int aLength = theUnits.size(), i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aSeq = new TColStd_HSequenceOfExtendedString;
    for(i = 0; i < aLength; i++) aSeq->Append((char*)theUnits[i].c_str());
    Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetRowUnits(aSeq);
  }
  else {
    SALOMEDS::StringSeq_var aSeq = new SALOMEDS::StringSeq();
    aSeq->length(aLength);
    for(i = 0; i < aLength; i++) aSeq[i] = TCollection_AsciiString((char*)theUnits[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetRowUnits(aSeq);
  }
}

std::vector<std::string> SALOMEDS_AttributeTableOfString::GetRowUnits()
{
  std::vector<std::string> aVector;
  int aLength, i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aSeq;
    aSeq = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetRowUnits();
    aLength = aSeq->Length();
    for(i = 1; i<= aLength; i++) aVector.push_back(TCollection_AsciiString(aSeq->Value(i)).ToCString());
  }
  else {
    SALOMEDS::StringSeq_var aSeq = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetRowUnits();
    aLength = aSeq->length();
    for(i = 0; i<aLength; i++) aVector.push_back(TCollection_AsciiString((char*)aSeq[i].in()).ToCString());
  }
  return aVector;
}

int SALOMEDS_AttributeTableOfString::GetNbRows()
{
  int aNb;
  if(_isLocal) aNb = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetNbRows();
  else aNb = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetNbRows();
  return aNb;
}

int SALOMEDS_AttributeTableOfString::GetNbColumns()
{  
  int aNb;
  if(_isLocal) aNb = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetNbColumns();
  else aNb = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetNbColumns();
  return aNb;
  
}

void SALOMEDS_AttributeTableOfString::AddRow(const std::vector<std::string>& theData)
{
  CheckLocked();
  int aLength = theData.size(), i;
  if(_isLocal) {
    Handle(SALOMEDSImpl_AttributeTableOfString) aTable;
    aTable = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl);
    Handle(TColStd_HSequenceOfExtendedString) aRow = new TColStd_HSequenceOfExtendedString;
    for (int i = 0; i < aLength; i++) aRow->Append((char*)theData[i].c_str());
    try {
      aTable->SetRowData(aTable->GetNbRows() + 1, aRow);
    }   
    catch(...) {
      throw SALOMEDS::AttributeTableOfString::IncorrectArgumentLength();
    }
  }
  else {
    SALOMEDS::StringSeq_var aSeq = new SALOMEDS::StringSeq();
    aSeq->length(aLength);
    for(i = 0; i < aLength; i++) aSeq[i] = TCollection_AsciiString((char*)theData[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->AddRow(aSeq);
  }
}

void SALOMEDS_AttributeTableOfString::SetRow(int theRow, const std::vector<std::string>& theData)
{
  CheckLocked();
  int aLength = theData.size(), i;
  if(_isLocal) {
    Handle(SALOMEDSImpl_AttributeTableOfString) aTable;
    aTable = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl);
    Handle(TColStd_HSequenceOfExtendedString) aRow = new TColStd_HSequenceOfExtendedString;
    for (int i = 0; i < aLength; i++) aRow->Append((char*)theData[i].c_str());
    try {
      aTable->SetRowData(theRow, aRow);
    }   
    catch(...) {
      throw SALOMEDS::AttributeTableOfString::IncorrectArgumentLength();
    }
  }
  else {
    SALOMEDS::StringSeq_var aSeq = new SALOMEDS::StringSeq();
    aSeq->length(aLength);
    for(i = 0; i < aLength; i++) aSeq[i] = TCollection_AsciiString((char*)theData[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetRow(theRow, aSeq);
  }
}

std::vector<std::string> SALOMEDS_AttributeTableOfString::GetRow(int theRow)
{
  std::vector<std::string> aVector;
  int aLength, i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aRow; 
    aRow = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetRowData(theRow);
    aLength = aRow->Length();
    for(i = 1; i<= aLength; i++) aVector.push_back(TCollection_AsciiString(aRow->Value(i)).ToCString());
  }
  else {
    SALOMEDS::StringSeq_var aRow = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetRow(theRow);
    for(i = 0; i < aLength; i++) aVector.push_back(TCollection_AsciiString((char*)aRow[i].in()).ToCString());
  }

  return aVector;
}

void SALOMEDS_AttributeTableOfString::AddColumn(const std::vector<std::string>& theData)
{
  CheckLocked();
  int aLength = theData.size(), i;
  if(_isLocal) {
    Handle(SALOMEDSImpl_AttributeTableOfString) aTable;
    aTable = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl);
    Handle(TColStd_HSequenceOfExtendedString) aColumn = new TColStd_HSequenceOfExtendedString;
    for (int i = 0; i < aLength; i++) aColumn->Append((char*)theData[i].c_str());
    try {
      aTable->SetColumnData(aTable->GetNbColumns() + 1, aColumn);
    }   
    catch(...) {
      throw SALOMEDS::AttributeTableOfString::IncorrectArgumentLength();
    }
  }
  else {
    SALOMEDS::StringSeq_var aColumn = new SALOMEDS::StringSeq();
    aColumn->length(aLength);
    for(i = 0; i < aLength; i++) aColumn[i] = TCollection_AsciiString((char*)theData[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->AddColumn(aColumn);
  }
}

void SALOMEDS_AttributeTableOfString::SetColumn(int theColumn, const std::vector<std::string>& theData)
{
  CheckLocked();
  int aLength = theData.size(), i;
  if(_isLocal) {
    Handle(SALOMEDSImpl_AttributeTableOfString) aTable;
    aTable = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl);
    Handle(TColStd_HSequenceOfExtendedString) aColumn = new TColStd_HSequenceOfExtendedString;
    for (int i = 0; i < aLength; i++) aColumn->Append((char*)theData[i].c_str());
    try {
      aTable->SetRowData(theColumn, aColumn);
    }   
    catch(...) {
      throw SALOMEDS::AttributeTableOfString::IncorrectArgumentLength();
    }
  }
  else {
    SALOMEDS::StringSeq_var aColumn = new SALOMEDS::StringSeq();
    aColumn->length(aLength);
    for(i = 0; i < aLength; i++) aColumn[i] = TCollection_AsciiString((char*)theData[i].c_str()).ToCString();
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetRow(theColumn, aColumn);
  }
}

std::vector<std::string> SALOMEDS_AttributeTableOfString::GetColumn(int theColumn)
{
  std::vector<std::string> aVector;
  int aLength, i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfExtendedString) aColumn; 
    aColumn = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetColumnData(theColumn);
    aLength = aColumn->Length();
    for(i = 1; i<= aLength; i++) aVector.push_back(TCollection_AsciiString(aColumn->Value(i)).ToCString());
  }
  else {
    SALOMEDS::StringSeq_var aColumn = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetColumn(theColumn);
    for(i = 0; i < aLength; i++) aVector.push_back(aColumn[i].in());
  }
  return aVector;
}

void SALOMEDS_AttributeTableOfString::PutValue(const char* theValue, int theRow, int theColumn)
{
  CheckLocked();
  TCollection_AsciiString aValue((char*)theValue);
  if(_isLocal) {
    try {
      Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->PutValue(aValue, theRow, theColumn);
    }   
    catch(...) {
      throw SALOMEDS::AttributeTableOfString::IncorrectIndex();
    }
  }
  else {
    SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->PutValue(aValue.ToCString(), theRow, theColumn);
  }
}

bool SALOMEDS_AttributeTableOfString::HasValue(int theRow, int theColumn)
{
  bool ret;
  if(_isLocal) ret = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->HasValue(theRow, theColumn);
  else ret = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->HasValue(theRow, theColumn);
  return ret;
}

char* SALOMEDS_AttributeTableOfString::GetValue(int theRow, int theColumn)
{
  TCollection_AsciiString aValue;
  if(_isLocal) {
    try {
      aValue = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetValue(theRow, theColumn);
    }   
    catch(...) {
      throw SALOMEDS::AttributeTableOfString::IncorrectIndex();
    }
  }
  else {
    aValue = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetValue(theRow, theColumn);
  }
  return aValue.ToCString();
}

std::vector<int> SALOMEDS_AttributeTableOfString::GetRowSetIndices(int theRow)
{
  std::vector<int> aVector;
  int aLength, i;
  if(_isLocal) {
    Handle(TColStd_HSequenceOfInteger) aSet; 
    aSet = Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->GetSetRowIndices(theRow);
    aLength = aSet->Length();
    for(i = 1; i<= aLength; i++) aVector.push_back(aSet->Value(i));
  }
  else {
    SALOMEDS::LongSeq_var aSet = SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->GetRowSetIndices(theRow);
    for(i = 0; i < aLength; i++) aVector.push_back(aSet[i]);
  }
  return aVector;
}

void SALOMEDS_AttributeTableOfString::SetNbColumns(int theNbColumns)
{
  if(_isLocal) Handle(SALOMEDSImpl_AttributeTableOfString)::DownCast(_local_impl)->SetNbColumns(theNbColumns);
  else SALOMEDS::AttributeTableOfString::_narrow(_corba_impl)->SetNbColumns(theNbColumns);
}
