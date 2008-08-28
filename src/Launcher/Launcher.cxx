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
#include "Batch_Date.hxx"
#include "Batch_FactBatchManager_eLSF.hxx"
#include "Batch_FactBatchManager_ePBS.hxx"
#include "Launcher.hxx"
#include "utilities.h"
#include <iostream>
#include <sstream>
#include <sys/stat.h>

using namespace std;

//=============================================================================
/*! 
 *  Constructor
 *  \param orb
 *  Define a CORBA single thread policy for the server, which avoid to deal
 *  with non thread-safe usage like Change_Directory in SALOME naming service
 */
//=============================================================================

Launcher_cpp::Launcher_cpp()
{
  MESSAGE ( "Launcher_cpp constructor" );
}

//=============================================================================
/*! 
 * destructor
 */
//=============================================================================

Launcher_cpp::~Launcher_cpp()
{
  MESSAGE ( "Launcher_cpp destructor" );
  std::map < string, Batch::BatchManager_eClient * >::const_iterator it1;
  for(it1=_batchmap.begin();it1!=_batchmap.end();it1++)
    delete it1->second;
  std::map < std::pair<std::string,long> , Batch::Job* >::const_iterator it2;
  for(it2=_jobmap.begin();it2!=_jobmap.end();it2++)
    delete it2->second;
}

//=============================================================================
/*! CORBA Method:
 *  Submit a batch job on a cluster and returns the JobId
 *  \param fileToExecute      : .py/.exe/.sh/... to execute on the batch cluster
 *  \param filesToExport      : to export on the batch cluster
 *  \param NumberOfProcessors : Number of processors needed on the batch cluster
 *  \param params             : Constraints for the choice of the batch cluster
 */
//=============================================================================
long Launcher_cpp::submitSalomeJob( const string fileToExecute ,
				    const vector<string>& filesToExport ,
				    const vector<string>& filesToImport ,
				    const batchParams& batch_params,
				    const machineParams& params) throw(LauncherException)
{
  MESSAGE ( "BEGIN OF Launcher_cpp::submitSalomeJob" );
  long jobId;
  vector<string> aMachineList;

  // check batch params
  if ( !check(batch_params) )
    throw LauncherException("Batch parameters are bad (see informations above)");

  // find a cluster matching the structure params
  vector<string> aCompoList ;
  try{
    aMachineList = _ResManager->GetFittingResources(params, aCompoList);
  }
  catch(const ResourcesException &ex){
    throw LauncherException(ex.msg.c_str());
  }
  if (aMachineList.size() == 0)
    throw LauncherException("No resources have been found with your parameters");

  ParserResourcesType p = _ResManager->GetResourcesList(aMachineList[0]);
  string clustername(p.Alias);
  MESSAGE ( "Choose cluster: " <<  clustername );
  
  // search batch manager for that cluster in map or instanciate one
  map < string, Batch::BatchManager_eClient * >::const_iterator it = _batchmap.find(clustername);
  if(it == _batchmap.end())
    {
      _batchmap[clustername] = FactoryBatchManager(p);
      // TODO: Add a test for the cluster !
    }
    
  try{
    // tmp directory on cluster to put files to execute
    string tmpdir = getTmpDirForBatchFiles();

    // create and submit job on cluster
    Batch::Parametre param;
    param[USER] = p.UserName;
    param[EXECUTABLE] = buildSalomeCouplingScript(fileToExecute,tmpdir,p);
    param[INFILE] = Batch::Couple( fileToExecute, getRemoteFile(tmpdir,fileToExecute) );
    for(int i=0;i<filesToExport.size();i++)
      param[INFILE] += Batch::Couple( filesToExport[i], getRemoteFile(tmpdir,filesToExport[i]) );


    ostringstream file_name_output;
    file_name_output << "~/" << tmpdir << "/" << "runSalome.output.log*";
    ostringstream file_name_error;
    file_name_error << "~/" << tmpdir << "/" << "runSalome.error.log*";
    ostringstream file_container_log;
    file_container_log << "~/" << tmpdir << "/" << "YACS_Server*";
    param[OUTFILE] = Batch::Couple( "", file_name_output.str());
    param[OUTFILE] += Batch::Couple( "", file_name_error.str());
    param[OUTFILE] += Batch::Couple( "", file_container_log.str());

    for(int i=0;i<filesToImport.size();i++)
      param[OUTFILE] += Batch::Couple( "", filesToImport[i] );

    param[NBPROC] = batch_params.nb_proc;
    param[WORKDIR] = batch_params.batch_directory;
    param[TMPDIR] = tmpdir;
    param[MAXWALLTIME] = getWallTime(batch_params.expected_during_time);
    param[MAXRAMSIZE] = getRamSize(batch_params.mem);
    param[HOMEDIR] = getHomeDir(p, tmpdir);

    Batch::Environnement env;

    Batch::Job* job = new Batch::Job(param,env);

    // submit job on cluster
    Batch::JobId jid = _batchmap[clustername]->submitJob(*job);

    // get job id in long
    istringstream iss(jid.getReference());
    iss >> jobId;

    _jobmap[ pair<string,long>(clustername,jobId) ] = job;
  }
  catch(const Batch::EmulationException &ex){
    throw LauncherException(ex.msg.c_str());
  }

  return jobId;
}

