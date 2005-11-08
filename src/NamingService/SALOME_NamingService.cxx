//  SALOME NamingService : wrapping NamingService services
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
//  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org
//
//
//
//  File   : SALOME_NamingService.cxx
//  Author : Estelle Deville
//  Module : SALOME
//  $Header$

#include "SALOME_NamingService.hxx"
#include "ServiceUnreachable.hxx"

#include "utilities.h"

#include <iostream>
#include <cstring>
#include <string>

using namespace std;

// ============================================================================
/*! \brief Default Constructor without ORB reference.
 *
 *  After Default Constructor, one needs to initialize ORB.
 *  \sa init_orb(CORBA::ORB_ptr orb), SALOME_NamingService(CORBA::ORB_ptr orb)
 */ 
// ============================================================================

SALOME_NamingService::SALOME_NamingService()
{
  MESSAGE("SALOME_NamingService default constructor");
  _orb = CORBA::ORB::_nil();
  _root_context = CosNaming::NamingContext::_nil();
}

// ============================================================================
/*! \brief Standard Constructor, with ORB reference. 
 * 
 * Initializes the naming service root context
 * \param orb CORBA::ORB_ptr arguments
 */ 
// ============================================================================

SALOME_NamingService::SALOME_NamingService(CORBA::ORB_ptr orb)
{
  MESSAGE("SALOME_NamingService creation");
  _orb = orb ;
  _initialize_root_context();
}

// ============================================================================
/*! \brief Standard destructor.
 *
 *  The standard destructor does nothing special.
 */ 
// ============================================================================

SALOME_NamingService::~SALOME_NamingService()
{
  // Problem MESSAGE with singleton: late destruction,
  // after trace system destruction ?
  //MESSAGE("SALOME_NamingService destruction");
}

// ============================================================================
/*! \brief initializes ORB reference and naming service root context.
 * 
 *  Initializes ORB reference and naming service root context.
 *  For use after default constructor.
 *  \param orb CORBA::ORB_ptr arguments
 */ 
// ============================================================================

void SALOME_NamingService::init_orb(CORBA::ORB_ptr orb)
{
  MESSAGE("SALOME_NamingService initialisation");

  Utils_Locker lock (&_myMutex);
  _orb = orb;

  _initialize_root_context();
}


// ============================================================================
/*! \brief Registers a CORBA object reference under a path.
 *
 * Registers a CORBA object reference under a path.
 * If the NamingService is out, the exception ServiceUnreachable is thrown.
 * \param ObjRef CORBA object reference to associate to the path.
 * \param Path   A relative or absolute pathname to store the object reference.
 *               If the pathname begins with a '/', pathname is taken
 *               as an absolute pathname. Else, pathname is taken as a relative
 *               path, to current context. Prefer absolute pathname, relative
 *               pathname are not safe, when SALOME_NamingService object is
 *               shared or use in multithreaded context.   
 * \sa           Change_Directory(const char* Path),
 *               CORBA::Object_ptr Resolve(const char* Path)
 */ 
// ============================================================================

void SALOME_NamingService::Register(CORBA::Object_ptr ObjRef,
                                    const char* Path)
  throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Register: " << Path);

  Utils_Locker lock (&_myMutex);
  int dimension_Path = strlen(Path) + 1;

  char** resultat_resolve_Path = new char * [dimension_Path];

  // --- _current_context is replaced to the _root_context
  //     if the Path begins whith '/'

  if (Path[0] == '/')
    {
      _current_context = _root_context;
    }

  // --- the resolution of the directory path has to be done
  //      to place the current_context to the correct node

  int dimension_resultat = 0;

  _result_resolve_Path (Path, dimension_resultat, resultat_resolve_Path);

  CosNaming::Name _context_name;
  CORBA::Boolean _not_exist = false ;

  CosNaming::NamingContext_var _temp_context;

  if (dimension_resultat > 1)
    {
      // A directory is treated (not only an object name)
      // test if the directory where ObjRef should be recorded already exists
      // If not, create the new context

      _context_name.length(dimension_resultat - 1);
      _create_context_name_dir(resultat_resolve_Path,
			       dimension_resultat - 1,
                               _context_name);

      try
        {
          CORBA::Object_var _obj = _current_context->resolve(_context_name);
          _current_context = CosNaming::NamingContext::_narrow(_obj);
        }

      catch (CosNaming::NamingContext::NotFound &)
        {
          // --- failed to resolve, therefore assume cold start
          _not_exist = true;
        }

      catch (CosNaming::NamingContext::InvalidName &)
        {
          INFOS("Register() : CosNaming::NamingContext::InvalidName");
        }

      catch (CosNaming::NamingContext::CannotProceed &)
        {
          INFOS("Register() : CosNaming::NamingContext::CannotProceed");
        }

      catch (CORBA::SystemException&)
        {
          INFOS("Register() : CORBA::SystemException: "
                << "unable to contact the naming service");
          throw ServiceUnreachable();
        }

      if (_not_exist)
        {
          try
            {
              _context_name.length(1);
              for (int i = 0 ; i < dimension_resultat - 1 ;i++)
                {
                  _context_name[0].id =
                    CORBA::string_dup(resultat_resolve_Path[i]);
                  _context_name[0].kind = CORBA::string_dup("dir");
                  // SCRUTE(_context_name[0].id);
                  // --- check if the path is created
                  try
                    {
                      // --- if the context is already created, nothing to do
                      CORBA::Object_var _obj =
                        _current_context->resolve(_context_name);
                      _current_context =
                        CosNaming::NamingContext::_narrow(_obj);
                    }

                  catch (CosNaming::NamingContext::NotFound &)
                    {
                      // --- the context must be created
                      _temp_context =
                        _current_context->bind_new_context(_context_name);
                      _current_context = _temp_context;
                    }
                }
            }

          catch (CosNaming::NamingContext::AlreadyBound&)
            {
              INFOS("Register() : CosNaming::NamingContext::AlreadyBound");
            }

          catch (CosNaming::NamingContext::NotFound& ex)
            {
              CosNaming::Name n = ex.rest_of_name;

              if (ex.why == CosNaming::NamingContext::missing_node)
                INFOS("Register() : " << (char *) n[0].id
                      << " (" << (char *) n[0].kind << ") not found");

              if (ex.why == CosNaming::NamingContext::not_context)
                INFOS("Register() : " << (char *) n[0].id
                      << " (" << (char *) n[0].kind
                      << ") is not a context");

              if (ex.why == CosNaming::NamingContext::not_object)
                INFOS("Register() : " << (char *) n[0].id
                      << " (" << (char *) n[0].kind
                      << ") is not an object");
            }

          catch (CosNaming::NamingContext::CannotProceed&)
            {
              INFOS("Register(): CosNaming::NamingContext::CannotProceed");
            }

          catch (CosNaming::NamingContext::InvalidName&)
            {
              INFOS("Register(): CosNaming::NamingContext::InvalidName");
            }

          catch (CORBA::SystemException&)
            {
              INFOS("Register():CORBA::SystemException: "
                    << "unable to contact the naming service");
              throw ServiceUnreachable();
            }
        }
    }

  // --- The current directory is now the directory where the object should
  //     be recorded

  _context_name.length(1);

  try
    {
      // --- the last element is an object and not a directory

      _context_name[0].id =
        CORBA::string_dup(resultat_resolve_Path[dimension_resultat - 1]);
      _context_name[0].kind = CORBA::string_dup("object");
      //SCRUTE(_context_name[0].id);

      _current_context->bind(_context_name, ObjRef);
    }

  catch (CosNaming::NamingContext::NotFound& ex)
    {
      CosNaming::Name n = ex.rest_of_name;

      if (ex.why == CosNaming::NamingContext::missing_node)
        INFOS("Register() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind << ") not found");

      if (ex.why == CosNaming::NamingContext::not_context)
        INFOS("Register() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind
              << ") is not a context");

      if (ex.why == CosNaming::NamingContext::not_object)
        INFOS("Register() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind
              << ") is not an object");
    }

  catch (CosNaming::NamingContext::CannotProceed&)
    {
      INFOS("Register(): CosNaming::NamingContext::CannotProceed");
    }

  catch (CosNaming::NamingContext::InvalidName&)
    {
      INFOS("Register(): CosNaming::NamingContext::InvalidName");
    }

  catch (CosNaming::NamingContext::AlreadyBound&)
    {
      INFOS("Register(): CosNaming::NamingContext::AlreadyBound, "
	    << "object will be rebind");
      _current_context->rebind(_context_name, ObjRef);
    }

  catch (CORBA::SystemException&)
    {
      INFOS("!!!Register(): CORBA::SystemException: "
            << "unable to contact the naming service");
      throw ServiceUnreachable();
    }

  // --- clean memory

  for (int i = 0 ; i < dimension_resultat ;i++)
    {
      delete [] resultat_resolve_Path[i];
    }

  delete[] resultat_resolve_Path ;
}

