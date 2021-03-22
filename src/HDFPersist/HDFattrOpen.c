/* Copyright (C) 2007-2021  CEA/DEN, EDF R&D, OPEN CASCADE
 *
 * Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
 * CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
 */

/*----------------------------------------------------------------------------
  File   : HDFattrOpen.c
  Module : SALOME
----------------------------------------------------------------------------*/

#include "hdfi.h"

/*
 * - Name: HDFattrOpen
 * - Description : open a HDF attribute
 * - Parameters :
 *     - pid (IN)  : father ID
 *     - name  (IN)  : attribute name 
 * - Result : 
 *     - if success : attribute ID
 *     - if failure : -1 
 */ 
hdf_idt HDFattrOpen(hdf_idt pid,char * name)
{
   hdf_idt aid;

   if ((aid = H5Aopen_name(pid,name)) < 0)
     return -1;

   return aid;
}
