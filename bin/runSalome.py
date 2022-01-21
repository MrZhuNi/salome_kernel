#!/usr/bin/env python3
#  -*- coding: iso-8859-1 -*-
# Copyright (C) 2007-2021  CEA/DEN, EDF R&D, OPEN CASCADE
#
# Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
# CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

## @package runSalome
# \brief Module that provides services to launch SALOME
#

import sys, os, string, glob, time, pickle, re
import orbmodule
import setenv
from launchConfigureParser import verbose
from server import process_id, Server
import json
import subprocess
from salomeContextUtils import ScriptAndArgsObjectEncoder
import platform

# -----------------------------------------------------------------------------

from killSalome import killAllPorts

def killLocalPort():
    """
    kill servers from a previous SALOME execution, if needed,
    on the CORBA port given in args of runSalome
    """

    from killSalomeWithPort import killMyPort
    my_port=str(args['port'])
    try:
        killMyPort(my_port)
    except Exception:
        print("problem in killLocalPort()")
        pass
    pass

def givenPortKill(port):
    """
    kill servers from a previous SALOME execution, if needed,
    on the same CORBA port
    """

    from killSalomeWithPort import killMyPort
    my_port=port
    try:
        killMyPort(my_port)
    except Exception:
        print("problem in LocalPortKill(), killMyPort(%s)"%port)
        pass
    pass

def kill_salome(args):
    """
    Kill servers from previous SALOME executions, if needed;
    depending on args 'killall' or 'portkill', kill all executions,
    or only execution on the same CORBA port
    """

    if args['killall']:
        killAllPorts()
    elif args['portkill']:
        givenPortKill(str(args['port']))

# -----------------------------------------------------------------------------
#
# Class definitions to launch CORBA Servers
#

class InterpServer(Server):
    def __init__(self,args):
        self.args=args
        if sys.platform == "win32":
          self.CMD=['cmd', '/c', 'start cmd.exe', '/K', 'python']
        elif sys.platform == "darwin":
          env_ld_library_path=['env', 'DYLD_LIBRARY_PATH=' + os.getenv("LD_LIBRARY_PATH")]
          self.CMD=['xterm', '-e'] + env_ld_library_path + ['python']
        else:
          env_ld_library_path=['env', 'LD_LIBRARY_PATH=' + os.getenv("LD_LIBRARY_PATH")]
          self.CMD=['xterm', '-e'] + env_ld_library_path + ['python']

    def run(self):
        global process_id
        command = self.CMD
        print("INTERPSERVER::command = ", command)
        import subprocess
        pid = subprocess.Popen(command).pid
        process_id[pid]=self.CMD
        self.PID = pid

# ---

def get_cata_path(list_modules,modules_root_dir):
    """Build a list of catalog paths (cata_path) to initialize the ModuleCatalog server
    """
    modules_cata={}
    cata_path=[]

    for module in list_modules:
        if module in modules_root_dir:
            module_root_dir=modules_root_dir[module]
            module_cata=module+"Catalog.xml"
            cata_file=os.path.join(module_root_dir, "share",setenv.salome_subdir, "resources",module.lower(), module_cata)

            if os.path.exists(cata_file):
                cata_path.append(cata_file)
                modules_cata[module]=cata_file
            else:
                cata_file=os.path.join(module_root_dir, "share",setenv.salome_subdir, "resources", module_cata)
                if os.path.exists(cata_file):
                    cata_path.append(cata_file)
                    modules_cata[module]=cata_file

    for path in os.getenv("SALOME_CATALOGS_PATH","").split(os.pathsep):
        if os.path.exists(path):
            for cata_file in glob.glob(os.path.join(path,"*Catalog.xml")):
                module_name= os.path.basename(cata_file)[:-11]
                if module_name not in modules_cata:
                    cata_path.append(cata_file)
                    modules_cata[module_name]=cata_file

    return cata_path