// ============================================================================
/*! \brief get the CORBA object reference associated to a name.
 * 
 *  get the CORBA object reference associated to a complete name with a path.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown 
 * \param Path pathname. If the pathname begins with a '/', pathname is taken
 *             as an absolute pathname. Else, pathname is taken as a relative
 *             path, to current context. Prefer absolute pathname, relative
 *             pathname are not safe, when SALOME_NamingService object is
 *             shared or use in multithreaded context.
 * \return the object reference if it exists under the pathname,
 *             or nil reference in other cases.
 * \sa         Register(CORBA::Object_ptr ObjRef, const char* Path),
 *             Change_Directory(const char* Path)
 */ 
// ============================================================================

CORBA::Object_ptr SALOME_NamingService::Resolve(const char* Path)
  throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Resolve: " << Path);

  Utils_Locker lock (&_myMutex);
  int dimension_Path = strlen(Path) + 1;

  char** resultat_resolve_Path = new char * [dimension_Path];

  // --- _current_context is replaced to the _root_context
  //     if the Path begins whith '/'

  if (Path[0] == '/')
    _current_context = _root_context;


  // --- the resolution of the directory path has to be done
  //     to place the current_context to the correct node

  int dimension_resultat = 0;

  _result_resolve_Path (Path, dimension_resultat, resultat_resolve_Path);

  CosNaming::Name _context_name;
  _context_name.length(dimension_resultat);

  CORBA::Object_ptr _obj =  CORBA::Object::_nil();

  _create_context_name_dir(resultat_resolve_Path,
			   dimension_resultat - 1,
                           _context_name);

  // --- the last element is an object and not a directory

  _context_name[dimension_resultat - 1].id =
    CORBA::string_dup(resultat_resolve_Path[dimension_resultat - 1]);

  _context_name[dimension_resultat - 1].kind = CORBA::string_dup("object");

  ASSERT(!CORBA::is_nil(_current_context));

  // --- Context creation

  try
    {
      _obj = _current_context->resolve(_context_name);
    }

  catch (CosNaming::NamingContext::NotFound& ex)
    {
      CosNaming::Name n = ex.rest_of_name;

      if (ex.why == CosNaming::NamingContext::missing_node)
        INFOS("Resolve() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind << ") not found");

      if (ex.why == CosNaming::NamingContext::not_context)
        INFOS("Resolve() : "
              << (char *) n[0].id << " (" << (char *) n[0].kind
              << ") is not a context");

      if (ex.why == CosNaming::NamingContext::not_object)
        INFOS("Resolve() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind
              << ") is not an object");
    }

  catch (CosNaming::NamingContext::CannotProceed&)
    {
      INFOS("Resolve(): CosNaming::NamingContext::CannotProceed");
    }

  catch (CosNaming::NamingContext::InvalidName&)
    {
      INFOS("Resolve(): CosNaming::NamingContext::InvalidName");
    }

  catch (CORBA::SystemException&)
    {
      INFOS("Resolve():CORBA::SystemException : unable to contact"
            << "the naming service");
      throw ServiceUnreachable();
    }

  // --- clean memory
  for (int i = 0 ; i < dimension_resultat ;i++)
    {
      delete [] resultat_resolve_Path[i];
    }

  delete[] resultat_resolve_Path ;

  return _obj;
}

// ============================================================================
/*! \brief get the CORBA object reference associated to an uncomplete name.
 *
 *  get the CORBA object reference associated to an uncomplete name with a
 *  path. Look for the first occurence of name*.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown 
 * \param Path pathname under the form "/path/name" (Absolute reference !)
 *             search the fist reference like "/path(.dir)/name*(.kind)"
 * \return     the object reference if found, or nil reference.
 * \sa         Resolve(const char* Path)
 */
// ============================================================================

CORBA::Object_ptr SALOME_NamingService::ResolveFirst(const char* Path)
  throw(ServiceUnreachable)
{
  MESSAGE("ResolveFirst");

  Utils_Locker lock (&_myMutex);
  SCRUTE(Path);

  string thePath = Path;
  string basePath = "/";
  string name = thePath;

  string::size_type idx = thePath.rfind('/');

  if (idx != string::npos) // at least one '/' found
    {
      basePath = thePath.substr(0, idx);
      name = thePath.substr(idx + 1);
      SCRUTE(basePath);
    }

  SCRUTE(name);
  CORBA::Object_ptr obj = CORBA::Object::_nil();

  bool isOk = Change_Directory(basePath.c_str());
  
  if (isOk)
    {
      vector<string> listElem = list_directory();
      vector<string>::iterator its = listElem.begin();
      
      while (its != listElem.end())
	{
	  MESSAGE(*its);
	  
	  if ((*its).find(name) == 0)
	    {
	      return Resolve((*its).c_str());
	    }
	  
	  its++;
	}
    }

  return obj;
}