//=============================================================================
/*! CORBA Method:
 *  Query a batch job on a cluster and returns the status of job
 *  \param jobId              : identification of Salome job
 *  \param params             : Constraints for the choice of the batch cluster
 */
//=============================================================================
string Launcher_cpp::querySalomeJob( long id, 
				     const machineParams& params) throw(LauncherException)
{
  // find a cluster matching params structure
  vector<string> aCompoList ;
  vector<string> aMachineList = _ResManager->GetFittingResources( params , aCompoList ) ;
  ParserResourcesType p = _ResManager->GetResourcesList(aMachineList[0]);
  string clustername(p.Alias);
    
  // search batch manager for that cluster in map
  std::map < string, Batch::BatchManager_eClient * >::const_iterator it = _batchmap.find(clustername);
  if(it == _batchmap.end())
    throw LauncherException("no batchmanager for that cluster");
    
  ostringstream oss;
  oss << id;
  Batch::JobId jobId( _batchmap[clustername], oss.str() );

  Batch::JobInfo jinfo = jobId.queryJob();
  Batch::Parametre par = jinfo.getParametre();
  return par[STATE];
}

//=============================================================================
/*! CORBA Method:
 *  Delete a batch job on a cluster 
 *  \param jobId              : identification of Salome job
 *  \param params             : Constraints for the choice of the batch cluster
 */
//=============================================================================
void Launcher_cpp::deleteSalomeJob( const long id, 
				    const machineParams& params) throw(LauncherException)
{
  // find a cluster matching params structure
  vector<string> aCompoList ;
  vector<string> aMachineList = _ResManager->GetFittingResources( params , aCompoList ) ;
  ParserResourcesType p = _ResManager->GetResourcesList(aMachineList[0]);
  string clustername(p.Alias);
    
  // search batch manager for that cluster in map
  map < string, Batch::BatchManager_eClient * >::const_iterator it = _batchmap.find(clustername);
  if(it == _batchmap.end())
    throw LauncherException("no batchmanager for that cluster");
  
  ostringstream oss;
  oss << id;
  Batch::JobId jobId( _batchmap[clustername], oss.str() );

  jobId.deleteJob();
}

//=============================================================================
/*! CORBA Method:
 *  Get result files of job on a cluster
 *  \param jobId              : identification of Salome job
 *  \param params             : Constraints for the choice of the batch cluster
 */
//=============================================================================
void Launcher_cpp::getResultSalomeJob( const string directory,
				       const long id, 
				       const machineParams& params) throw(LauncherException)
{
  vector<string> aCompoList ;
  vector<string> aMachineList = _ResManager->GetFittingResources( params , aCompoList ) ;
  ParserResourcesType p = _ResManager->GetResourcesList(aMachineList[0]);
  string clustername(p.Alias);
    
  // search batch manager for that cluster in map
  map < string, Batch::BatchManager_eClient * >::const_iterator it = _batchmap.find(clustername);
  if(it == _batchmap.end())
    throw LauncherException("no batchmanager for that cluster");
    
  Batch::Job* job = _jobmap[ pair<string,long>(clustername,id) ];

  _batchmap[clustername]->importOutputFiles( *job, directory );
}

//=============================================================================
/*!
 *  Factory to instanciate the good batch manager for choosen cluster.
 */ 
//=============================================================================

