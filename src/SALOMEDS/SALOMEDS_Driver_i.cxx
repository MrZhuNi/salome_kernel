using namespace std;  

#include "SALOMEDS_Driver_i.hxx"
#include "utilities.h"
#include "SALOMEDS_SObject_i.hxx"
#include "SALOMEDS_SComponent_i.hxx"
#include "SALOMEDS_Study_i.hxx"
#include "SALOMEDS.hxx"
#include <stdlib.h>

SALOMEDS_Driver_i::~SALOMEDS_Driver_i()
{
  CORBA::release(_driver);
  CORBA::release(_orb); 
}

unsigned char* SALOMEDS_Driver_i::Save(const Handle(SALOMEDSImpl_SComponent)& theComponent,
				       const TCollection_AsciiString& theURL,
				       long& theStreamLength,
				       bool isMultiFile)
{  
  SALOMEDS::SComponent_var sco = SALOMEDS_SComponent_i::New (theComponent, _orb);
  SALOMEDS::TMPFile_var aStream;
  CORBA::String_var url = CORBA::string_dup(theURL.ToCString());
  SALOMEDS::unlock(); 
  aStream = _driver->Save(sco.in(), url, isMultiFile);
  SALOMEDS::lock(); 
  theStreamLength = aStream->length();
  unsigned char* aRetStream = NULL;
  if(theStreamLength > 0) {
    aRetStream = new unsigned char[theStreamLength];
    memcpy(aRetStream, aStream->NP_data(), theStreamLength);
  }
  return aRetStream;
}

unsigned char* SALOMEDS_Driver_i::SaveASCII(const Handle(SALOMEDSImpl_SComponent)& theComponent,
					    const TCollection_AsciiString& theURL,
					    long& theStreamLength,
					    bool isMultiFile)
{
  SALOMEDS::SComponent_var sco = SALOMEDS_SComponent_i::New (theComponent, _orb);
  SALOMEDS::TMPFile_var aStream;
  CORBA::String_var url = CORBA::string_dup(theURL.ToCString());
  SALOMEDS::unlock(); 
  aStream = _driver->SaveASCII(sco.in(), url, isMultiFile);
  SALOMEDS::lock(); 
  theStreamLength = aStream->length();
  unsigned char* aRetStream = NULL;
  if(theStreamLength > 0) {
    aRetStream = new unsigned char[theStreamLength];
    memcpy(aRetStream, aStream->NP_data(), theStreamLength);
  }
  return aRetStream; 
}
  
bool SALOMEDS_Driver_i::Load(const Handle(SALOMEDSImpl_SComponent)& theComponent,
			     const unsigned char* theStream,
			     const long theStreamLength,
			     const TCollection_AsciiString& theURL,
			     bool isMultiFile)
{
  SALOMEDS::SComponent_var sco = SALOMEDS_SComponent_i::New (theComponent, _orb);
  CORBA::String_var url = CORBA::string_dup(theURL.ToCString());
  CORBA::Octet* anOctetBuf =  (CORBA::Octet*)theStream;

  SALOMEDS::TMPFile_var aStream;
  if(theStreamLength > 0) 
    aStream = new SALOMEDS::TMPFile(theStreamLength, theStreamLength, anOctetBuf, 0);  
  else 
    aStream = new SALOMEDS::TMPFile(0);

  SALOMEDS::unlock(); 
  return _driver->Load(sco.in(), aStream.in(), url, isMultiFile);
}

bool SALOMEDS_Driver_i::LoadASCII(const Handle(SALOMEDSImpl_SComponent)& theComponent,
				  const unsigned char* theStream,
				  const long theStreamLength,
				  const TCollection_AsciiString& theURL,
				  bool isMultiFile)
{
  SALOMEDS::SComponent_var sco = SALOMEDS_SComponent_i::New (theComponent, _orb);
  CORBA::String_var url = CORBA::string_dup(theURL.ToCString());
  CORBA::Octet* anOctetBuf =  (CORBA::Octet*)theStream;

  SALOMEDS::TMPFile_var aStream;
  if(theStreamLength > 0) 
    aStream = new SALOMEDS::TMPFile(theStreamLength, theStreamLength, anOctetBuf, 0);  
  else 
    aStream = new SALOMEDS::TMPFile(0);

  SALOMEDS::unlock(); 
  return _driver->LoadASCII(sco.in(), aStream.in(), url, isMultiFile);
}

void SALOMEDS_Driver_i::Close(const Handle(SALOMEDSImpl_SComponent)& theComponent)
{
  SALOMEDS::SComponent_var sco = SALOMEDS_SComponent_i::New (theComponent, _orb);
  SALOMEDS::unlock(); 
  _driver->Close(sco.in());
  SALOMEDS::lock(); 
}
 


TCollection_AsciiString SALOMEDS_Driver_i::IORToLocalPersistentID(const Handle(SALOMEDSImpl_SObject)& theSObject,
								  const TCollection_AsciiString& IORString,
								  bool isMultiFile,
								  bool isASCII)
{
  SALOMEDS::SObject_var so = SALOMEDS_SObject_i::New (theSObject, _orb);
  CORBA::String_var ior = CORBA::string_dup(IORString.ToCString());
  SALOMEDS::unlock(); 
  CORBA::String_var pers_string =_driver->IORToLocalPersistentID(so.in(), ior.in(), isMultiFile, isASCII);
  SALOMEDS::lock(); 
  return TCollection_AsciiString(pers_string);
}