// ============================================================================
/*! \brief find a component instance from hostname, containername,
 *         componentName and number of processors.
 *
 *  find a component instance from hostname, containername, componentName and
 *  number of processors.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown 
 * \param hostname      name of the machine on which the component is searched.
 * \param containername name of the container in which the component is
                        instanciated.
 * \param componentname name of the component we are looking for an existing 
                        instance.
 * \param nbproc        in case of multi processor machine, container name is
 *                      suffixed with _nbproc.
 * \return the object reference
 */ 
// ============================================================================

CORBA::Object_ptr
SALOME_NamingService::ResolveComponent(const char* hostname,
				       const char* containerName,
				       const char* componentName,
				       const int nbproc)
  throw(ServiceUnreachable)
{
  MESSAGE("ResolveComponent");

  Utils_Locker lock (&_myMutex);

  string name = "/Containers/";

  name += hostname;

  if ( strlen(containerName) != 0 )
    {
      name += "/";

      if ( nbproc >= 1 )
        {
          char *newContainerName = new char[strlen(containerName) + 8];
          sprintf(newContainerName, "%s_%d", containerName, nbproc);
          name += newContainerName;
	  delete [] newContainerName;
        }

      else
        name += containerName;

      name += "/";

      name += componentName;

      return ResolveFirst(name.c_str());
    }

  else
    {
      Change_Directory(name.c_str());
      vector<string> contList = list_directory();

      for (unsigned int ind = 0; ind < contList.size(); ind++)
        {
          name = contList[ind].c_str();
          name += "/";
          name += componentName;
          CORBA::Object_ptr obj = ResolveFirst(name.c_str());

          if ( !CORBA::is_nil(obj) )
            return obj;
        }

      return CORBA::Object::_nil();
    }
}

// ============================================================================
/*! \brief provide a default container name if empty.
 *
 *  the given container name is returned unchanged, unless it is empty.
 * \param  containerName
 * \return container name, where empty input is replaced by "FactoryServer"
 */
// ============================================================================

string SALOME_NamingService::ContainerName(const char *containerName)
{
  string ret;

  if (strlen(containerName) == 0)
    ret = "FactoryServer";
  else
    ret = containerName;

  return ret;
}

// ============================================================================
/*! \brief build a container name, given a MachineParameters struct.
 *
 *  Build a container name with a MachineParameters struct. In case of multi
 *  processor machine, container name is suffixed with _nbproc.
 * \param MachineParameters struct from which we get container name (may be
 *                          empty),  number of nodes and number of processor
 *                          per node.
 * \return a container name
 */
// ============================================================================

string 
SALOME_NamingService::ContainerName(const Engines::MachineParameters& params)
{
  int nbproc;

  if ( !params.isMPI )
    nbproc = 0;
  else if ( (params.nb_node <= 0) && (params.nb_proc_per_node <= 0) )
    nbproc = 1;
  else if ( params.nb_node == 0 )
    nbproc = params.nb_proc_per_node;
  else if ( params.nb_proc_per_node == 0 )
    nbproc = params.nb_node;
  else
    nbproc = params.nb_node * params.nb_proc_per_node;

  string ret = ContainerName(params.container_name);

  if ( nbproc >= 1 )
    {
      char *suffix = new char[8];
      sprintf(suffix, "_%d", nbproc);
      ret += suffix;
    }

  return ret;
}

// ============================================================================
/*! 
 *
 */
// ============================================================================

string SALOME_NamingService::BuildContainerNameForNS(const char *containerName,
						     const char *hostname)
{
  string ret = "/Containers/";
  ret += hostname;
  ret += "/";
  ret += ContainerName(containerName);

  return ret;
}

// ============================================================================
/*! 
 *
 */
// ============================================================================

string
SALOME_NamingService::
BuildContainerNameForNS(const Engines::MachineParameters& params,
			const char *hostname)
{
  string ret = "/Containers/";
  ret += hostname;
  ret += "/";
  ret += ContainerName(params);

  return ret;
}

// ============================================================================
/*!  Function : Find
 *  Purpose  : method to research a name from the current directory 
 *            of the naming service. 
 *  The naming service changes directory to go to the directory where 
 *  the last occurence was found.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown
 *  \param name const char* arguments
 *  \return the number of occurences found
 *  \sa _Find
 */ 
// ============================================================================

int SALOME_NamingService::Find(const char* name)
throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Find " << name);

  Utils_Locker lock (&_myMutex)

    ;
  CORBA::Long occurence_number = 0 ;

  try
    {
      _Find(name, occurence_number);
    }

  catch (CORBA::SystemException&)
    {
      INFOS("!!!Find() : CORBA::SystemException : unable to contact"
            << " the naming service");
      throw ServiceUnreachable();
    }

  return occurence_number;
}

// ============================================================================
/*! Function : Create_Directory
 *  Purpose  : method to create a directory from the current directory.
 * If the NamingService is out, the exception ServiceUnreachable is thrown
 *  \param Path const char* arguments
 *  \return a boolean to indicate if the creation succeeded
 */ 
// ============================================================================

