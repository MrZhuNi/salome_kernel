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
#include "SALOME_ResourcesCatalog_Parser.hxx"
#include "utilities.h"
#include <iostream>

#define NULL_VALUE 0

using namespace std;

unsigned int ResourceDataToSort::_nbOfNodesWanted = NULL_VALUE;
unsigned int ResourceDataToSort::_nbOfProcPerNodeWanted = NULL_VALUE;
unsigned int ResourceDataToSort::_CPUFreqMHzWanted = NULL_VALUE;
unsigned int ResourceDataToSort::_memInMBWanted = NULL_VALUE;

ResourceDataToSort::ResourceDataToSort()
{}

ResourceDataToSort::ResourceDataToSort(const string& hostname,
                                       unsigned int nbOfNodes,
                                       unsigned int nbOfProcPerNode,
                                       unsigned int CPUFreqMHz,
                                       unsigned int memInMB):
    _hostName(hostname),
    _nbOfNodes(nbOfNodes),
    _nbOfProcPerNode(nbOfProcPerNode),
    _CPUFreqMHz(CPUFreqMHz),
    _memInMB(memInMB)
{}

//! Method used by list::sort to sort the resources used in SALOME_ResourcesManager::GetResourcesFitting
bool ResourceDataToSort::operator< (const ResourceDataToSort& other) const
  {
    unsigned int nbPts = GetNumberOfPoints();
    return nbPts < other.GetNumberOfPoints();
  }

unsigned int ResourceDataToSort::GetNumberOfPoints() const
  {
    unsigned int ret = 0;
    //priority 1 : Nb of nodes

    if (_nbOfNodesWanted != NULL_VALUE)
      {
        if (_nbOfNodes == _nbOfNodesWanted)
          ret += 3000;
        else if (_nbOfNodes > _nbOfNodesWanted)
          ret += 2000;
        else
          ret += 1000;
      }

    //priority 2 : Nb of proc by node
    if (_nbOfProcPerNodeWanted != NULL_VALUE)
      {
        if (_nbOfProcPerNode == _nbOfProcPerNodeWanted)
          ret += 300;
        else if (_nbOfProcPerNode > _nbOfProcPerNodeWanted)
          ret += 200;
        else
          ret += 100;
      }

    //priority 3 : Cpu freq
    if (_CPUFreqMHzWanted != NULL_VALUE)
      {
        if (_CPUFreqMHz == _CPUFreqMHzWanted)
          ret += 30;
        else if (_CPUFreqMHz > _CPUFreqMHzWanted)
          ret += 20;
        else
          ret += 10;
      }

    //priority 4 : memory
    if (_memInMBWanted != NULL_VALUE)
      {
        if (_memInMB == _memInMBWanted)
          ret += 3;
        else if (_memInMB > _memInMBWanted)
          ret += 2;
        else
          ret += 1;
      }

    return ret;
  }

//! Method used for debug
void ResourceDataToSort::Print() const
  {
    SCRUTE(_nbOfNodes);
    SCRUTE(_nbOfProcPerNode);
    SCRUTE(_CPUFreqMHz);
    SCRUTE(_memInMB);
  }

void ParserResourcesType::Print() const
{
  ostringstream oss;
  oss << endl <<
    "HostName : " << DataForSort._hostName << endl << 
    "Alias : " << Alias << endl <<
    "NbOfNodes : " << DataForSort._nbOfNodes << endl <<
    "NbOfProcPerNode : " << DataForSort._nbOfProcPerNode << endl <<
    "CPUFreqMHz : " << DataForSort._CPUFreqMHz << endl <<
    "MemInMB : " << DataForSort._memInMB << endl <<
    "Protocol : " << Protocol << endl <<
    "Mode : " << Mode << endl <<
    "Batch : " << Batch << endl <<
    "mpi : " << mpi << endl <<
    "UserName : " << UserName << endl <<
    "AppliPath : " << AppliPath << endl <<
    "OS : " << OS << endl <<
    "Modules : " << endl;

  for(int i=0;i<ModulesList.size();i++)
    oss << "Module " << i+1 << " called : " << ModulesList[i] << endl;

  MESSAGE(oss.str());

}

void ParserResourcesType::Clear()
{
  DataForSort._hostName = "";
  DataForSort._nbOfNodes = 1;
  DataForSort._nbOfProcPerNode = 1;
  DataForSort._CPUFreqMHz = 0;
  DataForSort._memInMB = 0;
  Alias = "";
  Protocol = rsh;
  Mode = interactive;
  Batch = none;
  mpi = indif;
  UserName = "";
  AppliPath = "";
  ModulesList.clear();
  OS = "";
}
