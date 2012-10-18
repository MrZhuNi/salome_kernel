// Copyright (C) 2007-2012  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
//
// This library is distributed in the hope that it will be useful,
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

//  SALOME DataContainer : implementation of data container
//  File   : SALOME_DataContainer_i.cxx
//  Author : Mikhail PONIKAROV
//  Module : SALOME
//  $Header$
//

#include <SALOME_DataContainer_i.hxx>

#include <fstream>
#include <iostream>

using namespace std;

Engines_DataContainer_i::Engines_DataContainer_i()
{
}

Engines_DataContainer_i::Engines_DataContainer_i(const char* url,
  const char* name, const char* identifier, const bool removeAfterGet)
 : myName(name), myIdentifier(identifier), myURL(url), myRemoveAfterGet(removeAfterGet)
{
  std::string anExtension(url);
  if (anExtension.rfind(".") != std::string::npos) { // keep only extension
    myExt = anExtension.substr(anExtension.rfind(".") + 1);
  } else myExt = "";
}

Engines_DataContainer_i::~Engines_DataContainer_i()
{
}

Engines::TMPFile* Engines_DataContainer_i::get()
{
  // open file to make stream from its content
#ifdef WIN32
  ifstream aFile(myURL.c_str(), std::ios::binary);
#else
  ifstream aFile(myURL.c_str());
#endif
  if (!aFile.good()) {
    std::cerr<<"File "<<myURL.c_str()<<" can not be opened for reading"<<std::endl;
  }
  aFile.seekg(0, std::ios::end);
  int aFileSize = aFile.tellg();
  char* aBuffer = new char[aFileSize];
                                                           
  aFile.seekg(0, std::ios::beg);
  aFile.read(aBuffer, aFileSize);
  aFile.close();

  // remove file after it converted to a stream
  if (myRemoveAfterGet) {
    #ifdef WIN32
      DeleteFile(myURL.c_str());
    #else
      unlink(myURL.c_str());
    #endif
  }
                                                                                                                                                    
  // make CORBA TMP file from the buffer
  CORBA::Octet* anOctetBuf =  (CORBA::Octet*)aBuffer;
  Engines::TMPFile_var aStreamFile = new Engines::TMPFile(aFileSize, aFileSize, anOctetBuf, 1);
  
  return aStreamFile._retn();
}

char* Engines_DataContainer_i::name()
{
  return CORBA::string_dup(myName.c_str());
}

char* Engines_DataContainer_i::identifier()
{
  return CORBA::string_dup(myIdentifier.c_str());
}

char* Engines_DataContainer_i::extension()
{
  return CORBA::string_dup(myExt.c_str());
}