bool SALOME_NamingService::Create_Directory(const char* Path)
throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Create_Directory");

  Utils_Locker lock (&_myMutex)

    ;
  int dimension_Path = strlen(Path) + 1;

  char** resultat_resolve_Path = new char * [dimension_Path];

  ;
  CORBA::Boolean _return_code = true ;

  // _current_context is replaced to the _root_context
  // if the Path begins whith '/'
  if (Path[0] == '/')
    _current_context = _root_context;

  int dimension_resultat = 0;

  _result_resolve_Path (Path, dimension_resultat, resultat_resolve_Path);


  // We had to test if a part of the directory to treat
  // is already done
  // If not, the new context has to be created

  CosNaming::Name _context_name;

  _context_name.length(1);

  CosNaming::NamingContext_var _temp_context;

  ASSERT(!CORBA::is_nil(_current_context));

  // Context creation
  try
    {

      for (int i = 0 ; i < dimension_resultat ;i++)
        {
          _context_name[0].id =
            CORBA::string_dup(resultat_resolve_Path[i]);
          _context_name[0].kind = CORBA::string_dup("dir");
          // SCRUTE(_context_name[0].id);
          //The Path could be in part already created.
          //We had to test it

          try
            {
              // this context is already created.
              // Nothing to be done
              CORBA::Object_var _obj =
                _current_context->resolve(_context_name);
              _current_context =
                CosNaming::NamingContext::_narrow(_obj);
              MESSAGE("This context was already created");
            }

          catch (CosNaming::NamingContext::NotFound &)
            {
              // This context is not created. It will be done
              _temp_context =
                _current_context->bind_new_context(_context_name);
              _current_context = _temp_context;
              INFOS("This context was'nt created, it's now done");
            }
        }
    }

  catch (CosNaming::NamingContext::AlreadyBound&)
    {
      INFOS("!!! Create_Directory() CosNaming::NamingContext::AlreadyBound");
      _return_code = false;
    }

  catch (CosNaming::NamingContext::NotFound& ex)
    {
      _return_code = false;
      CosNaming::Name n = ex.rest_of_name;

      if (ex.why == CosNaming::NamingContext::missing_node)
        INFOS("Create_Directory() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind << ") not found");

      if (ex.why == CosNaming::NamingContext::not_context)
        INFOS("Create_Directory() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind
              << ") is not a context");

      if (ex.why == CosNaming::NamingContext::not_object)
        INFOS("Create_Directory() : " << (char *) n[0].id
              << " (" << (char *) n[0].kind
              << ") is not an object");
    }

  catch (CosNaming::NamingContext::CannotProceed&)
    {
      _return_code = false;
      INFOS("!!!Create_Directory():CosNaming::NamingContext::CannotProceed");
    }

  catch (CosNaming::NamingContext::InvalidName&)
    {
      _return_code = false;
      INFOS("!!!Create_Directory():CosNaming::NamingContext::InvalidName");
    }

  catch (CORBA::SystemException&)
    {
      _return_code = false;
      INFOS("!!!Register() :CORBA::SystemException : unable to contact"
            << " the naming service");
      throw ServiceUnreachable();
    }

  // Memory destruction
  for (int i = 0 ; i < dimension_resultat;i++)
    {
      delete [] resultat_resolve_Path[i];
    }

  delete[] resultat_resolve_Path ;
  return _return_code;
}

// ============================================================================
/*! Function : Change_Directory
 *  Purpose  : method to change the current directory to the
 *             directory Path indicated in "in" Parameter.
 *  If Path ="/", the current directory changes to the root directory.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown.
 * \param Path const char* arguments
 * \return a boolean to indicate if the change succeeded
 */ 
// ============================================================================

bool SALOME_NamingService::Change_Directory(const char* Path)
throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Change_Directory " << Path);

  Utils_Locker lock (&_myMutex)

    ;
  int dimension_Path = strlen(Path) + 1;

  char** resultat_resolve_Path = new char * [dimension_Path];

  CORBA::Boolean _return_code = true ;

  // _current_context is replaced to the _root_context
  // if the Path begins whith '/'
  if (Path[0] == '/')
    _current_context = _root_context;

  if ((Path[0] == '/') && (dimension_Path == 2))
    {
      MESSAGE("Change_Directory is called to go to the root_context");
    }

  //nothing to de done, the change_dur is called to go to the root_context
  // no Path to resolve
  else
    //the resolution of the directory path has to be done
    //to place the currect_context to the correct node
    {
      int dimension_resultat = 0;
      _result_resolve_Path(Path, dimension_resultat, resultat_resolve_Path);

      CosNaming::Name _context_name;
      _context_name.length(dimension_resultat);
      CORBA::Object_var _obj;

      _create_context_name_dir(resultat_resolve_Path, dimension_resultat,
                               _context_name);

      ASSERT(!CORBA::is_nil(_current_context));
      // Context creation

      try
        {
          _obj = _current_context->resolve(_context_name);
          _current_context = CosNaming::NamingContext::_narrow(_obj);
          ASSERT(!CORBA::is_nil(_current_context))
        }

      catch (CosNaming::NamingContext::NotFound& ex)
        {
          _return_code = false;
          CosNaming::Name n = ex.rest_of_name;

          if (ex.why == CosNaming::NamingContext::missing_node)
            INFOS( "Change_Directory() : " << (char *) n[0].id
                   << " (" << (char *) n[0].kind << ") not found")
            if (ex.why == CosNaming::NamingContext::not_context)
              INFOS("Change_Directory() : " << (char *) n[0].id
                    << " (" << (char *) n[0].kind
                    << ") is not a context" )
              if (ex.why == CosNaming::NamingContext::not_object)
                INFOS( "Change_Directory() : " << (char *) n[0].id
                       << " (" << (char *) n[0].kind
                       << ") is not an object" )
              }

      catch (CosNaming::NamingContext::CannotProceed&)
        {
          _return_code = false;
          INFOS( "!!!Change_Directory() : CosNaming::NamingContext::CannotProceed" )
        }

      catch (CosNaming::NamingContext::InvalidName&)
        {
          _return_code = false;
          INFOS( "!!!Change_Directory() : CosNaming::NamingContext::InvalidName" )
        }

      catch (CORBA::SystemException&)
        {
          _return_code = false;
          INFOS( "!!!Change_Directory() :CORBA::SystemException : unable to contact"
                 << "the naming service")
          throw ServiceUnreachable();
        }

      // Memory destruction
      for (int i = 0 ; i < dimension_resultat ;i++)
        {
          delete [] resultat_resolve_Path[i];
        }

      delete[] resultat_resolve_Path ;
    }

  return _return_code;
}

// ============================================================================
/*! Function : Current_Directory
 *  Purpose  : method to get the current directory.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown
 * \return the path of the current_context
 * \sa  _current_directory
 */ 
// ============================================================================

char* SALOME_NamingService::Current_Directory()
throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Current_Directory");

  Utils_Locker lock (&_myMutex)

    ;

  CosNaming::NamingContext_var _ref_context = _current_context;

  int i = 0;

  int length_path = 0;

  char** result_path = new char * [50]; // 50 is it enough?


  // We go to the root_context to begin the search from the root
  _current_context = _root_context ;

  CORBA::Boolean _continue = true ;

  try
    {
      _current_directory(result_path, i, _ref_context, _continue );
    }

  catch (CORBA::SystemException&)
    {
      INFOS("!!!Current_Directory(): CORBA::SystemException : unable to contact"
            << " the naming service" )
      throw ServiceUnreachable();
    }

  for (int k = 0 ; k < i ;k++)
    {
      // We count the length of the char* + 1 for the '/' to separate
      // the directories
      length_path = length_path + strlen(result_path[k]) + 1;
    }

  char* return_Path = new char[length_path + 2];
  return_Path[0] = '/' ;
  return_Path[1] = '\0' ;
#ifndef WNT

  for (int k = 0 ; k < i ;k++)
#else

  for (k = 0 ; k < i ;k++)
#endif

    {
      //SCRUTE(result_path[k])
      strcat(return_Path, result_path[k]);
      strcat(return_Path, "/");
    }
  //SCRUTE(return_Path)
  _current_context = _ref_context ;

  return return_Path;
}