class CatalogServer(Server):
    def __init__(self,args):
        self.args=args
        self.initArgs()
        self.SCMD1=['SALOME_ModuleCatalog_Server']
        if 'launcher' in self.args:
            pos = args['launcher'].find(":")
            if pos != -1:
              self.SCMD1+=['-ORBInitRef']
              machine = args['launcher'][0:pos]
              port = args['launcher'][pos+1:]
              self.SCMD1+=["NameService=corbaname::" + machine + ":" + port]
        self.SCMD1+=['-common']
        self.SCMD2=[]
        home_dir=os.path.expanduser("~")
        if home_dir is not None:
            self.SCMD2=['-personal',os.path.join(home_dir,'Salome', 'resources', 'CatalogModulePersonnel.xml')]

    def setpath(self,modules_list,modules_root_dir):
        list_modules = modules_list[:]
        list_modules.reverse()
        if self.args["gui"] :
            list_modules = ["KERNEL", "GUI"] + list_modules
        else :
            list_modules = ["KERNEL"] + list_modules

        cata_path=get_cata_path(list_modules,modules_root_dir)

        self.CMD=self.SCMD1 + ['"' + '"::"'.join(cata_path) + '"'] + self.SCMD2

# ---

class SalomeDSServer(Server):
    def __init__(self,args):
        self.args=args
        self.initArgs()
        self.CMD=['SALOMEDS_Server']
        if 'launcher' in self.args:
            pos = args['launcher'].find(":")
            if pos != -1:
              self.CMD+=['-ORBInitRef']
              machine = args['launcher'][0:pos]
              port = args['launcher'][pos+1:]
              self.CMD+=["NameService=corbaname::" + machine + ":" + port]

# ---

class ConnectionManagerServer(Server):
    def __init__(self,args):
        self.args=args
        self.initArgs()
        self.CMD=['SALOME_ConnectionManagerServer']
        if 'launcher' in self.args:
            pos = args['launcher'].find(":")
            if pos != -1:
              self.CMD+=['-ORBInitRef']
              machine = args['launcher'][0:pos]
              port = args['launcher'][pos+1:]
              self.CMD+=["NameService=corbaname::" + machine + ":" + port]


# ---

class RegistryServer(Server):
    def __init__(self,args):
        self.args=args
        self.initArgs()
        self.CMD=['SALOME_Registry_Server', '--salome_session','theSession']
        if 'launcher' in self.args:
            pos = args['launcher'].find(":")
            if pos != -1:
              self.CMD+=['-ORBInitRef']
              machine = args['launcher'][0:pos]
              port = args['launcher'][pos+1:]
              self.CMD+=["NameService=corbaname::" + machine + ":" + port]

# ---

class ContainerCPPServer(Server):
    def __init__(self,args,with_gui=False):
        self.args=args
        self.initArgs()
        self.CMD=['SALOME_Container']
        if 'launcher' in self.args:
            pos = args['launcher'].find(":")
            if pos != -1:
              self.CMD+=['-ORBInitRef']
              machine = args['launcher'][0:pos]
              port = args['launcher'][pos+1:]
              self.CMD+=["NameService=corbaname::" + machine + ":" + port]
        self.CMD+=['FactoryServer']
        if not with_gui and self.args["valgrind_session"]:
            l = ["valgrind"]
            val = os.getenv("VALGRIND_OPTIONS")
            if val:
                l += val.split()
                pass
            self.CMD = l + self.CMD
            pass

# ---

class LoggerServer(Server):
    def __init__(self,args):
        self.args=args
        self.initArgs()
        from salome_utils import generateFileName, getLogDir
        logfile = generateFileName( getLogDir(),
                                    prefix="logger",
                                    extension="log",
                                    with_username=True,
                                    with_hostname=True,
                                    with_port=True)
        print("===========================================================")
        print("Logger server: put log to the file:")
        print(logfile)
        print("===========================================================")
        self.CMD=['SALOME_Logger_Server', logfile]
        pass
    pass # end of LoggerServer class

# ---

