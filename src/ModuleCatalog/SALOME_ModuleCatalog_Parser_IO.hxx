//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SALOME ModuleCatalog : implementation of ModuleCatalog server which parsers xml description of modules
//  File   : SALOME_ModuleCatalog_Parser_IO.hxx
//  Author : Marc Tajchman
//  Module : SALOME
//  $Header$
//
#ifndef SALOME_CATALOG_PARSER_IO_H
#define SALOME_CATALOG_PARSER_IO_H

#include "SALOME_ModuleCatalog_Parser.hxx"
#include <iostream>

std::ostream & operator<< (std::ostream & f, 
                           const ParserParameter & P);
std::ostream & operator<< (std::ostream & f, 
                           const ParserDataStreamParameter & P);

std::ostream & operator<< (std::ostream & f, 
                           const ParserService & S);

std::ostream & operator<< (std::ostream & f, 
                           const ParserInterface & I);

std::ostream & operator<< (std::ostream & f, 
                           const ParserComponent & C);

std::ostream & operator<< (std::ostream & f, 
                           const ParserComponentType & T);

#endif