// ============================================================================
/*! Function : list
 *  Purpose  : method to list and print all the context contained from
 *            the current context
 *  If the NamingService is out, the exception ServiceUnreachable is thrown
 */ 
// ============================================================================

void SALOME_NamingService::list()
throw(ServiceUnreachable)
{
  MESSAGE("Begin of list");

  Utils_Locker lock (&_myMutex)

    ;
  CosNaming::BindingList_var _binding_list;

  CosNaming::BindingIterator_var _binding_iterator;

  unsigned long nb = 0 ; // for using only the BindingIterator to access the bindings

  CosNaming::Binding_var _binding ;

  CosNaming::NamingContext_var _ref_context = _current_context;

  _current_context->list(nb, _binding_list, _binding_iterator) ;

  while (_binding_iterator->next_one(_binding))
    {
      CosNaming::Name _bindingName = _binding->binding_name;

      if (_binding->binding_type == CosNaming::ncontext)
        {
          MESSAGE( "Context : " << _bindingName[0].id );

          try
            {
              Change_Directory(_bindingName[0].id);
            }

          catch (ServiceUnreachable&)
            {
              INFOS( "!!!list(): ServiceUnreachable" )
              throw ServiceUnreachable();
            }

          list();
          _current_context = _ref_context ;
        }

      else if (_binding->binding_type == CosNaming::nobject)
        {
          MESSAGE( "Object : " << _bindingName[0].id );
        }
    }

  _binding_iterator->destroy();
}

// ============================================================================
/*! Function : list_directory
 *  Purpose  : method to get all the contexts contained in the current 
 *             directory
 *             Get only objects, isn't iterative 
 *  If the NamingService is out, the exception ServiceUnreachable is thrown
 */ 
// ============================================================================
vector<string> SALOME_NamingService::list_directory()
throw(ServiceUnreachable)
{
  vector<string> _list ;
  _list.resize(0);
  CosNaming::BindingList_var _binding_list;
  CosNaming::BindingIterator_var _binding_iterator;
  unsigned long nb = 0 ; // for using only the BindingIterator to access the bindings
  CosNaming::Binding_var _binding ;
  CosNaming::NamingContext_var _ref_context = _current_context;
  _current_context->list(nb, _binding_list, _binding_iterator) ;

  if (_binding_iterator->_is_nil())
    return _list;

  while (_binding_iterator->next_one(_binding))
    {
      CosNaming::Name _bindingName = _binding->binding_name;

      if (_binding->binding_type == CosNaming::nobject)
        {
          _list.push_back(CORBA::string_dup(_bindingName[0].id));
        }
    }

  //for (unsigned int ind = 0; ind < _list.size(); ind++)
  //  MESSAGE("list_directory : Object : " << _list[ind]);

  _binding_iterator->destroy();

  return _list;
}

// ============================================================================
/*! Function : list_directory_recurs
 *  Purpose  : method to get all the contexts contained in the current 
 *             directory
 *             Get only objects and is recursive 
 *  If the NamingService is out, the exception ServiceUnreachable is thrown
 */ 
// ============================================================================

vector<string> SALOME_NamingService::list_directory_recurs()
throw(ServiceUnreachable)
{
  MESSAGE("list_directory_recurs");

  Utils_Locker lock (&_myMutex)

    ;
  vector<string> _list ;

  char *currentDir = Current_Directory();

  _list_directory_recurs(_list, 0, currentDir);

  delete [] currentDir;

  return _list;
}

// ============================================================================
/*! Function : Destroy_Name
 *  Purpose  : method to destroy an association Path-Object Reference.
 *             WARNING : The complete Path should be given.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown 
 * \param Path const char* arguments
 */ 
// ============================================================================

void SALOME_NamingService::Destroy_Name(const char* Path)
throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Destroy_Name");

  Utils_Locker lock (&_myMutex)

    ;
  int dimension_Path = strlen(Path) + 1;

  char** resultat_resolve_Path = new char * [dimension_Path];

  // _current_context is replaced to the _root_context
  // if the Path begins whith '/'
  if (Path[0] == '/')
    _current_context = _root_context;


  //the resolution of the directory path has to be done
  //to place the currect_context to the correct node
  int dimension_resultat = 0;

  _result_resolve_Path (Path, dimension_resultat, resultat_resolve_Path);

  CosNaming::Name _context_name;

  if (dimension_resultat > 1)
    {
      // We go in the directory where the object to destroy is
      _context_name.length(dimension_resultat - 1);

      _create_context_name_dir(resultat_resolve_Path, dimension_resultat - 1,
                               _context_name);

      try
        {
          CORBA::Object_var _obj = _current_context->resolve(_context_name);
          _current_context = CosNaming::NamingContext::_narrow(_obj);
        }

      catch (CosNaming::NamingContext::NotFound& ex)
        {
          CosNaming::Name n = ex.rest_of_name;

          if (ex.why == CosNaming::NamingContext::missing_node)
            INFOS( "Destroy_Name() : " << (char *) n[0].id
                   << " (" << (char *) n[0].kind << ") not found" )
            if (ex.why == CosNaming::NamingContext::not_context)
              INFOS( "Destroy_Name() : " << (char *) n[0].id
                     << " (" << (char *) n[0].kind
                     << ") is not a context" )
              if (ex.why == CosNaming::NamingContext::not_object)
                INFOS( "Destroy_Name() : " << (char *) n[0].id
                       << " (" << (char *) n[0].kind
                       << ") is not an object" )
              }

      catch (CosNaming::NamingContext::InvalidName &)
        {
          INFOS( "!!!Destroy_Name() : CosNaming::NamingContext::InvalidName" )
        }

      catch (CosNaming::NamingContext::CannotProceed &)
        {
          INFOS( "!!!Destroy_Name(): CosNaming::NamingContext::CannotProceed" )
        }

      catch (CORBA::SystemException&)
        {
          INFOS( "!!!Destroy_Name() : CORBA::SystemException : unable to contact"
                 << " the naming service")
          throw ServiceUnreachable();
        }
    }

  // the last element is the object to destroy
  _context_name.length(1);

  _context_name[0].id =
    CORBA::string_dup(resultat_resolve_Path[dimension_resultat - 1]);

  _context_name[0].kind = CORBA::string_dup("object");

  SCRUTE(_context_name[0].id);

  ASSERT(!CORBA::is_nil(_current_context));

  // Object destruction
  try
    {
      _current_context->unbind(_context_name);
      MESSAGE( "The object " << _context_name[0].id << " has been deleted" )
    }

  catch (CosNaming::NamingContext::NotFound& ex)
    {
      CosNaming::Name n = ex.rest_of_name;

      if (ex.why == CosNaming::NamingContext::missing_node)
        INFOS( "Destroy_Name() : " << (char *) n[0].id
               << " (" << (char *) n[0].kind << ") not found" )
        if (ex.why == CosNaming::NamingContext::not_context)
          INFOS( "Destroy_Name() : " << (char *) n[0].id
                 << " (" << (char *) n[0].kind
                 << ") is not a context" )
          if (ex.why == CosNaming::NamingContext::not_object)
            INFOS( "Destroy_Name() : " << (char *) n[0].id
                   << " (" << (char *) n[0].kind
                   << ") is not an object" )
          }

  catch (CosNaming::NamingContext::CannotProceed&)
    {
      INFOS( "!!!Destroy_Name() : CosNaming::NamingContext::CannotProceed")
    }

  catch (CosNaming::NamingContext::InvalidName&)
    {
      INFOS( "!!!Destroy_Name() : CosNaming::NamingContext::InvalidName")
    }

  catch (CORBA::SystemException&)
    {
      INFOS( "!!!Destroy_Name() :CORBA::SystemException : unable to contact"
             << " the naming service")
      throw ServiceUnreachable();
    }

  // Memory destruction
  for (int i = 0 ; i < dimension_resultat ;i++)
    {
      delete [] resultat_resolve_Path[i];
    }

  delete[] resultat_resolve_Path ;
}