class SessionServer(Server):
    def __init__(self,args,modules_list,modules_root_dir):
        self.args = args.copy()
        # Bug 11512 (Problems with runSalome --xterm on Mandrake and Debian Sarge)
        #self.args['xterm']=0
        #
        self.initArgs()
        self.SCMD1=['SALOME_Session_Server']
        if "SQUISH_PREFIX" in os.environ:
            if platform.system() == "Windows" :
                self.SCMD1 = [os.path.join(os.getenv("SQUISH_PREFIX"), "bin", "dllpreload.exe"),os.path.join(os.getenv("SQUISH_SALOME_PATH"), "W64", "GUI", "bin", "salome", self.SCMD1[0])]
            else :
                os.environ["LD_LIBRARY_PATH"] = os.environ["SQUISH_PREFIX"] + "/lib:" + os.environ["LD_LIBRARY_PATH"]
        self.SCMD2=[]
        if 'launcher' in self.args:
            pos = args['launcher'].find(":")
            if pos != -1:
              self.SCMD1+=['-ORBInitRef']
              machine = args['launcher'][0:pos]
              port = args['launcher'][pos+1:]
              self.SCMD1+=["NameService=corbaname::" + machine + ":" + port]
        if 'registry' in self.args['embedded']:
            self.SCMD1+=['--with','Registry',
                         '(','--salome_session','theSession',')']
        if 'moduleCatalog' in self.args['embedded']:
            self.SCMD1+=['--with','ModuleCatalog','(','-common']
            home_dir=os.path.expanduser("~")
            if home_dir is not None:
                self.SCMD2+=['-personal',os.path.join(home_dir,'Salome','resources','CatalogModulePersonnel.xml')]
            self.SCMD2+=[')']
        if 'study' in self.args['embedded']:
            self.SCMD2+=['--with','SALOMEDS','(',')']
        if 'cppContainer' in self.args['embedded']:
            self.SCMD2+=['--with','Container','(','FactoryServer',')']
        if 'SalomeAppEngine' in self.args['embedded']:
            self.SCMD2+=['--with','SalomeAppEngine','(',')']

        if 'cppContainer' in self.args['standalone'] or 'cppContainer' in self.args['embedded']:
            self.SCMD2+=['CPP']
        if 'pyContainer' in self.args['standalone'] or 'pyContainer' in self.args['embedded']:
            raise Exception('Python containers no longer supported')
        if self.args['gui']:
            session_gui = self.args.get('session_gui', True)
            if not session_gui:
                self.SCMD2+=['--hide-desktop']
            else:
                if not self.args['splash']:
                    self.SCMD2+=['--hide-splash']
                    pass
                if self.args['study_hdf'] is not None:
                    self.SCMD2+=['--study-hdf=%s'%self.args['study_hdf']]
                    pass
                pass
                if 'pyscript' in self.args and len(self.args['pyscript']) > 0:
                    msg = json.dumps(self.args['pyscript'], cls=ScriptAndArgsObjectEncoder)
                    self.SCMD2+=['--pyscript=%s'%(msg)]
                    pass
                pass
            pass
        if self.args['noexcepthandler']:
            self.SCMD2+=['--no-exception-handler']
        if 'user_config' in self.args:
            self.SCMD2+=['--resources=%s'%self.args['user_config']]
        if 'modules' in self.args:
            list_modules = []
            #keep only modules with GUI
            for m in modules_list:
              if m not in modules_root_dir:
                list_modules.insert(0,m)
              else:
                fr1 = os.path.join(modules_root_dir[m],"share","salome","resources",m.lower(),"SalomeApp.xml")
                fr2 = os.path.join(modules_root_dir[m],"share","salome","resources","SalomeApp.xml")
                if os.path.exists(fr1) or os.path.exists(fr2):
                  list_modules.insert(0,m)
            list_modules.reverse()
            self.SCMD2+=['--modules (%s)' % ":".join(list_modules)]
            pass
        if 'language' in self.args:
            self.SCMD2+=['--language=%s' % self.args['language']]
        pass

    def setpath(self,modules_list,modules_root_dir):
        list_modules = modules_list[:]
        list_modules.reverse()
        if self.args["gui"] :
            list_modules = ["KERNEL", "GUI"] + list_modules
        else :
            list_modules = ["KERNEL"] + list_modules

        cata_path=get_cata_path(list_modules,modules_root_dir)

        if ("gui" in self.args) & ('moduleCatalog' in self.args['embedded']):
            #Use '::' instead ":" because drive path with "D:\" is invalid on windows platform
            self.CMD=self.SCMD1 + ['"' + '"::"'.join(cata_path) + '"'] + self.SCMD2
        else:
            self.CMD=self.SCMD1 + self.SCMD2
        if 'test' in self.args:
            self.CMD+=['-test'] + self.args['test']
        elif 'play' in self.args:
            self.CMD+=['-play'] + self.args['play']

        if self.args["gdb_session"] or self.args["ddd_session"]:
            f = open(".gdbinit4salome", "w")
            f.write("set args ")
            args = " ".join(self.CMD[1:])
            args = args.replace("(", "\(")
            args = args.replace(")", "\)")
            f.write(args)
            f.write("\n")
            f.close()
            if self.args["ddd_session"]:
                self.CMD = ["ddd", "--command=.gdbinit4salome", self.CMD[0]]
            elif self.args["gdb_session"]:
                self.CMD = ["xterm", "-e", "gdb", "--command=.gdbinit4salome", self.CMD[0]]
                pass
            pass

        if self.args["valgrind_session"]:
            l = ["valgrind"]
            val = os.getenv("VALGRIND_OPTIONS")
            if val:
                l += val.split()
                pass
            self.CMD = l + self.CMD
            pass

