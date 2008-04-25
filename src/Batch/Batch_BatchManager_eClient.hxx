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
/*
 * BatchManager_eLSF.hxx : emulation of LSF client
 *
 * Auteur : Bernard SECHER - CEA DEN
 * Mail   : mailto:bernard.secher@cea.fr
 * Date   : Thu Apr 24 10:17:22 2008
 * Projet : PAL Salome 
 *
 */

#ifndef _BATCHMANAGER_eClient_H_
#define _BATCHMANAGER_eClient_H_


#include "MpiImpl.hxx"
#include "Batch_Job.hxx"

namespace Batch {

  class Job;

  class EmulationException
  {
  public:
    const std::string msg;
    
    EmulationException(const std::string m) : msg(m) {}
  };

  class BatchManager_eClient
  {
  public:
    // Constructeur et destructeur
    BatchManager_eClient(const char* host="localhost", const char* protocol="ssh", const char* mpiImpl="indif");
    virtual ~BatchManager_eClient();

  protected:
    std::string _host; // serveur ou tourne le BatchManager
    std::string _protocol; // protocol to access _hostname
    std::string _username; // username to access _hostname
    MpiImpl *_mpiImpl; // Mpi implementation to launch executable in batch script

    std::string BuildTemporaryFileName() const;
    void RmTmpFile(std::string & TemporaryFileName);
    MpiImpl* FactoryMpiImpl(string mpiImpl) throw(EmulationException);
    void exportInputFiles(const Job & job) throw(EmulationException);

  private:

  };

}

#endif