// ============================================================================
/*! Function : Destroy_Directory.
 *  Purpose  : method to destroy a directory if it is empty.
 *  WARNING : The complete Path  to the directory (from the root_context)
 *  to destroy should be given.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown.
 * \param Path const char* arguments
 */ 
// ============================================================================

void SALOME_NamingService::Destroy_Directory(const char* Path)
throw(ServiceUnreachable)
{
  MESSAGE("BEGIN OF Destroy_Directory");

  Utils_Locker lock (&_myMutex)

    ;
  int dimension_Path = strlen(Path) + 1;

  char** resultat_resolve_Path = new char * [dimension_Path];

  // _current_context is replaced to the _root_context
  // if the Path begins whith '/'
  if (Path[0] == '/')
    _current_context = _root_context;

  CosNaming::NamingContext_var _ref_context = _current_context;

  //the resolution of the directory path has to be done
  //to place the currect_context to the correct node
  int dimension_resultat = 0;

  _result_resolve_Path (Path, dimension_resultat, resultat_resolve_Path);

  CosNaming::Name _context_name;

  if (dimension_resultat > 1)
    {
      // We go in the directory where the context to destroy is
      _context_name.length(dimension_resultat - 1);

      _create_context_name_dir(resultat_resolve_Path, dimension_resultat - 1,
                               _context_name);

      try
        {
          CORBA::Object_var _obj = _current_context->resolve(_context_name);
          _current_context = CosNaming::NamingContext::_narrow(_obj);
          _ref_context = _current_context ;
        }

      catch (CosNaming::NamingContext::NotFound& ex)
        {
          CosNaming::Name n = ex.rest_of_name;

          if (ex.why == CosNaming::NamingContext::missing_node)
            INFOS( "Destroy_Directory() : " << (char *) n[0].id
                   << " (" << (char *) n[0].kind << ") not found")
            if (ex.why == CosNaming::NamingContext::not_context)
              INFOS( "Destroy_Directory() : " << (char *) n[0].id
                     << " (" << (char *) n[0].kind
                     << ") is not a context" )
              if (ex.why == CosNaming::NamingContext::not_object)
                INFOS( "Destroy_Directory() : " << (char *) n[0].id
                       << " (" << (char *) n[0].kind
                       << ") is not an object" )
              }

      catch (CosNaming::NamingContext::InvalidName &)
        {
          INFOS( "!!!Destroy_Directory() : CosNaming::NamingContext::InvalidName" )
        }

      catch (CosNaming::NamingContext::CannotProceed &)
        {
          INFOS("!!!Destroy_Directory(): CosNaming::NamingContext::CannotProceed" )
        }

      catch (CORBA::SystemException&)
        {
          INFOS( "!!!Destroy_Directory() : CORBA::SystemException : unable to contact"
                 << " the naming service" )
          throw ServiceUnreachable();
        }
    }

  // the last element is the context to destroy
  _context_name.length(1);

  _context_name[0].id =
    CORBA::string_dup(resultat_resolve_Path[dimension_resultat - 1]);

  _context_name[0].kind = CORBA::string_dup("dir");

  SCRUTE(_context_name[0].id);

  try
    {
      // We go in the context to destroy
      CORBA::Object_var _obj = _current_context->resolve(_context_name);
      _current_context = CosNaming::NamingContext::_narrow(_obj);
    }

  catch (CosNaming::NamingContext::NotFound& ex)
    {
      CosNaming::Name n = ex.rest_of_name;

      if (ex.why == CosNaming::NamingContext::missing_node)
        INFOS( "Destroy_Directory() : " << (char *) n[0].id
               << " (" << (char *) n[0].kind << ") not found" )
        if (ex.why == CosNaming::NamingContext::not_context)
          INFOS( "Destroy_Directory() : " << (char *) n[0].id
                 << " (" << (char *) n[0].kind
                 << ") is not a context" )
          if (ex.why == CosNaming::NamingContext::not_object)
            INFOS( "Destroy_Directory() : " << (char *) n[0].id
                   << " (" << (char *) n[0].kind
                   << ") is not an object" )
          }

  catch (CosNaming::NamingContext::InvalidName &)
    {
      INFOS( "!!!Destroy_Directory() : CosNaming::NamingContext::InvalidName" )
    }

  catch (CosNaming::NamingContext::CannotProceed &)
    {
      INFOS( "!!!Destroy_Directory(): CosNaming::NamingContext::CannotProceed" )
    }

  catch (CORBA::SystemException&)
    {
      INFOS( "!!!Destroy_Directory() : CORBA::SystemException : unable to contact"
             << " the naming service" )
      throw ServiceUnreachable();
    }

  ASSERT(!CORBA::is_nil(_current_context));
  // Context Destruction

  try
    {
      _current_context->destroy();
      MESSAGE( "The context " << _context_name[0].id << " has been deleted" )
    }

  catch (CosNaming::NamingContext::NotEmpty&)
    {
      INFOS( "!!!Destroy_Directory() : CosNaming::NamingContext::NoEmpty "
             << Path << " is not empty" )
    }

  catch (CORBA::SystemException&)
    {
      INFOS( "!!!Destroy_Directory() :CORBA::SystemException : "
             << "unable to contact the naming service")
      throw ServiceUnreachable();
    }

  // We go to the directory just before the context to delete
  _current_context = _ref_context ;

  try
    {
      _current_context->unbind(_context_name);
      MESSAGE( "The bind to the context " << _context_name[0].id << " has been deleted" )
    }

  catch (CosNaming::NamingContext::NotFound& ex)
    {
      CosNaming::Name n = ex.rest_of_name;

      if (ex.why == CosNaming::NamingContext::missing_node)
        INFOS( "Destroy_Directory() : " << (char *) n[0].id
               << " (" << (char *) n[0].kind << ") not found" )
        if (ex.why == CosNaming::NamingContext::not_context)
          INFOS( "Destroy_Directory() : " << (char *) n[0].id
                 << " (" << (char *) n[0].kind
                 << ") is not a context" )
          if (ex.why == CosNaming::NamingContext::not_object)
            INFOS( "Destroy_Directory() : " << (char *) n[0].id
                   << " (" << (char *) n[0].kind
                   << ") is not an object" )
          }

  catch (CosNaming::NamingContext::CannotProceed&)
    {
      INFOS( "!!!Destroy_Directory() : CosNaming::NamingContext::CannotProceed")
    }

  catch (CosNaming::NamingContext::InvalidName&)
    {
      INFOS( "!!!Destroy_Directory() : CosNaming::NamingContext::InvalidName")
    }

  catch (CORBA::SystemException&)
    {
      INFOS( "!!!Destroy_Directory() :CORBA::SystemException : unable to contact"
             << " the naming service")
      throw ServiceUnreachable();
    }

  // Memory destruction
  for (int i = 0 ; i < dimension_resultat ;i++)
    {
      delete [] resultat_resolve_Path[i];
    }

  delete[] resultat_resolve_Path ;
}