TCollection_AsciiString SALOMEDS_Driver_i::LocalPersistentIDToIOR(const Handle(SALOMEDSImpl_SObject)& theObject,
								  const TCollection_AsciiString& aLocalPersistentID,
								  bool isMultiFile,
								  bool isASCII)
{
  SALOMEDS::SObject_var so = SALOMEDS_SObject_i::New (theObject, _orb);
  CORBA::String_var pers_string = CORBA::string_dup(aLocalPersistentID.ToCString());
  SALOMEDS::unlock(); 
  CORBA::String_var IOR =_driver->LocalPersistentIDToIOR(so.in(), pers_string.in(), isMultiFile, isASCII);
  SALOMEDS::lock(); 
  return TCollection_AsciiString(IOR);
}

bool SALOMEDS_Driver_i::CanCopy(const Handle(SALOMEDSImpl_SObject)& theObject)
{
  SALOMEDS::SObject_var so = SALOMEDS_SObject_i::New (theObject, _orb);
  SALOMEDS::unlock();
  return _driver->CanCopy(so.in());
}

/*
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
*/

unsigned char* SALOMEDS_Driver_i::CopyFrom(const Handle(SALOMEDSImpl_SObject)& theObject, 
					   int& theObjectID,
					   long& theStreamLength)
{
  SALOMEDS::SObject_var so = SALOMEDS_SObject_i::New (theObject, _orb);
  SALOMEDS::TMPFile_var aStream;
  CORBA::Long anObjectID;

  SALOMEDS::unlock();
  aStream = _driver->CopyFrom(so.in(), anObjectID);
  SALOMEDS::lock();

  theObjectID = anObjectID;
  theStreamLength = aStream->length();

  unsigned char* aRetStream = NULL;
  if(theStreamLength > 0) {
    aRetStream = new unsigned char[theStreamLength];
    memcpy(aRetStream, aStream->NP_data(), theStreamLength);
  }

/*
  int fd = open("/dn05/salome/srn/sh_copy.brep", O_CREAT | O_WRONLY);
  cout << "Written " << write(fd, (char*)aRetStream, theStreamLength) << endl;
  close(fd);
*/
  return aRetStream;
}

bool SALOMEDS_Driver_i::CanPaste(const TCollection_AsciiString& theComponentName, int theObjectID)
{
  SALOMEDS::unlock();
  return _driver->CanPaste(theComponentName.ToCString(), theObjectID);
}

TCollection_AsciiString SALOMEDS_Driver_i::PasteInto(const unsigned char* theStream,
						     const long theStreamLength,
						     int theObjectID,
						     const Handle(SALOMEDSImpl_SObject)& theObject)
{
  SALOMEDS::SObject_var so = SALOMEDS_SObject_i::New (theObject, _orb);
  CORBA::Octet* anOctetBuf =  (CORBA::Octet*)theStream;

  SALOMEDS::TMPFile_var aStream;
  if(theStreamLength > 0) 
    aStream = new SALOMEDS::TMPFile(theStreamLength, theStreamLength, anOctetBuf, 0);  
  else 
    aStream = new SALOMEDS::TMPFile(0);

  SALOMEDS::unlock();
  SALOMEDS::SObject_var ret_so = _driver->PasteInto(aStream.in(), theObjectID, so.in());
  SALOMEDS::lock();

  return TCollection_AsciiString((char*)ret_so->GetID());
}

unsigned char* SALOMEDS_Driver_i::DumpPython(const Handle(SALOMEDSImpl_Study)& theStudy, 
					     bool isPublished, 
					     bool& isValidScript,
					     long& theStreamLength)
{
  SALOMEDS_Study_i *  st_servant = new SALOMEDS_Study_i (theStudy, _orb);
  SALOMEDS::Study_var st  = SALOMEDS::Study::_narrow(st_servant->_this());
  Engines::TMPFile_var aStream;
  CORBA::Boolean aValidScript, aPublished;
  aPublished = isPublished;
  Engines::Component_ptr aComponent = Engines::Component::_narrow(_driver);
  SALOMEDS::unlock();
  aStream = aComponent->DumpPython(st.in(), aPublished, aValidScript);
  SALOMEDS::lock();
  isValidScript = aValidScript;
  theStreamLength = aStream->length();
  unsigned char* aRetStream = NULL;

  if(theStreamLength > 0) {
    aRetStream = new unsigned char[theStreamLength];
    memcpy(aRetStream, aStream->NP_data(), theStreamLength);
  }

  return aRetStream;
}

//###############################################################################################################
//                                          SALOMEDS_DriverFactory
//###############################################################################################################

SALOMEDSImpl_Driver* SALOMEDS_DriverFactory_i::GetDriverByType(const TCollection_AsciiString& theComponentType)
{
  CORBA::Object_var obj;

  TCollection_AsciiString aFactoryType;
  if (theComponentType == "SUPERV") aFactoryType = "SuperVisionContainer";
  else aFactoryType = "FactoryServer";
  
  obj = SALOME_LifeCycleCORBA(_name_service).FindOrLoad_Component(aFactoryType.ToCString(), 
								  theComponentType.ToCString());
  
  if (CORBA::is_nil(obj)) {
    obj = SALOME_LifeCycleCORBA(_name_service).FindOrLoad_Component("FactoryServerPy", theComponentType.ToCString());
  }
		
  if (!CORBA::is_nil(obj)) {
    SALOMEDS::Driver_var aDriver = SALOMEDS::Driver::_narrow(obj);
    return new SALOMEDS_Driver_i(aDriver, _orb);
  }

  return NULL;
}

SALOMEDSImpl_Driver* SALOMEDS_DriverFactory_i::GetDriverByIOR(const TCollection_AsciiString& theIOR)
{
  CORBA::Object_var obj;
  obj = _orb->string_to_object(theIOR.ToCString());
 
  if (!CORBA::is_nil(obj)) {
    SALOMEDS::Driver_var aDriver = SALOMEDS::Driver::_narrow(obj);
    return new SALOMEDS_Driver_i(aDriver, _orb);
  }

  return NULL;
}
