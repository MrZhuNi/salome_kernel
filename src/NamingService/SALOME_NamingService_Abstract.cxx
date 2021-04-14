// Copyright (C) 2021  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
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

#include "SALOME_NamingService_Abstract.hxx"

#include <sstream>

// ============================================================================
/*! \brief provide a default container name if empty.
 *
 *  the given container name is returned unchanged, unless it is empty.
 * \param  containerName
 * \return container name, where empty input is replaced by "FactoryServer",
 *         without the path.
 * \sa BuildContainerNameForNS(const char *containerName, const char *hostname)
 */
// ============================================================================

std::string SALOME_NamingService_Abstract::ContainerName(const char *containerName)
{
  std::string ret,containerNameCpp(containerName);

  if (containerNameCpp.empty())
    ret = "FactoryServer";
  else
    ret = containerName;

  return ret;
}

// ============================================================================
/*! \brief build a string representing a container in Naming Service.
 *
 *  Build a string representing the absolute pathname of a container in
 *  SALOME_NamingService. This form gives a suffixed containerName in case of
 *  multi processor machine.
 * \param containerName name of the container in which the component is
                        instantiated.
 * \param hostname name of the host of the container, without domain names.
 * \return the path under the form /Containers/hostname/containerName
 * \sa ContainerName(const Engines::MachineParameters& params)
 */
// ============================================================================

std::string SALOME_NamingService_Abstract::BuildContainerNameForNS(const char *containerName, const char *hostname)
{
  std::string ret("/Containers/");
  ret += hostname;
  ret += "/";
  ret += ContainerName(containerName);

  return ret;
}

// ============================================================================
/*! \brief build a container name, given a ContainerParameters struct.
 *
 *  Build a container name with a ContainerParameters struct. In case of multi
 *  processor machine, container name is suffixed with number of processors.
 * \param params struct from which we get container name (may be empty) and
 *               number of processors.
 * \return a container name without the path.
 * \sa BuildContainerNameForNS(const Engines::ContainerParameters& params,
 *                             const char *hostname)
 */
// ============================================================================

std::string SALOME_NamingService_Abstract::ContainerName(const Engines::ContainerParameters& params)
{
  int nbproc;

  if ( !params.isMPI )
    nbproc = 0;
  else if ( params.nb_proc <= 0 )
    nbproc = 1;
  else
    nbproc = params.nb_proc;

  std::string ret(SALOME_NamingService_Abstract::ContainerName(params.container_name));

  if ( nbproc >= 1 )
    {
          std::ostringstream suffix;
          suffix << "_" << nbproc;
      ret += suffix.str();
    }

  return ret;
}

// ============================================================================
/*! \brief build a string representing a container in Naming Service.
 *
 *  Build a string representing the absolute pathname of a container in
 *  SALOME_NamingService.
 * \param params used as it is, or replaced by FactoryServer if empty.
 * \param hostname name of the host of the container, without domain names.
 * \return the path under the form /Containers/hostname/containerName
 * \sa ContainerName(const char *containerName)
 */
// ============================================================================

std::string SALOME_NamingService_Abstract::BuildContainerNameForNS(const Engines::ContainerParameters& params, const char *hostname)
{
  std::string ret("/Containers/");
  ret += hostname;
  ret += "/";
  ret += ContainerName(params);

  return ret;
}