// ============================================================================
/*! Function : Destroy_Directory.
 *  Purpose  : method to destroy a directory if it is empty.
 *  WARNING : The complete Path  to the directory (from the root_context)
 *  to destroy should be given.
 *  If the NamingService is out, the exception ServiceUnreachable is thrown.
 * \param Path const char* arguments
 */ 
// ============================================================================

void SALOME_NamingService::Destroy_FullDirectory(const char* Path)
throw(ServiceUnreachable)
{
  Change_Directory(Path);
  vector<string> contList = list_directory();

  for (unsigned int ind = 0; ind < contList.size(); ind++)
    Destroy_Name(contList[ind].c_str());

  Destroy_Directory(Path);

  Destroy_Name(Path);
}

// ============================================================================
/*! Function : _initialize_root_context
 * Purpose  :  method called by constructor to initialize _root_context
 */ 
// ============================================================================

void SALOME_NamingService::_initialize_root_context()
{
  //MESSAGE("Get the root context");

  try
    {
      CORBA::Object_var obj = _orb->resolve_initial_references("NameService");
      _root_context = CosNaming::NamingContext::_narrow(obj);
      _current_context = _root_context ;
      ASSERT(!CORBA::is_nil(_root_context));
    }

  catch (CORBA::SystemException&)
    {
      INFOS("CORBA::SystemException: unable to contact the naming service");
      throw ServiceUnreachable();
    }

  catch (...)
    {
      INFOS("Unknown Exception: unable to contact the naming service");
      throw ServiceUnreachable();
    }
}

// ============================================================================
/*! Function : _resolve_Path
 * Purpose  : method to decompose a Path : /Kernel/Services/Sessions.
 * 
 * \return a char* containing the first char between '/' (in this case Kernel)
 */ 
// ============================================================================

char* SALOME_NamingService::_resolve_Path(char* Path)
{
  int i = 0 ;
  int length = strlen(Path);
  char *resultat;

  if (length == 0)
    return NULL;
  else
    {
      while ((i < length) && (Path[i] != '/'))
        i++;

      resultat = new char[i + 1];

      strncpy(resultat, Path, i);

      resultat[i] = '\0';

      return resultat;
    }
}

// ============================================================================
/*! Function : _result_resolve_Path.
 *  Purpose  : method to decompose a Path : /Kernel/Services/Sessions.
 *  Gives an array of char* containing Kernel, Services, Sessions.
 * \param  Path const char* arguments, the Path to decompose
 * \param j int& arguments, the size of the array of char*
 * \param resultat_resolve_Path char** arguments
 */ 
// ============================================================================

void
SALOME_NamingService::_result_resolve_Path(const char* Path,
    int& j,
    char ** resultat_resolve_Path)
{
  //MESSAGE("BEGIN OF _result_resolve_Path");
  int dimension_Path = strlen(Path) + 1;
  char** temp = new char * [dimension_Path];
  char** tempslash = new char * [dimension_Path];

  temp[j] = new char[dimension_Path];
  strcpy(temp[j], Path);

  while (strlen(temp[j]) > 0)
    {
      // temp[j] contains the characters to be treated :
      //  (Path - characters already treted)
      // tempslash[j] = temp[j] if the string temp[j] doesn't begin whith '/'
      // tempslash[j] = temp[j] without '/' if the string begins whith '/'
      int length_temp = strlen(temp[j]);

      if (temp[j][0] == '/')
        {
          // the characters to be treated begin whith '/'
          // we don't have to take the '/'
          tempslash[j] = new char [length_temp] ;

          for (int k = 0; k < length_temp - 1; k++)
            tempslash[j][k] = temp[j][k + 1];

          tempslash[j][length_temp - 1] = '\0';
        }

      else
        {
          //the characters to be trated don't begin with '/'
          // Nothing to be done on the char
          tempslash[j] = new char [length_temp + 1] ;
          strcpy(tempslash[j], temp[j]);
        }

      // decomposition of the Path
      resultat_resolve_Path[j] = _resolve_Path(tempslash[j]);

      //SCRUTE(resultat_resolve_Path[j]);

      int length_resultat = strlen(resultat_resolve_Path[j]) ;

      int dimension_temp = length_temp - length_resultat ;

      j++;

      temp[j] = new char[dimension_temp + 1];

      for (int i = 0 ; i < dimension_temp ;i++)
        {
          temp[j][i] = tempslash[j - 1][i + length_resultat];
        }

      temp[j][dimension_temp] = '\0';
      //SCRUTE(temp[j]);
    }

  // Memory destruction
  for (int i = 0 ; i < j;i++)
    {
      delete [] temp[i];
      delete [] tempslash[i];
    }

  delete[] temp;
  delete [] tempslash ;
}

// ============================================================================
/*! Function : _Find.
 *  Purpose  : method to research a name from the current directory 
 *             of the naming service.   
 *  The naming service changes directory to go to the directory where 
 *  the last occurence was found.
 *  \param name const char* arguments
 *  \param occurence_number CORBA::LONG (by value)
 */ 
// ============================================================================