Batch::BatchManager_eClient *Launcher_cpp::FactoryBatchManager( const ParserResourcesType& params ) throw(LauncherException)
{

  std::string hostname, protocol, mpi;
  Batch::FactBatchManager_eClient* fact;

  hostname = params.Alias;
  switch(params.Protocol){
  case rsh:
    protocol = "rsh";
    break;
  case ssh:
    protocol = "ssh";
    break;
  default:
    throw LauncherException("unknown protocol");
    break;
  }
  switch(params.mpi){
  case lam:
    mpi = "lam";
    break;
  case mpich1:
    mpi = "mpich1";
    break;
  case mpich2:
    mpi = "mpich2";
    break;
  case openmpi:
    mpi = "openmpi";
    break;
  case slurm:
    mpi = "slurm";
    break;
  default:
    mpi = "indif";
    break;
  }    
  MESSAGE ( "Instanciation of batch manager" );
  switch( params.Batch ){
  case pbs:
    MESSAGE ( "Instantiation of PBS batch manager" );
    fact = new Batch::FactBatchManager_ePBS;
    break;
  case lsf:
    MESSAGE ( "Instantiation of LSF batch manager" );
    fact = new Batch::FactBatchManager_eLSF;
    break;
  default:
    MESSAGE ( "BATCH = " << params.Batch );
    throw LauncherException("no batchmanager for that cluster");
  }
  return (*fact)(hostname.c_str(),protocol.c_str(),mpi.c_str());
}

string Launcher_cpp::buildSalomeCouplingScript(const string fileToExecute, const string dirForTmpFiles, const ParserResourcesType& params)
{
#ifndef WIN32 //TODO: need for porting on Windows
  int idx = dirForTmpFiles.find("Batch/");
  std::string filelogtemp = dirForTmpFiles.substr(idx+6, dirForTmpFiles.length());

  string::size_type p1 = fileToExecute.find_last_of("/");
  string::size_type p2 = fileToExecute.find_last_of(".");
  std::string fileNameToExecute = fileToExecute.substr(p1+1,p2-p1-1);
  std::string TmpFileName = "/tmp/runSalome_" + fileNameToExecute + ".sh";

  MpiImpl* mpiImpl = FactoryMpiImpl(params.mpi);

  ofstream tempOutputFile;
  tempOutputFile.open(TmpFileName.c_str(), ofstream::out );

  // Begin
  tempOutputFile << "#! /bin/sh -f" << endl ;
  tempOutputFile << "cd " ;
  tempOutputFile << params.AppliPath << endl ;
  tempOutputFile << "export SALOME_BATCH=1\n";
  tempOutputFile << "export PYTHONPATH=~/" ;
  tempOutputFile << dirForTmpFiles ;
  tempOutputFile << ":$PYTHONPATH" << endl ;

  // Test node rank
  tempOutputFile << "if test " ;
  tempOutputFile << mpiImpl->rank() ;
  tempOutputFile << " = 0; then" << endl ;

  // -----------------------------------------------
  // Code for rank 0 : launch runAppli and a container
  // RunAppli
  if(params.ModulesList.size()>0)
    tempOutputFile << "  ./runAppli --terminal --modules=" ;
  else
    tempOutputFile << "  ./runAppli --terminal ";
  for ( int i = 0 ; i < params.ModulesList.size() ; i++ ) {
    tempOutputFile << params.ModulesList[i] ;
    if ( i != params.ModulesList.size()-1 )
      tempOutputFile << "," ;
  }
  tempOutputFile << " --standalone=registry,study,moduleCatalog --ns-port-log="
		 << filelogtemp 
		 << " &\n";

  // Wait NamingService
  tempOutputFile << "  current=0\n"
		 << "  stop=20\n" 
		 << "  while ! test -f " << filelogtemp << "\n"
		 << "  do\n"
		 << "    sleep 2\n"
		 << "    let current=current+1\n"
		 << "    if [ \"$current\" -eq \"$stop\" ] ; then\n"
		 << "      echo Error Naming Service failed ! >&2"
		 << "      exit\n"
		 << "    fi\n"
		 << "  done\n"
		 << "  port=`cat " << filelogtemp << "`\n";
    
  // Wait other containers
  tempOutputFile << "  for ((ip=1; ip < ";
  tempOutputFile << mpiImpl->size();
  tempOutputFile << " ; ip++))" << endl;
  tempOutputFile << "  do" << endl ;
  tempOutputFile << "    arglist=\"$arglist YACS_Server_\"$ip" << endl ;
  tempOutputFile << "  done" << endl ;
  tempOutputFile << "  sleep 5" << endl ;
  tempOutputFile << "  ./runSession waitContainers.py $arglist" << endl ;
  
  // Launch user script
  tempOutputFile << "  ./runSession python ~/" << dirForTmpFiles << "/" << fileNameToExecute << ".py" << endl;

  // Stop application
  tempOutputFile << "  rm " << filelogtemp << "\n"
		 << "  ./runSession shutdownSalome.py" << endl;

  // -------------------------------------
  // Other nodes launch a container
  tempOutputFile << "else" << endl ;

  // Wait NamingService
  tempOutputFile << "  current=0\n"
		 << "  stop=20\n" 
		 << "  while ! test -f " << filelogtemp << "\n"
		 << "  do\n"
		 << "    sleep 2\n"
		 << "    let current=current+1\n"
		 << "    if [ \"$current\" -eq \"$stop\" ] ; then\n"
		 << "      echo Error Naming Service failed ! >&2"
		 << "      exit\n"
		 << "    fi\n"
		 << "  done\n"
		 << "  port=`cat " << filelogtemp << "`\n";

  // Launching container
  tempOutputFile << "  ./runSession SALOME_Container YACS_Server_";
  tempOutputFile << mpiImpl->rank()
		 << " > ~/" << dirForTmpFiles << "/YACS_Server_" 
		 << mpiImpl->rank() << "_container_log." << filelogtemp
		 << " 2>&1\n";
  tempOutputFile << "fi" << endl ;
  tempOutputFile.flush();
  tempOutputFile.close();
  chmod(TmpFileName.c_str(), 0x1ED);
  MESSAGE ( TmpFileName.c_str() );

  delete mpiImpl;

  return TmpFileName;
#else
  return "";
#endif
    
}