# ---

class LauncherServer(Server):
    def __init__(self,args):
        self.args=args
        self.initArgs()
        self.SCMD1=['SALOME_LauncherServer']
        self.SCMD2=[]
        if args["gui"] :
            if 'registry' in self.args['embedded']:
                self.SCMD1+=['--with','Registry',
                             '(','--salome_session','theSession',')']
            if 'moduleCatalog' in self.args['embedded']:
                self.SCMD1+=['--with','ModuleCatalog','(','-common']
                home_dir=os.path.expanduser("~")
                if home_dir is not None:
                    self.SCMD2=['-personal',os.path.join(home_dir,'Salome','resources','CatalogModulePersonnel.xml')]
                self.SCMD2+=[')']
            if 'study' in self.args['embedded']:
                self.SCMD2+=['--with','SALOMEDS','(',')']
            if 'cppContainer' in self.args['embedded']:
                self.SCMD2+=['--with','Container','(','FactoryServer',')']

    def setpath(self,modules_list,modules_root_dir):
        list_modules = modules_list[:]
        list_modules.reverse()
        if self.args["gui"] :
            list_modules = ["KERNEL", "GUI"] + list_modules
        else :
            list_modules = ["KERNEL"] + list_modules

        cata_path=get_cata_path(list_modules,modules_root_dir)

        if ("gui" in self.args) & ('moduleCatalog' in self.args['embedded']):
            #Use '::' instead ":" because drive path with "D:\" is invalid on windows platform
            self.CMD=self.SCMD1 + ['"' + '"::"'.join(cata_path) + '"'] + self.SCMD2
        else:
            self.CMD=self.SCMD1 + self.SCMD2
#
# -----------------------------------------------------------------------------

def startGUI(clt):
    """Salome Session Graphic User Interface activation"""
    import Engines
    import SALOME
    import SALOMEDS
    import SALOME_ModuleCatalog
    import SALOME_Session_idl
    session=clt.waitNS("/Kernel/Session",SALOME.Session)
    session.GetInterface()

# -----------------------------------------------------------------------------

