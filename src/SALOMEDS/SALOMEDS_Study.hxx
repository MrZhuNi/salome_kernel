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
//  File   : SALOMEDS_Study.hxx
//  Author : Sergey RUIN
//  Module : SALOME

#ifndef __SALOMEDS_STUDY_H__
#define __SALOMEDS_STUDY_H__

#include <vector>
#include <string>

#include "SALOMEDSClient.hxx"
#include "SALOMEDSImpl_Study.hxx"

// IDL headers
#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SALOMEDS)

class Standard_EXPORT SALOMEDS_Study: public SALOMEDSClient_Study
{

private:
  bool                       _isLocal;
  Handle(SALOMEDSImpl_Study) _local_impl;
  SALOMEDS::Study_var        _corba_impl;
  CORBA::ORB_var             _orb;

public:

  SALOMEDS_Study(const Handle(SALOMEDSImpl_Study)& theStudy);
  SALOMEDS_Study(SALOMEDS::Study_ptr theStudy);
  ~SALOMEDS_Study();

  virtual std::string GetPersistentReference();
  virtual std::string GetTransientReference();
  virtual bool IsEmpty();
  virtual _PTR(SComponent) FindComponent (const std::string& aComponentName);
  virtual _PTR(SComponent) FindComponentID(const std::string& aComponentID);
  virtual _PTR(SObject) FindObject(const std::string& anObjectName);
  virtual std::vector<_PTR(SObject)> FindObjectByName( const std::string& anObjectName, const std::string& aComponentName ) ;  
  virtual _PTR(SObject) FindObjectID(const std::string& anObjectID);
  virtual _PTR(SObject) CreateObjectID(const std::string& anObjectID);
  virtual _PTR(SObject) FindObjectIOR(const std::string& anObjectIOR);
  virtual _PTR(SObject) FindObjectByPath(const std::string& thePath);
  virtual std::string GetObjectPath(const _PTR(SObject)& theSO);
  virtual void SetContext(const std::string& thePath);
  virtual std::string GetContext();  
  virtual std::vector<std::string> GetObjectNames(const std::string& theContext);
  virtual std::vector<std::string> GetDirectoryNames(const std::string& theContext);
  virtual std::vector<std::string> GetFileNames(const std::string& theContext);
  virtual std::vector<std::string> GetComponentNames(const std::string& theContext);
  virtual _PTR(ChildIterator) NewChildIterator(const _PTR(SObject)& theSO);
  virtual _PTR(SComponentIterator) NewComponentIterator();
  virtual _PTR(StudyBuilder) NewBuilder();
  virtual std::string Name();
  virtual void  Name(const std::string& name);
  virtual bool IsSaved();
  virtual void  IsSaved(bool save);
  virtual bool IsModified();
  virtual std::string URL();
  virtual void  URL(const std::string& url);
  virtual int StudyId();
  virtual void  StudyId(int id);
  virtual std::vector<_PTR(SObject)> FindDependances(const _PTR(SObject)& theSO);
  virtual _PTR(AttributeStudyProperties) GetProperties();
  virtual std::string GetLastModificationDate();
  virtual std::vector<std::string> GetModificationsDate();
  virtual _PTR(UseCaseBuilder) GetUseCaseBuilder();
  virtual void Close();
  virtual void EnableUseCaseAutoFilling(bool isEnabled);
  virtual bool DumpStudy(const std::string& thePath,const std::string& theBaseName,bool isPublished); 
  virtual _PTR(AttributeParameter) GetCommonParameters(const std::string& theID, int theSavePoint);
  virtual _PTR(AttributeParameter) GetModuleParameters(const std::string& theID, 
						       const std::string& theModuleName, int theSavePoint);
  std::string ConvertObjectToIOR(CORBA::Object_ptr theObject);
  CORBA::Object_ptr ConvertIORToObject(const std::string& theIOR);     

  SALOMEDS::Study_ptr GetStudy();

private:
  void init_orb();

};
#endif
