#! /usr/bin/env python3
# Copyright (C) 2021  CEA/DEN, EDF R&D
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
import setenv
import runSalomeCommon
import os

class NoSessionServer(runSalomeCommon.CommonSessionServer):
    def __init__(self,args,modules_list,modules_root_dir):
        super().__init__(args,modules_list,modules_root_dir)
        for i in range (len(self.SCMD1)):
            if self.SCMD1[i] == "SALOME_Session_Server" :
                self.SCMD1[i] = "SALOME_Session_Server_No_Server"
        SalomeAppSLConfig=os.getenv("SalomeAppConfig","")
        os.putenv("SalomeAppSLConfig", SalomeAppSLConfig)
    def getSessionServerExe(self):
        return "SALOME_Session_Server_No_Server"

def main():
    args, modules_list, modules_root_dir = setenv.get_config()
    mySessionServ = NoSessionServer(args, args.get('modules', []), modules_root_dir)
    mySessionServ.setpath(modules_list, modules_root_dir)
    mySessionServ.run()

if __name__ == "__main__":
    main()