def startSalome(args, modules_list, modules_root_dir):
    """Launch all SALOME servers requested by args"""
    init_time = os.times()

    if verbose(): print("startSalome ", args)

    #
    # Set server launch command
    #
    if 'server_launch_mode' in args:
        Server.set_server_launch_mode(args['server_launch_mode'])

    #
    # Wake up session option
    #
    if args['wake_up_session']:
        if "OMNIORB_CONFIG" not in os.environ:
            from salome_utils import generateFileName
            omniorbUserPath = os.getenv("OMNIORB_USER_PATH")
            kwargs={}
            if omniorbUserPath is not None:
                kwargs["with_username"]=True

            last_running_config = generateFileName(omniorbUserPath, prefix="omniORB",
                                                   suffix="last",
                                                   extension="cfg",
                                                   hidden=True,
                                                   **kwargs)
            os.environ['OMNIORB_CONFIG'] = last_running_config
            pass
        pass

    #
    # Initialisation ORB and Naming Service
    #

    clt=orbmodule.client(args)
    addToPidict(args)

    #
    # Wake up session option
    #
    if args['wake_up_session']:
        import Engines
        import SALOME
        import SALOMEDS
        import SALOME_ModuleCatalog
        import SALOME_Session_idl
        session = clt.waitNS("/Kernel/Session",SALOME.Session)
        status = session.GetStatSession()
        if status.activeGUI:
            from salome_utils import getPortNumber
            port = getPortNumber()
            msg  = "Warning :"
            msg += "\n"
            msg += "Session GUI for port number %s is already active."%(port)
            msg += "\n"
            msg += "If you which to wake up another session,"
            msg += "\n"
            msg += "please use variable OMNIORB_CONFIG"
            msg += "\n"
            msg += "to get the correct session object in naming service."
            sys.stdout.write(msg+"\n")
            sys.stdout.flush()
            return clt
        session.GetInterface()
        args["session_object"] = session
        return clt

    # Launch Logger Server (optional)
    # and wait until it is registered in naming service
    #

    if args['logger']:
        myServer=LoggerServer(args)
        myServer.run()
        clt.waitLogger("Logger")
        addToPidict(args)

    # Launch  Session Server (to show splash ASAP)
    #

    if args["gui"] and not args['launcher_only']:
        mySessionServ = SessionServer(args,args['modules'],modules_root_dir)
        mySessionServ.setpath(modules_list,modules_root_dir)
        mySessionServ.run()
        addToPidict(args)

    #
    # Launch Registry Server,
    # and wait until it is registered in naming service
    #

    if ('registry' not in args['embedded']) | (args["gui"] == 0) :
        myServer=RegistryServer(args)
        myServer.run()
        if sys.platform == "win32":
          clt.waitNS("/Registry")
        else:
          clt.waitNSPID("/Registry",myServer.PID)
        addToPidict(args)

    #
    # Launch Catalog Server,
    # and wait until it is registered in naming service
    #

    if ('moduleCatalog' not in args['embedded']) | (args["gui"] == 0):
        cataServer=CatalogServer(args)
        cataServer.setpath(modules_list,modules_root_dir)
        cataServer.run()
        import SALOME_ModuleCatalog
        if sys.platform == "win32":
          clt.waitNS("/Kernel/ModulCatalog",SALOME_ModuleCatalog.ModuleCatalog)
        else:
          clt.waitNSPID("/Kernel/ModulCatalog",cataServer.PID,SALOME_ModuleCatalog.ModuleCatalog)
        addToPidict(args)

    #
    # Launch SalomeDS Server,
    # and wait until it is registered in naming service
    #

    # print("ARGS = ",args)
    if ('study' not in args['embedded']) | (args["gui"] == 0):
        print("RunStudy")
        myServer=SalomeDSServer(args)
        myServer.run()
        if sys.platform == "win32":
          clt.waitNS("/Study")
        else:
          clt.waitNSPID("/Study",myServer.PID)
        addToPidict(args)

    #
    # Launch LauncherServer
    #

    if not 'launcher' in args:
      myCmServer = LauncherServer(args)
      myCmServer.setpath(modules_list,modules_root_dir)
      myCmServer.run()
      addToPidict(args)

    #
    # Launch ConnectionManagerServer
    #

    if not args['launcher_only']:
      myConnectionServer = ConnectionManagerServer(args)
      myConnectionServer.run()

    from Utils_Identity import getShortHostName

    if os.getenv("HOSTNAME") == None:
        if os.getenv("HOST") == None:
            os.environ["HOSTNAME"]=getShortHostName()
        else:
            os.environ["HOSTNAME"]=os.getenv("HOST")

    theComputer = getShortHostName()

    #
    # Launch local C++ Container (FactoryServer),
    # and wait until it is registered in naming service
    #

    if ('cppContainer' in args['standalone']) | (args["gui"] == 0) :
        myServer=ContainerCPPServer(args, with_gui=args["gui"]!=0)
        myServer.run()
        if sys.platform == "win32":
          clt.waitNS("/Containers/" + theComputer + "/FactoryServer")
        else:
          clt.waitNSPID("/Containers/" + theComputer + "/FactoryServer",myServer.PID)
        addToPidict(args)

    if 'pyContainer' in args['standalone']:
        raise Exception('Python containers no longer supported')

    #
    # Wait until Session Server is registered in naming service
    #

    if args["gui"] and not args['launcher_only']:
##----------------
        import Engines
        import SALOME
        import SALOMEDS
        import SALOME_ModuleCatalog
        import SALOME_Session_idl
        if sys.platform == "win32":
          session=clt.waitNS("/Kernel/Session",SALOME.Session)
        else:
          session=clt.waitNSPID("/Kernel/Session",mySessionServ.PID,SALOME.Session)
        args["session_object"] = session
    end_time = os.times()
    if verbose(): print()
    print("Start SALOME, elapsed time : %5.1f seconds"% (end_time[4]
                                                         - init_time[4]))

    # ASV start GUI without Loader
    #if args['gui']:
    #    session.GetInterface()

    #
    # additional external python interpreters
    #
    nbaddi=0

    try:
        if 'interp' in args:
            nbaddi = args['interp']
    except Exception:
        import traceback
        traceback.print_exc()
        print("-------------------------------------------------------------")
        print("-- to get an external python interpreter:runSalome --interp=1")
        print("-------------------------------------------------------------")

    if verbose(): print("additional external python interpreters: ", nbaddi)
    if nbaddi:
        for i in range(nbaddi):
            print("i=",i)
            anInterp=InterpServer(args)
            anInterp.run()

    # set PYTHONINSPECT variable (python interpreter in interactive mode)
    if args['pinter']:
        os.environ["PYTHONINSPECT"]="1"
        try:
            import readline
        except ImportError:
            pass

    return clt

