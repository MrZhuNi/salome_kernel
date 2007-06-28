// Copyright (C) 2007  OPEN CASCADE, CEA/DEN, EDF R&D, PRINCIPIA R&D
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : Salome_file_i.cxx
//  Author : André RIBES, EDF
//  Module : SALOME
//  $Header: 

#include "Salome_file_i.hxx"
#include "utilities.h"
#include <stdlib.h>
#include <unistd.h>

//=============================================================================
/*! 
 *  Default constructor,
 */
//=============================================================================

Salome_file_i::Salome_file_i()
{
  _fileId = 0;
  _path_max = 1 + pathconf("/", _PC_PATH_MAX);
  _state.name = CORBA::string_dup("");
  _state.hdf5_file_name = CORBA::string_dup("");
  _state.number_of_files = 0;
  _state.files_ok = true;
}

//=============================================================================
/*! 
 *  Destructor
 */
//=============================================================================

Salome_file_i::~Salome_file_i()
{
}

void 
Salome_file_i::load(const char* hdf5_file) {
  MESSAGE("Salome_file_i::load : NOT YET IMPLEMENTED");
  _state.hdf5_file_name = CORBA::string_dup(hdf5_file);
}

void 
Salome_file_i::save(const char* hdf5_file) {
  MESSAGE("Salome_file_i::save : NOT YET IMPLEMENTED");
  _state.hdf5_file_name = CORBA::string_dup(hdf5_file);
}

void 
Salome_file_i::setLocalFile(const char* comp_file_name)
{
  std::string file_name("");
  std::string path("");
  std::string type("local");
  std::string source_file_name("");
  std::string status("not_ok");

  std::string cp_file_name(comp_file_name);
  std::size_t index = cp_file_name.rfind("/");
  if (index != -1)
  {
    file_name = cp_file_name.substr(index+1);
    path =  cp_file_name.substr(0,index+1);
  }
  else
  {
    file_name = comp_file_name;
    char CurrentPath[_path_max];
    getcwd(CurrentPath, _path_max);
    path = CurrentPath;
  }

  // Test if this file is already added
  _t_fileManaged::iterator it = _fileManaged.find(file_name);
  if (it != _fileManaged.end()) 
  {
    SALOME::ExceptionStruct es;
    es.type = SALOME::INTERNAL_ERROR;
    std::string text = "file already added";
    es.text = CORBA::string_dup(text.c_str());
    throw SALOME::SALOME_Exception(es);
  }

  // Test if the file is ok
  if(fopen(comp_file_name,"rb") != NULL)
    status = "ok";

  // Adding file with is informations
  Engines::file infos;
  infos.file_name = CORBA::string_dup(file_name.c_str());
  infos.path = CORBA::string_dup(path.c_str());
  infos.type = CORBA::string_dup(type.c_str());
  infos.source_file_name = CORBA::string_dup(source_file_name.c_str());
  infos.status = CORBA::string_dup(status.c_str());

  _fileManaged[file_name] = infos;

  // Update Salome_file state
  _state.number_of_files++;
  if (status != "ok")
    _state.files_ok = false;
}

void 
Salome_file_i::setDistributedFile(const char* comp_file_name, 
				  Engines::Salome_file_ptr source_Salome_file)
{
  std::string file_name("");
  std::string path("");
  std::string type("distributed");
  std::string source_file_name("");
  std::string status("not_ok");

  std::string cp_file_name(comp_file_name);
  std::size_t index = cp_file_name.rfind("/");
  if (index != -1)
  {
    file_name = cp_file_name.substr(index+1);
    path =  cp_file_name.substr(0,index+1);
  }
  else
  {
    file_name = comp_file_name;
    char CurrentPath[_path_max];
    getcwd(CurrentPath, _path_max);
    path = CurrentPath;
  }

  // Test if this file is already added
  _t_fileManaged::iterator it = _fileManaged.find(file_name);
  if (it != _fileManaged.end()) 
  {
    SALOME::ExceptionStruct es;
    es.type = SALOME::INTERNAL_ERROR;
    std::string text = "file already added";
    es.text = CORBA::string_dup(text.c_str());
    throw SALOME::SALOME_Exception(es);
  }

  // Adding file with is informations
  Engines::file infos;
  infos.file_name = CORBA::string_dup(file_name.c_str());
  infos.path = CORBA::string_dup(path.c_str());
  infos.type = CORBA::string_dup(type.c_str());
  infos.source_file_name = CORBA::string_dup(source_file_name.c_str());
  infos.status = CORBA::string_dup(status.c_str());

  _fileManaged[file_name] = infos;
  _fileDistributedSource[file_name] = Engines::Salome_file::_duplicate(source_Salome_file);

  // Update Salome_file state
  _state.number_of_files++;
  _state.files_ok = false;
}