MpiImpl *Launcher_cpp::FactoryMpiImpl(MpiImplType mpi) throw(LauncherException)
{
  switch(mpi){
  case lam:
    return new MpiImpl_LAM();
  case mpich1:
    return new MpiImpl_MPICH1();
  case mpich2:
    return new MpiImpl_MPICH2();
  case openmpi:
    return new MpiImpl_OPENMPI();
  case slurm:
    return new MpiImpl_SLURM();
  case indif:
    throw LauncherException("you must specify a mpi implementation in CatalogResources.xml file");
  default:
    ostringstream oss;
    oss << mpi << " : not yet implemented";
    throw LauncherException(oss.str().c_str());
  }

}

string Launcher_cpp::getTmpDirForBatchFiles()
{
  string ret;
  string thedate;

  // Adding date to the directory name
  Batch::Date date = Batch::Date(time(0));
  thedate = date.str();
  int lend = thedate.size() ;
  int i = 0 ;
  while ( i < lend ) {
    if ( thedate[i] == '/' || thedate[i] == '-' || thedate[i] == ':' ) {
      thedate[i] = '_' ;
    }
    i++ ;
  }

  ret = string("Batch/");
  ret += thedate;
  return ret;
}

string Launcher_cpp::getRemoteFile( std::string remoteDir, std::string localFile )
{
  string::size_type pos = localFile.find_last_of("/") + 1;
  int ln = localFile.length() - pos;
  string remoteFile = remoteDir + "/" + localFile.substr(pos,ln);
  return remoteFile;
}