# -----------------------------------------------------------------------------

def useSalome(args, modules_list, modules_root_dir):
    """
    Launch all SALOME servers requested by args,
    save list of process, give info to user,
    show registered objects in Naming Service.
    """
    global process_id

    clt=None
    try:
        clt = startSalome(args, modules_list, modules_root_dir)
    except Exception:
        import traceback
        traceback.print_exc()
        print()
        print()
        print("--- Error during Salome launch ---")

    # print(process_id)

    from addToKillList import addToKillList
    from killSalomeWithPort import getPiDict

    filedict = getPiDict(args['port'])
    for pid, cmd in list(process_id.items()):
        addToKillList(pid, cmd, args['port'])
        pass

    if verbose(): print("""
    Saving of the dictionary of Salome processes in %s
    To kill SALOME processes from a console (kill all sessions from all ports):
      python killSalome.py
    To kill SALOME from the present interpreter, if it is not closed :
      killLocalPort()      --> kill this session
                               (use CORBA port from args of runSalome)
      givenPortKill(port)  --> kill a specific session with given CORBA port
      killAllPorts()       --> kill all sessions

    runSalome, with --killall option, starts with killing
    the processes resulting from the previous execution.
    """%filedict)

    #
    #  Print Naming Service directory list
    #

    if clt != None:
        if verbose():
            print()
            print(" --- registered objects tree in Naming Service ---")
            clt.showNS()
            pass

        if not args['gui'] or not args['session_gui']:
            if args['shutdown_servers']:
                class __utils__:
                    def __init__(self, port):
                        self.port = port
                        import killSalomeWithPort
                        self.killSalomeWithPort = killSalomeWithPort
                        return
                    def __del__(self):
                        self.killSalomeWithPort.killMyPort(self.port)
                        return
                    pass
                def func(s):
                    del s
                import atexit
                atexit.register(func, __utils__(args['port']))
                pass
            pass

        # run python scripts, passed as command line arguments
        toimport = []
        if 'gui' in args and 'session_gui' in args:
            if not args['gui'] or not args['session_gui']:
                if 'study_hdf' in args:
                    toopen = args['study_hdf']
                    if toopen:
                        import salome
                        salome.salome_init(path=toopen)
                if 'pyscript' in args:
                    toimport = args['pyscript']
        from salomeContextUtils import formatScriptsAndArgs
        command = formatScriptsAndArgs(toimport, escapeSpaces=True)
        if command:
            proc = subprocess.Popen(command, shell=True)
            addToKillList(proc.pid, command, args['port'])
            res = proc.wait()
            if res: sys.exit(1) # if there's an error when executing script, we should explicitly exit

    return clt

def execScript(script_path):
    print('executing', script_path)
    sys.path.insert(0, os.path.realpath(os.path.dirname(script_path)))
    exec(compile(open(script_path).read(), script_path, 'exec'),globals())
    del sys.path[0]

# -----------------------------------------------------------------------------

def registerEnv(args, modules_list, modules_root_dir):
    """
    Register args, modules_list, modules_root_dir in a file
    for further use, when SALOME is launched embedded in an other application.
    """
    from salome_utils import getTmpDir
    fileEnv = getTmpDir()
    from salome_utils import getUserName
    fileEnv += getUserName() + "_" + str(args['port']) \
            + '_' + args['appname'].upper() + '_env'
    fenv=open(fileEnv,'w')
    pickle.dump((args, modules_list, modules_root_dir),fenv)
    fenv.close()
    os.environ["SALOME_LAUNCH_CONFIG"] = fileEnv

# -----------------------------------------------------------------------------

