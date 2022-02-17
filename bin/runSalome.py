#!/usr/bin/env python3
#  -*- coding: iso-8859-1 -*-
# Copyright (C) 2022 CEA/DEN, EDF R&D
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
from server import process_id, Server
import json
import subprocess
from salomeContextUtils import ScriptAndArgsObjectEncoder
import runSalomeNoServer
import platform
import logging
logger = logging.getLogger()

class ColoredFormatter(logging.Formatter):
    BLACK, RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE = range(30,38)
    COLORS = { 'WARNING': YELLOW, 'INFO': WHITE, 'DEBUG': BLUE, 'CRITICAL': YELLOW, 'ERROR': RED }
    def __init__(self, *args, **kwargs):
        logging.Formatter.__init__(self, *args, **kwargs)
    def format(self, record):
        RESET_SEQ = "\033[0m"
        COLOR_SEQ = "\033[1;%dm"
        record.levelname = COLOR_SEQ % ColoredFormatter.COLORS[record.levelname] + record.levelname + RESET_SEQ
        return logging.Formatter.format(self, record)


def setVerbose(verbose):
    global logger
    logger = logging.getLogger()
    formatter = logging.Formatter('%(levelname)s : %(asctime)s : %(message)s ',style='%')
    formatter.default_time_format = '%H:%M:%S'
    formatter.default_msec_format = "%s.%03d"
    stream_handler = logging.StreamHandler()
    stream_handler.setFormatter(formatter)
    logger.addHandler(stream_handler)

    verbose_map = { "0": logging.WARNING, "1": logging.INFO, "2": logging.DEBUG}
    if verbose in verbose_map:
        logger.setLevel(verbose_map[verbose])

# -----------------------------------------------------------------------------

from killSalome import killAllPorts

def kill_salome(args):
    """
    Kill servers from previous SALOME executions, if needed;
    depending on args 'killall' or 'portkill', kill all executions,
    or only execution on the same CORBA port
    """

    if args['killall']:
        killAllPorts()
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

    logger.debug("startSalome : {} ".format(args))

    # Launch  Session Server (to show splash ASAP)
    #

    if args["gui"] and not args['launcher_only']:
        mySessionServ = runSalomeNoServer.NoSessionServer(args,args['modules'],modules_root_dir)
        mySessionServ.setpath(modules_list,modules_root_dir)
        mySessionServ.run()
    
    end_time = os.times()

    #
    # Wait until Session Server is registered in naming service
    #
    logger.debug("Start SALOME, elapsed time : %5.1f seconds"% (end_time[4] - init_time[4]))

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

    logger.debug("additional external python interpreters: {}".format(nbaddi))
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

    return

# -----------------------------------------------------------------------------

def useSalome(args, modules_list, modules_root_dir):
    """
    Launch all SALOME servers requested by args,
    save list of process, give info to user,
    show registered objects in Naming Service.
    """
    global process_id

    try:
        startSalome(args, modules_list, modules_root_dir)
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

    logger.debug("""
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

    return

def execScript(script_path):
    print('executing', script_path)
    sys.path.insert(0, os.path.realpath(os.path.dirname(script_path)))
    exec(compile(open(script_path).read(), script_path, 'exec'),globals())
    del sys.path[0]

# -----------------------------------------------------------------------------

def addToPidict(args):
    global process_id
    from addToKillList import addToKillList
    for pid, cmd in list(process_id.items()):
        addToKillList(pid, cmd, args['port'])

# -----------------------------------------------------------------------------

def main(exeName=None):
    """Salome launch as a main application"""
    keep_env = not os.getenv('SALOME_PLEASE_SETUP_ENVIRONMENT_AS_BEFORE')
    args, modules_list, modules_root_dir = setenv.get_config(exeName=exeName, keepEnvironment=keep_env)
    setVerbose(args["verbosity"])
    kill_salome(args)
    # --
    setenv.set_env(args, modules_list, modules_root_dir, keepEnvironment=keep_env)
    useSalome(args, modules_list, modules_root_dir)
    return args

# -----------------------------------------------------------------------------

def foreGround(args):
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
    args = main()
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
        foreGround(args)
        pass
    pass
#

# -----------------------------------------------------------------------------

if __name__ == "__main__":
    runSalome()
#