void 
Salome_file_i::setDistributedSourceFile(const char* file_name,
					const char * source_file_name)
{
  std::string fname(file_name);

  // Test if this file is managed
  _t_fileManaged::iterator it = _fileManaged.find(fname);
  if (it == _fileManaged.end()) 
  {
    SALOME::ExceptionStruct es;
    es.type = SALOME::INTERNAL_ERROR;
    std::string text = "file is not managed";
    es.text = CORBA::string_dup(text.c_str());
    throw SALOME::SALOME_Exception(es);
  }

  _fileManaged[fname].source_file_name = CORBA::string_dup(source_file_name);
}

void 
Salome_file_i::recvFiles() {
  
  std::string files_not_ok("");

  _t_fileManaged::iterator begin = _fileManaged.begin();
  _t_fileManaged::iterator end = _fileManaged.end();
  for(;begin!=end;begin++) 
  {
    bool result = true;
    Engines::file file_infos = begin->second;
    if (std::string(file_infos.type.in()) == "local")
    {
      if (std::string(file_infos.status.in()) == "not_ok")
	result = checkLocalFile(file_infos.file_name.in());
    }
    else
    {
      if (std::string(file_infos.status.in()) == "not_ok")
	result = getDistributedFile(file_infos.file_name.in());
    }
    if (!result) 
    {
      files_not_ok.append(" ");
      files_not_ok.append(file_infos.file_name.in());
    }
  }

  if (files_not_ok != "")
  {
    SALOME::ExceptionStruct es;
    es.type = SALOME::INTERNAL_ERROR;
    std::string text = "files not ready : " + files_not_ok;
    es.text = CORBA::string_dup(text.c_str());
    throw SALOME::SALOME_Exception(es);
  }

  // We change the state of the Salome_file
  _state.files_ok = true;
}

bool
Salome_file_i::checkLocalFile(std::string file_name)
{
  bool result = true;

  std::string comp_file_name(_fileManaged[file_name].path.in());
  comp_file_name.append(_fileManaged[file_name].file_name.in());
  if(fopen(comp_file_name.c_str(),"rb") == NULL)
  {
    INFOS("file " << comp_file_name << " cannot be open for reading");
    _fileManaged[file_name].status = CORBA::string_dup("not_ok");
    result = false;
  }

  if (result)
  {
    _fileManaged[file_name].status = CORBA::string_dup("ok");
  }
  return result;
}

bool
Salome_file_i::getDistributedFile(std::string file_name)
{
  bool result = true;
  const char * source_file_name = _fileManaged[file_name].source_file_name.in();
  int fileId;
  FILE* fp;
  std::string comp_file_name(_fileManaged[file_name].path.in());
  comp_file_name.append(_fileManaged[file_name].file_name.in());

  if ((fp = fopen(comp_file_name.c_str(),"wb")) == NULL)
  {
    INFOS("file " << comp_file_name << " cannot be open for writing");
    _fileManaged[file_name].status = CORBA::string_dup("not_ok");
    result = false;
    return result;
  }

  try 
  {
    fileId = _fileDistributedSource[file_name]->open(source_file_name);
  }
  catch (...) 
  {
    _fileManaged[file_name].status = CORBA::string_dup("not_ok");
    fclose(fp);
    result = false;
    return result;
  }

  if (fileId > 0)
  {
    Engines::fileBlock* aBlock;
    int toFollow = 1;
    int ctr=0;
    MESSAGE("begin of transfer of " << comp_file_name);
    while (toFollow)
    {
      ctr++;
      aBlock = _fileDistributedSource[file_name]->getBlock(fileId);
      toFollow = aBlock->length();
      CORBA::Octet *buf = aBlock->get_buffer();
      int nbWri = fwrite(buf, sizeof(CORBA::Octet), toFollow, fp);
      ASSERT(nbWri == toFollow);
    }
    fclose(fp);
    MESSAGE("end of transfer of " << comp_file_name);
    _fileDistributedSource[file_name]->close(fileId);
  }
  else
  {
    INFOS("open reference file for copy impossible");
    result = false;
    fclose(fp);
    _fileManaged[file_name].status = CORBA::string_dup("not_ok");
    return result;
  }

  _fileManaged[file_name].status = CORBA::string_dup("ok");
  return result;
}