def no_main():
    """Salome Launch, when embedded in other application"""
    fileEnv = os.environ["SALOME_LAUNCH_CONFIG"]
    fenv=open(fileEnv,'r')
    args, modules_list, modules_root_dir = pickle.load(fenv)
    fenv.close()
    kill_salome(args)
    from searchFreePort import searchFreePort
    searchFreePort(args, 0)
    clt = useSalome(args, modules_list, modules_root_dir)
    return clt

# -----------------------------------------------------------------------------

def addToPidict(args):
    global process_id
    from addToKillList import addToKillList
    for pid, cmd in list(process_id.items()):
        addToKillList(pid, cmd, args['port'])

# -----------------------------------------------------------------------------

def main(exeName=None):
    """Salome launch as a main application"""

    # define folder to store omniorb config (initially in virtual application folder)
    try:
        from salomeContextUtils import setOmniOrbUserPath
        setOmniOrbUserPath()
    except Exception as e:
        print(e)
        sys.exit(1)

    from salome_utils import getHostName
    keep_env = not os.getenv('SALOME_PLEASE_SETUP_ENVIRONMENT_AS_BEFORE')
    args, modules_list, modules_root_dir = setenv.get_config(exeName=exeName, keepEnvironment=keep_env)
    print("runSalome running on %s" % getHostName())

    kill_salome(args)
    save_config = True
    if 'save_config' in args:
        save_config = args['save_config']
    # --
    test = True
    if args['wake_up_session']:
        test = False
        pass
    if test and not 'launcher' in args:
        from searchFreePort import searchFreePort
        searchFreePort(args, save_config, args.get('useport'))
        pass
    # --
    #setenv.main()
    setenv.set_env(args, modules_list, modules_root_dir, keepEnvironment=keep_env)
    clt = useSalome(args, modules_list, modules_root_dir)
    return clt,args

# -----------------------------------------------------------------------------

def foreGround(clt, args):
    # --
    if "session_object" not in args:
        return
    session = args["session_object"]
    # --
    # Wait until gui is arrived
    # tmax = nbtot * dt
    # --
    gui_detected = False
    dt = 0.1
    nbtot = 100
    nb = 0
    session_pid = None
    while 1:
        try:
            status = session.GetStatSession()
            gui_detected = status.activeGUI
            session_pid = session.getPID()
        except Exception:
            pass
        if gui_detected:
            break
        from time import sleep
        sleep(dt)
        nb += 1
        if nb == nbtot:
            break
        pass
    # --
    if not gui_detected:
        return
    # --
    from salome_utils import getPortNumber
    port = getPortNumber()
    # --
    server = Server({})
    if sys.platform == "win32":
      server.CMD = [os.getenv("PYTHONBIN"), "-m", "killSalomeWithPort", "--spy", "%s"%(session_pid or os.getpid()), "%s"%(port)]
    else:
      server.CMD = ["killSalomeWithPort.py", "--spy", "%s"%(session_pid or os.getpid()), "%s"%(port)]
    server.run(True)
    # os.system("killSalomeWithPort.py --spy %s %s &"%(os.getpid(), port))
    # --
    dt = 1.0
    try:
        while 1:
            try:
                status = session.GetStatSession()
                assert status.activeGUI
            except Exception:
                break
            from time import sleep
            sleep(dt)
            pass
        pass
    except KeyboardInterrupt:
        from killSalomeWithPort import killMyPort
        killMyPort(port)
        pass
    return
#

def runSalome():
    clt,args = main()
    # --
    test = args['gui'] and args['session_gui']
    test = test or args['wake_up_session']
    # --
    # The next test covers the --pinter option or if var PYTHONINSPECT is set
    # --
    test = test and not os.environ.get('PYTHONINSPECT')
    # --
    # The next test covers the python -i $KERNEL_ROOT_DIR/bin/salome/runSalome.py case
    # --
    try:
        from ctypes import POINTER, c_int, cast, pythonapi
        iflag_ptr = cast(pythonapi.Py_InteractiveFlag, POINTER(c_int))
        test = test and not iflag_ptr.contents.value
    except Exception:
        pass
    # --
#    test = test and os.getenv("SALOME_TEST_MODE", "0") != "1"
    test = test and args['foreground']
    # --
    if test:
        from time import sleep
        sleep(3.0)
        foreGround(clt, args)
        pass
    pass
#

# -----------------------------------------------------------------------------

if __name__ == "__main__":
    runSalome()
#