bool Launcher_cpp::check(const batchParams& batch_params)
{
  bool rtn = true;
  MESSAGE ( "Job parameters are :" );
  MESSAGE ( "Directory : $HOME/Batch/$date" );

  // check expected_during_time (check the format)
  std::string edt_info = batch_params.expected_during_time;
  std::string edt_value = batch_params.expected_during_time;
  if (edt_value != "") {
    std::string begin_edt_value = edt_value.substr(0, 2);
    std::string mid_edt_value = edt_value.substr(2, 1);
    std::string end_edt_value = edt_value.substr(3);
  
    long value;
    std::istringstream iss(begin_edt_value);
    if (!(iss >> value)) {
      edt_info = "Error on definition ! : " + edt_value;
      rtn = false;
    }
    else if (value < 0) {
      edt_info = "Error on definition time is negative ! : " + value;
      rtn = false;
    }
    std::istringstream iss_2(end_edt_value);
    if (!(iss_2 >> value)) {
      edt_info = "Error on definition ! : " + edt_value;
      rtn = false;
    }
    else if (value < 0) {
      edt_info = "Error on definition time is negative ! : " + value;
      rtn = false;
    }
    if (mid_edt_value != ":") {
      edt_info = "Error on definition ! :" + edt_value;
      rtn = false;
    }
  }
  else {
    edt_info = "No value given";
  }
  MESSAGE ( "Expected during time : " << edt_info );

  // check memory (check the format)
  std::string mem_info;
  std::string mem_value = batch_params.mem;
  if (mem_value != "") {
    std::string begin_mem_value = mem_value.substr(0, mem_value.length()-2);
    long re_mem_value;
    std::istringstream iss(begin_mem_value);
    if (!(iss >> re_mem_value)) {
      mem_info = "Error on definition ! : " + mem_value;
      rtn = false;
    }
    else if (re_mem_value <= 0) {
      mem_info = "Error on definition memory is negative ! : " + mem_value;
      rtn = false;
    }
    std::string end_mem_value = mem_value.substr(mem_value.length()-2);
    if (end_mem_value != "gb" && end_mem_value != "mb") {
      mem_info = "Error on definition, type is bad ! " + mem_value;
      rtn = false;
    }
  }
  else {
    mem_info = "No value given";
  }
  MESSAGE ( "Memory : " << mem_info );

  // check nb_proc
  std::string nb_proc_info;
  ostringstream nb_proc_value;
  nb_proc_value << batch_params.nb_proc;
  if(batch_params.nb_proc <= 0) {
    nb_proc_info = "Bad value ! nb_proc = ";
    nb_proc_info += nb_proc_value.str();
    rtn = false;
  }
  else {
    nb_proc_info = nb_proc_value.str();
  }
  MESSAGE ( "Nb of processors : " << nb_proc_info );

  return rtn;
}

long Launcher_cpp::getWallTime(std::string edt)
{
  long hh, mm, ret;

  if( edt.size() == 0 )
    return 0;

  string::size_type pos = edt.find(":");
  string h = edt.substr(0,pos);
  string m = edt.substr(pos+1,edt.size()-pos+1);
  istringstream issh(h);
  issh >> hh;
  istringstream issm(m);
  issm >> mm;
  ret = hh*60 + mm;
  return  ret;
}

long Launcher_cpp::getRamSize(std::string mem)
{
  long mv;

  if( mem.size() == 0 )
    return 0;

  string ram = mem.substr(0,mem.size()-2);
  istringstream iss(ram);
  iss >> mv;
  string unity = mem.substr(mem.size()-2,2);
  if( (unity.find("gb") != string::npos) || (unity.find("GB") != string::npos) )
    return mv*1024;
  else if( (unity.find("mb") != string::npos) || (unity.find("MB") != string::npos) )
    return mv;
  else if( (unity.find("kb") != string::npos) || (unity.find("KB") != string::npos) )
    return mv/1024;
  else if( (unity.find("b") != string::npos) || (unity.find("B") != string::npos) )
    return mv/(1024*1024);
  else
    return 0;
}

std::string
Launcher_cpp::getHomeDir(const ParserResourcesType& p, const std::string& tmpdir)
{
    std::string home;
    std::string command;
    int idx = tmpdir.find("Batch/");
    std::string filelogtemp = tmpdir.substr(idx+6, tmpdir.length());
    filelogtemp = "/tmp/logs" + filelogtemp + "_home";

    if( p.Protocol == rsh )
      command = "rsh ";
    else if( p.Protocol == ssh )
      command = "ssh ";
    else
      throw LauncherException("Unknown protocol");
    if (p.UserName != ""){
      command += p.UserName;
      command += "@";
    }
    command += p.Alias;
    command += " 'echo $HOME' > ";
    command += filelogtemp;
    MESSAGE ( command.c_str() );
    int status = system(command.c_str());
    if(status)
      throw LauncherException("Error of launching home command on remote host");

    std::ifstream file_home(filelogtemp.c_str());
    std::getline(file_home, home);
    file_home.close();
    return home;
}