void 
Salome_file_i::removeFile(const char* file_name) 
{
  MESSAGE("Salome_file_i::removeFile : NOT YET IMPLEMENTED");
}
    
void 
Salome_file_i::deleteFile(const char* file_name) {
  MESSAGE("Salome_file_i::deleteFile : NOT YET IMPLEMENTED");
}

void 
Salome_file_i::removeFiles() {
  MESSAGE("Salome_file_i::removeFiles : NOT YET IMPLEMENTED");
}

void 
Salome_file_i::deleteFiles() {
  MESSAGE("Salome_file_i::deleteFiles : NOT YET IMPLEMENTED");
}

Engines::files* 
Salome_file_i::getFilesInfos() {

  Engines::files * infos = new Engines::files();
  infos->length(_fileManaged.size());

  _t_fileManaged::iterator begin = _fileManaged.begin();
  _t_fileManaged::iterator end = _fileManaged.end();
  int i = 0;
  for(;begin!=end;begin++) {
    (*infos)[i] = *(new Engines::file(begin->second));
    i++;
  }
  return infos;
}

Engines::file* 
Salome_file_i::getFileInfos(const char* file_name) {

  std::string fname(file_name);

  // Test if this file is managed
  _t_fileManaged::iterator it = _fileManaged.find(fname);
  if (it == _fileManaged.end()) 
  {
    SALOME::ExceptionStruct es;
    es.type = SALOME::INTERNAL_ERROR;
    es.text = "file is not managed";
    throw SALOME::SALOME_Exception(es);
  }

  Engines::file * infos = new Engines::file(_fileManaged[fname]);
  return infos;
}

Engines::SfState* 
Salome_file_i::getSalome_fileState() 
{
  return new Engines::SfState(_state);
}

//=============================================================================
/*! 
 *  CORBA method: try to open the file given. If the file is readable, return
 *  a positive integer else return 0;
 *  \param  fileName path to the file to be transfered
 *  \return fileId = positive integer > 0 if open OK.
 */
//=============================================================================

CORBA::Long 
Salome_file_i::open(const char* file_name)
{
  int aKey = 0;

  std::string fname(file_name);
  _t_fileManaged::iterator it = _fileManaged.find(fname);
  if (it == _fileManaged.end())
  {
    return aKey;
  }
  
  std::string comp_file_name(_fileManaged[file_name].path.in());
  comp_file_name.append(_fileManaged[file_name].file_name.in());
  MESSAGE("Salome_file_i::open " << comp_file_name);
  FILE* fp;
  if ((fp = fopen(comp_file_name.c_str(),"rb")) == NULL)
    {
      INFOS("file " << comp_file_name << " is not readable");
      return aKey;
    }

  aKey = ++_fileId;
  _fileAccess[aKey] = fp;
  return aKey;
}

//=============================================================================
/*! 
 *  CORBA method: close the file associated to the fileId given at open.
 *  \param fileId got in return from open method
 */
//=============================================================================

void 
Salome_file_i::close(CORBA::Long fileId)
{
  MESSAGE("Salome_file_i::close");
  FILE* fp;
  if (!(fp = _fileAccess[fileId]) )
    {
      INFOS(" no FILE structure associated to fileId " << fileId);
    }
  else fclose(fp);
}

//=============================================================================
/*! 
 *  CORBA method: get a block of data from the file associated to the fileId
 *  given at open.
 *  \param fileId got in return from open method
 *  \return an octet sequence. Last one is empty.
 */
//=============================================================================

#define FILEBLOCK_SIZE 256*1024

Engines::fileBlock* 
Salome_file_i::getBlock(CORBA::Long fileId)
{
  Engines::fileBlock* aBlock = new Engines::fileBlock;

  FILE* fp;
  if (! (fp = _fileAccess[fileId]) )
  {
    INFOS(" no FILE structure associated to fileId " <<fileId);
    return aBlock;
  }

  // use replace member function for sequence to avoid copy
  // see Advanced CORBA Programming with C++ pp 187-194
  CORBA::Octet *buf;
  buf = Engines::fileBlock::allocbuf(FILEBLOCK_SIZE);
  int nbRed = fread(buf, sizeof(CORBA::Octet), FILEBLOCK_SIZE, fp);
  aBlock->replace(nbRed, nbRed, buf, 1); // 1 means give ownership
  return aBlock;
}

