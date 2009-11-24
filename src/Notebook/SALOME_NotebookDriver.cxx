
#include <SALOME_NotebookDriver.hxx>
#include <SALOMEDS_Tool.hxx>
#include <stdio.h>

SALOME_NotebookDriver::SALOME_NotebookDriver()
{
}

SALOME_NotebookDriver::~SALOME_NotebookDriver()
{
}

std::string SALOME_NotebookDriver::GetFileName( SALOMEDS::SComponent_ptr theComponent, bool isMultiFile ) const
{
  // Prepare a file name to open
  std::string aNameWithExt;
  if( isMultiFile )
    aNameWithExt = SALOMEDS_Tool::GetNameFromPath( theComponent->GetStudy()->URL() ).c_str();
  aNameWithExt += "_Notebook.dat";
  return aNameWithExt;
}

SALOMEDS::TMPFile* SALOME_NotebookDriver::Save( SALOMEDS::SComponent_ptr theComponent, const char* theURL, bool isMultiFile )
{
  SALOMEDS::TMPFile_var aStreamFile;

  // Get a temporary directory to store a file
  std::string aTmpDir = isMultiFile ? theURL : SALOMEDS_Tool::GetTmpDir();

  // Create a list to store names of created files
  SALOMEDS::ListOfFileNames_var aSeq = new SALOMEDS::ListOfFileNames;
  aSeq->length( 1 );

  std::string aNameWithExt = GetFileName( theComponent, isMultiFile );
  aSeq[0] = CORBA::string_dup( aNameWithExt.c_str() );

  // Build a full file name of temporary file
  std::string aFullName = std::string( aTmpDir.c_str() ) + aNameWithExt;

  // Save Notebook component into this file
  if( theComponent->GetStudy()->GetNotebook()->Save( aFullName.c_str() ) )
    // Convert a file to the byte stream
    aStreamFile = SALOMEDS_Tool::PutFilesToStream( aTmpDir.c_str(), aSeq.in(), isMultiFile );

  // Remove the created file and tmp directory
  if( !isMultiFile )
    SALOMEDS_Tool::RemoveTemporaryFiles( aTmpDir.c_str(), aSeq.in(), true );

  // Return the created byte stream
  return aStreamFile._retn();
}

SALOMEDS::TMPFile* SALOME_NotebookDriver::SaveASCII( SALOMEDS::SComponent_ptr theComponent, const char* theURL, bool isMultiFile )
{
  SALOMEDS::TMPFile_var aStreamFile = Save( theComponent, theURL, isMultiFile );
  return aStreamFile._retn();
}

CORBA::Boolean SALOME_NotebookDriver::Load( SALOMEDS::SComponent_ptr theComponent, const SALOMEDS::TMPFile& theStream,
                                            const char* theURL, bool isMultiFile )
{
  printf( "SALOME_NotebookDriver::Load\n" );

  // Get a temporary directory for a file
  std::string aTmpDir = isMultiFile ? theURL : SALOMEDS_Tool::GetTmpDir();

  // Convert the byte stream theStream to a file and place it in tmp directory
  SALOMEDS::ListOfFileNames_var aSeq =
    SALOMEDS_Tool::PutStreamToFiles( theStream, aTmpDir.c_str(), isMultiFile );

  // Prepare a file name to open
  std::string aNameWithExt = GetFileName( theComponent, isMultiFile );
  std::string aFullName = std::string( aTmpDir.c_str() ) + aNameWithExt;

  // Open document
  if( !theComponent->GetStudy()->GetNotebook()->Load( aFullName.c_str() ) )
    return false;

  // Remove the created file and tmp directory
  if( !isMultiFile )
    SALOMEDS_Tool::RemoveTemporaryFiles( aTmpDir.c_str(), aSeq.in(), true );

  return true;
}

CORBA::Boolean SALOME_NotebookDriver::LoadASCII( SALOMEDS::SComponent_ptr theComponent, const SALOMEDS::TMPFile& theStream,
                                                 const char* theURL, bool isMultiFile )
{
  return Load( theComponent, theStream, theURL, isMultiFile );
}

void SALOME_NotebookDriver::Close( SALOMEDS::SComponent_ptr theComponent )
{
}

char* SALOME_NotebookDriver::ComponentDataType()
{
  return CORBA::string_dup( "NOTEBOOK" );
}

char* SALOME_NotebookDriver::IORToLocalPersistentID( SALOMEDS::SObject_ptr theSObject, const char* theIOR,
                                                     CORBA::Boolean isMultiFile, CORBA::Boolean isASCII )
{
  return 0;
}

char* SALOME_NotebookDriver::LocalPersistentIDToIOR( SALOMEDS::SObject_ptr theSObject, const char* theLocalPersistentID,
                                                     CORBA::Boolean isMultiFile, CORBA::Boolean isASCII )
{
  return 0;
}

bool SALOME_NotebookDriver::CanPublishInStudy( CORBA::Object_ptr theIOR )
{
  return false;
}

SALOMEDS::SObject_ptr SALOME_NotebookDriver::PublishInStudy( SALOMEDS::Study_ptr theStudy, SALOMEDS::SObject_ptr theSObject,
                                                             CORBA::Object_ptr theObject, const char* theName )
{
  SALOMEDS::SObject_var aRes;
  return aRes._retn();
}

CORBA::Boolean SALOME_NotebookDriver::CanCopy( SALOMEDS::SObject_ptr theObject )
{
  return false;
}

SALOMEDS::TMPFile* SALOME_NotebookDriver::CopyFrom ( SALOMEDS::SObject_ptr theObject, CORBA::Long& theObjectID )
{
  return 0;
}

CORBA::Boolean SALOME_NotebookDriver::CanPaste ( const char* theComponentName, CORBA::Long theObjectID )
{
  return false;
}

SALOMEDS::SObject_ptr SALOME_NotebookDriver::PasteInto( const SALOMEDS::TMPFile& theStream, CORBA::Long theObjectID,
                                                        SALOMEDS::SObject_ptr theObject )
{
  SALOMEDS::SObject_var aRes;
  return aRes._retn();
}

Engines::TMPFile* SALOME_NotebookDriver::DumpPython( SALOMEDS::Study_ptr theStudy, bool& isValid ) const
{
  char* aScript = theStudy->GetNotebook()->DumpPython();
  CORBA::Octet* anOctetBuf =  ( CORBA::Octet* )aScript;
  int aSize = strlen( aScript )+1;
  Engines::TMPFile_var aStreamFile = new Engines::TMPFile( aSize, aSize, anOctetBuf, 1 ); 
  isValid = aScript != 0;
  return aStreamFile._retn(); 
}

SALOMEDS::Driver_ptr SALOME_NotebookDriver::getInstance()
{
  static SALOME_NotebookDriver* aDriver = new SALOME_NotebookDriver();
  SALOMEDS::Driver_var aDriverVar = aDriver->_this();
  return aDriverVar._retn();
}