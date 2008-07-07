#!/usr/bin/env python

# Copyright (C) 2005  OPEN CASCADE, CEA, EDF R&D, LEG
#           PRINCIPIA R&D, EADS CCR, Lip6, BV, CEDRAT
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either 
# version 2.1 of the License.
# 
# This library is distributed in the hope that it will be useful 
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

import os, sys, pickle, string, signal
from launchConfigureParser import verbose

########## adds to the kill list of SALOME one more process ##########

def findFileDict():
    """
    Detect current SALOME session's port number.
    Returns port number.
    """
    from salome_utilities import getPortNumber
    port = getPortNumber()
    if verbose(): print "myport = ", port
    return port
    
def addToKillList(command_pid, command, port=None):
    """
    Add the process to the SALOME processes dictionary file.
    Parameters:
    - command_pid : command PID
    - command     : command (string or list of strings)
    - [port]      : SALOME port number; if this parameter is None (default),
    it is detected automatically
    """
    # retrieve current processes dictionary
    from killSalomeWithPort import getPiDict
    if port is None: port=findFileDict()
    filedict=getPiDict(port)
    try:
        fpid=open(filedict, 'r')
        process_ids=pickle.load(fpid)
        fpid.close()
    except:
        process_ids=[]
        pass
    # check if PID is already in dictionary
    already_in=False
    for process_id in process_ids:
        for pid, cmd in process_id.items():
            if int(pid) == int(command_pid):
                already_in=True
                break
            pass
        if already_in: break
        pass
    # add process to the dictionary
    if not already_in:
        import types
        if type(command) == types.ListType: command=" ".join(command)
        command=command.split()[0]
        try:
            if verbose(): print "addToKillList: %s : %s" % ( str(command_pid), command )
            process_ids.append({int(command_pid): [command]})
            fpid=open(filedict,'w')
            pickle.dump(process_ids, fpid)
            fpid.close()
        except:
            if verbose(): print "addToKillList: can not add command %s to the kill list" % filedict
            pass
        pass
    pass

def killList(port=None):
    """
    Kill all the processes listed in the SALOME processes dictionary file.
    - [port]      : SALOME port number; if this parameter is None (default),
    it is detected automatically
    """
    # retrieve processes dictionary
    from killSalomeWithPort import getPiDict
    if port is None: port=findFileDict()
    filedict=getPiDict(port)
    try:
        fpid=open(filedict, 'r')
        process_ids=pickle.load(fpid)
        fpid.close()
    except:
        process_ids=[]
        pass
    # kill processes
    for process_id in process_ids:
        #print process_id
        for pid, cmd in process_id.items():
            print "stop process %s : %s"% (pid, cmd[0])
            try:
                os.kill(int(pid),signal.SIGKILL)
            except:
                print "  ------------------ process %s : %s inexistant"% (pid, cmd[0])
                pass
            pass
        pass
    # remove processes dictionary file
    os.remove(filedict)
    pass

if __name__ == "__main__":
    if verbose(): print sys.argv
    addToKillList(sys.argv[1], sys.argv[2])
    pass
