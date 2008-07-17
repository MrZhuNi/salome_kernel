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
//  File   : SALOMEDSImpl_AttributeIOR.hxx
//  Author : Sergey RUIN
//  Module : SALOME

#ifndef _SALOMEDSImpl_AttributeString_HeaderFile
#define _SALOMEDSImpl_AttributeString_HeaderFile

#include "SALOMEDSImpl_Defines.hxx"
#include "DF_Attribute.hxx"
#include <string>
#include "DF_Label.hxx" 
#include "SALOMEDSImpl_GenericAttribute.hxx"


class SALOMEDSIMPL_EXPORT SALOMEDSImpl_AttributeString : 
  public SALOMEDSImpl_GenericAttribute 
{
private:

  std::string myString;

public:

  static const std::string& GetID() ;

  SALOMEDSImpl_AttributeString() :SALOMEDSImpl_GenericAttribute("AttributeString") {}

  static SALOMEDSImpl_AttributeString* Set(const DF_Label& L, const std::string& Val); 
  void SetValue (const std::string& S);
  std::string Value() const { return myString; }

  virtual std::string Save() { return myString; }
  virtual void Load(const std::string& theValue) { myString = theValue; }

  const std::string& ID() const;
  void Restore(DF_Attribute* with) ;
  DF_Attribute* NewEmpty() const;
  void Paste(DF_Attribute* into);
  ~SALOMEDSImpl_AttributeString() {}

};

#endif
