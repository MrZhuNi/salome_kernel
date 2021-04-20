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

#include "SALOME_Fake_NamingService.hxx"
#include "Utils_SALOME_Exception.hxx"
#include "SALOME_KernelORB.hxx"

#include CORBA_CLIENT_HEADER(SALOME_Component)

#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>

std::mutex SALOME_Fake_NamingService::_mutex;
std::map<std::string,CORBA::Object_var> SALOME_Fake_NamingService::_map;
bool SALOME_Fake_NamingService::_log_container_file_thread_launched = false;
std::string SALOME_Fake_NamingService::_log_container_file_name;

SALOME_Fake_NamingService::SALOME_Fake_NamingService(CORBA::ORB_ptr orb)
{
}

std::vector< std::string > SALOME_Fake_NamingService::repr()
{
  std::lock_guard<std::mutex> g(_mutex);
  std::vector< std::string > ret;
  for(auto it : _map)
  {
    ret.push_back( it.first );
  }
  return ret;
}

void SALOME_Fake_NamingService::init_orb(CORBA::ORB_ptr orb)
{
}

void SALOME_Fake_NamingService::Register(CORBA::Object_ptr ObjRef, const char* Path)
{
  std::lock_guard<std::mutex> g(_mutex);
  CORBA::Object_var ObjRefAuto = CORBA::Object::_duplicate(ObjRef);
  _map[Path] = ObjRefAuto;
}

void SALOME_Fake_NamingService::Destroy_Name(const char* Path)
{
}

void SALOME_Fake_NamingService::Destroy_Directory(const char* Path)
{
}

void SALOME_Fake_NamingService::Destroy_FullDirectory(const char* Path)
{
}

bool SALOME_Fake_NamingService::Change_Directory(const char* Path)
{
  return true;
}

std::vector<std::string> SALOME_Fake_NamingService::list_subdirs()
{
  return std::vector<std::string>();
}

std::vector<std::string> SALOME_Fake_NamingService::list_directory()
{
  return std::vector<std::string>();
}

std::vector<std::string> SALOME_Fake_NamingService::list_directory_recurs()
{
  return std::vector<std::string>();
}

CORBA::Object_ptr SALOME_Fake_NamingService::Resolve(const char* Path)
{
  std::lock_guard<std::mutex> g(_mutex);
  std::string pathCpp(Path);
  auto it = _map.find(pathCpp);
  if( it != _map.end() )
    return CORBA::Object::_duplicate((*it).second);
  return CORBA::Object::_nil();
}

CORBA::Object_ptr SALOME_Fake_NamingService::ResolveFirst(const char* Path)
{
  return CORBA::Object::_nil();
}

SALOME_NamingService_Abstract *SALOME_Fake_NamingService::clone()
{
  return new SALOME_Fake_NamingService;
}

CORBA::Object_ptr SALOME_Fake_NamingService::ResolveComponent(const char* hostname, const char* containerName, const char* componentName, const int nbproc)
{
  std::ostringstream oss;
  oss << SEP << "Containers" << SEP << hostname << SEP << containerName << SEP << componentName;
  std::string entryToFind(oss.str());
  return Resolve(entryToFind.c_str());
}

std::vector<Engines::Container_var> SALOME_Fake_NamingService::ListOfContainersInNS()
{
  std::lock_guard<std::mutex> g(_mutex);
  std::vector<Engines::Container_var> ret;
  for(auto it : _map)
  {
    Engines::Container_var elt = Engines::Container::_narrow(it.second);
    if(!CORBA::is_nil(elt))
      ret.push_back(elt);
  }
  return ret;
}

std::string SALOME_Fake_NamingService::DumpInFileIORS()
{
  std::vector<Engines::Container_var> conts( ListOfContainersInNS() );
  std::ostringstream oss;
  CORBA::ORB_ptr orb = KERNEL::getORB();
  char SEP[2] = { '\0', '\0'};
  for(auto it : conts)
  {
    CORBA::String_var ior(orb->object_to_string(it));
    oss << SEP << ior;
    SEP[0] = '\n';
  }
  return oss.str();
}

void WriteContinuously(const std::string& logFileName)
{
  while(true)
  {
    std::chrono::milliseconds delta( 2000 );
    std::this_thread::sleep_for( delta );
    std::string content(SALOME_Fake_NamingService::DumpInFileIORS());
    { 
      std::ofstream ofs(logFileName);
      ofs.write(content.c_str(),content.length());
    }
  }
}

void SALOME_Fake_NamingService::LaunchLogContainersFile(const std::string& logFileName)
{
  if(_log_container_file_thread_launched)
    THROW_SALOME_EXCEPTION("SALOME_Fake_NamingService::LaunchLogContainersFile : Thread lready launched !");
  _log_container_file_name = logFileName;
  std::thread t(WriteContinuously,logFileName);
  t.detach();
}

std::string SALOME_Fake_NamingService::GetLogContainersFile()
{
  return _log_container_file_name;
}