void SALOME_NamingService::_Find(const char* name,
                                 CORBA::Long& occurence_number)
{
  //MESSAGE("BEGIN OF _Find") SCRUTE(name);
  CosNaming::BindingList_var _binding_list;
  CosNaming::BindingIterator_var _binding_iterator;
  unsigned long nb = 0 ; //for using only the BindingIterator
  // to access the bindings
  CosNaming::Binding_var _binding ;
  CosNaming::NamingContext_var _ref_context = _current_context;
  CosNaming::NamingContext_var _found_context = _current_context;

  _current_context->list(nb, _binding_list, _binding_iterator) ;

  while (_binding_iterator->next_one(_binding))
    {
      CosNaming::Name _bindingName = _binding->binding_name;

      if (_binding->binding_type == CosNaming::ncontext)
        {
          // We work on a directory, the search should be done in this directory
          Change_Directory(_bindingName[0].id);
          _Find(name, occurence_number);
          // We'll go back to the initial context
          _current_context = _ref_context ;
        }

      else if (_binding->binding_type == CosNaming::nobject)
        {
          // We work on an object...

          if (!strcmp( _bindingName[0].id, name))
            {
              //MESSAGE("One occurence was found");
              occurence_number++;
              // We keep in memory the directory where one occurence was found
              _found_context = _current_context ;
            }
        }
    }

  _binding_iterator->destroy();
  // We go to the last directory where an occurence was found
  _current_context = _found_context ;
  //SCRUTE(occurence_number);
}

// ============================================================================
/*! Function : _create_context_name_dir.
 *  Purpose  : method to create a Context_name from an array of char.
 *             The number of elements to be copied are indicated 
 *             with lenth_copy.
 *
 * \param resultat_resolve_Path char** arguments
 * \param length_copy int arguments
 * \param _context_name CosNaming::Name arguments (by value)
 */ 
// ============================================================================

void
SALOME_NamingService::_create_context_name_dir(char** resultat_resolve_Path,
					       int length_copy,
					       CosNaming::Name& _context_name)
{
  //MESSAGE("BEGIN OF _create_context_name_dir");

  for (int i = 0 ; i < length_copy;i++)
    {
      _context_name[i].id = CORBA::string_dup(resultat_resolve_Path[i]);
      _context_name[i].kind = CORBA::string_dup("dir");
      //SCRUTE(_context_name[i].id);
    }
}

// ============================================================================
/*! Function : _current_directory.
 * Purpose  : method to parse the naming service tree to find a context
 *            and determine the path to go to this context from the 
 *            _root_context.
 *  \param result_path char** arguments
 *  \param length_result int arguments by value
 *  \param context_to_found CosNaming::NamingContext_var arguments
 *  \param _continue boolean arguments
 */ 
// ============================================================================

void
SALOME_NamingService::
_current_directory(char** result_path,
		   int& length_result,
		   CosNaming::NamingContext_var context_to_found,
		   CORBA::Boolean& _continue)
{
  //MESSAGE("BEGIN OF _current_Directory");
  CosNaming::BindingList_var _binding_list;
  CosNaming::BindingIterator_var _binding_iterator;
  unsigned long nb = 0 ; //for using only the BindingIterator
  // to access the bindings
  CosNaming::Binding_var _binding ;
  CosNaming::NamingContext_var _ref_context = _current_context;
  CosNaming::NamingContext_var _temp_context = _current_context;

  _current_context->list(nb, _binding_list, _binding_iterator) ;

  if ( !_binding_iterator->_is_nil() )
    {
      while ((_binding_iterator->next_one(_binding)) && _continue)
        {
          CosNaming::Name _bindingName = _binding->binding_name;

          if (_binding->binding_type == CosNaming::ncontext)
            {
              // We work on a directory, the search should be done in this directory

              result_path[length_result] = new char(strlen(_bindingName[0].id) + 1);
              strcpy(result_path[length_result], _bindingName[0].id);
              //SCRUTE(result_path[length_result])
              length_result++;

              CORBA::Object_var _obj = _current_context->resolve(_bindingName);
              _temp_context = CosNaming::NamingContext::_narrow(_obj);

              if (_temp_context->_is_equivalent(context_to_found))
                {
                  //MESSAGE("The context is found, we stop the search");
                  _continue = false;
                  //SCRUTE(_continue);
                }

              if (_continue)
                {
                  //SCRUTE(_bindingName[0].id);
                  Change_Directory(_bindingName[0].id);
                  _current_directory(result_path, length_result,
                                     context_to_found, _continue );

                  if (_continue)
                    {
                      // We'll go back to the initial context
                      _current_context = _ref_context ;
                      //MESSAGE("Just before the delete of ")
                      //SCRUTE(result_path[length_result-1]);
                      delete result_path[length_result - 1];
                      length_result--;
                    }
                }
            }
        }

      _binding_iterator->destroy();
    }

  // We go to the last directory where an occurence was found
  _current_context = _ref_context ;
}


// ============================================================================
/*! Function :_list_directory_recurs.
 * Purpose  : method to list recursively all the objects contained in the tree of absCurDirectory/relativeSubDir.
 *  \param myList The list that will be filled.
 *  \param relativeSubDir The directory from absCurDirectory in which the objects are found.
 *  \param absCurDirectory The directory in ABSOLUTE form.
 *  _current_context must refer to absCurDirectory.
 */ 
// ============================================================================

void SALOME_NamingService::_list_directory_recurs(vector<string>& myList,
						  const char *relativeSubDir,
						  const char *absCurDirectory)
{
  CosNaming::BindingList_var _binding_list;
  CosNaming::BindingIterator_var _binding_iterator;
  unsigned long nb = 0 ; // for using only the BindingIterator to access the bindings
  CosNaming::Binding_var _binding ;
  char *absDir;

  CosNaming::NamingContext_var _ref_context = _current_context;

  if (relativeSubDir)
    {
      Change_Directory(relativeSubDir);
      absDir = new char[strlen(absCurDirectory) + 2 + strlen(relativeSubDir)];
      strcpy(absDir, absCurDirectory);
      strcat(absDir, relativeSubDir);
      strcat(absDir, "/");
    }

  else
    absDir = (char *)absCurDirectory;

  _current_context->list(nb, _binding_list, _binding_iterator) ;

  while (_binding_iterator->next_one(_binding))
    {
      CosNaming::Name _bindingName = _binding->binding_name;

      if (_binding->binding_type == CosNaming::ncontext)
        {
          _list_directory_recurs(myList, _bindingName[0].id, absDir);
        }

      else if (_binding->binding_type == CosNaming::nobject)
        {
          char *elt = new char[strlen(absDir) + 2 + strlen(_bindingName[0].id)];
          strcpy(elt, absDir);
          strcat(elt, _bindingName[0].id);
          myList.push_back(elt);
          delete [] elt;
        }
    }

  if (relativeSubDir)
    {
      _current_context = _ref_context ;
      delete [] absDir;
    }

  _binding_iterator->destroy();
}

// ============================================================================
/*!
 *
 */
// ============================================================================

char * SALOME_NamingService::getIORaddr()
{
  return _orb->object_to_string(_root_context);
}

