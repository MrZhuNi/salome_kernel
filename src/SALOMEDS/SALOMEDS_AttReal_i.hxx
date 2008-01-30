//  SALOME SALOMEDS : data structure of SALOME and sources of Salome data server 
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
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
//  File   : SALOMEDS_AttReal_i.hxx
//  Author : Estelle Deville
//  Module : SALOME
//  $Header$

#ifndef __SALOMEDS_ATTREAL_I_H__
#define __SALOMEDS_ATTREAL_I_H__

// IDL headers
#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SALOMEDS_AttReal)
#include "SALOMEDS_BasicAttribute_i.hxx"

class SALOMEDS_AttReal_i: public POA_SALOMEDS::AttReal,
			  public SALOMEDS_BasicAttribute_i {
protected:
  char * _type;

public:
  
  SALOMEDS_AttReal_i() { _type = "AttReal";};  
  ~SALOMEDS_AttReal_i() {};
  void Set(CORBA::Double r);
  CORBA::Double Get();
  virtual  char* Save();
  virtual  void Load();
  CORBA::Boolean GetPtr(SALOMEDS::SObject_ptr so);
  virtual char* GetType();
};
#endif
