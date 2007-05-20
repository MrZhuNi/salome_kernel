#! /usr/bin/env python
#
#  SALOME Container : implementation of container and engine for Kernel
#
#  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
#  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS 
# 
#  This library is free software; you can redistribute it and/or 
#  modify it under the terms of the GNU Lesser General Public 
#  License as published by the Free Software Foundation; either 
#  version 2.1 of the License. 
# 
#  This library is distributed in the hope that it will be useful, 
#  but WITHOUT ANY WARRANTY; without even the implied warranty of 
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
#  Lesser General Public License for more details. 
# 
#  You should have received a copy of the GNU Lesser General Public 
#  License along with this library; if not, write to the Free Software 
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA 
# 
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#
#
#
#  File   : SALOME_ComponentPy.py
#  Author : Paul RASCLE, EDF
#  Module : SALOME
#  $Header$

import os
import sys
import time
import string
import signal
from omniORB import CORBA, PortableServer
import Engines, Engines__POA
import Registry
from Utils_Identity import *
from SALOME_NamingServicePy import *
from libNOTIFICATION import *

from SALOME_utilities import *

from thread import *

#=============================================================================

_Sleeping = 0

#define an implementation of the component interface

class SALOME_ComponentPy_i (Engines__POA.Component):
    _orb = None
    _poa = None
    _fieldsDict = []
    _studyId = -1
    
    #-------------------------------------------------------------------------

    def __init__ (self, orb, poa, contID, containerName,
                  instanceName, interfaceName, notif=0):
        # Notif for notification services
        # NOT YET IMPLEMENTED
        MESSAGE(  "SALOME_ComponentPy_i::__init__" + " " + str (containerName) + " " + str(instanceName) + " " + str(interfaceName) )
        self._orb = orb
        self._poa = poa
        self._instanceName = instanceName
        self._interfaceName = interfaceName
        self._containerName = containerName
        self._notif = notif
        self._myConnexionToRegistry = 0
        self._graphName = ''
        self._nodeName = ''
        self._ThreadId = 0
        self._StartUsed = 0
        self._ThreadCpuUsed = 0
        self._Executed = 0
        self._contId = contID

        naming_service = SALOME_NamingServicePy_i(self._orb)
        myMachine=getShortHostName()
        Component_path = self._containerName + "/" + self._instanceName
        MESSAGE(  'SALOME_ComponentPy_i Register' + str( Component_path ) )
        naming_service.Register(self._this(), Component_path)

        # Add componentinstance to registry
        obj = naming_service.Resolve('/Registry')
        if obj is None:
            MESSAGE(  "Registry Reference is invalid" )
        else:
            regist = obj._narrow(Registry.Components)
            if regist is None:
                MESSAGE(  "Registry Reference is invalid" )
            else:
                ior = orb.object_to_string(contID)
                MESSAGE(  ior )

                lesInfos = Identity(self._instanceName)
                infos = Registry.Infos(lesInfos._name,
                                       lesInfos._pid,
                                       lesInfos._machine,
                                       lesInfos._adip,
                                       lesInfos._uid,
                                       lesInfos._pwname,
                                       int(lesInfos._tc_start),
                                       0,0,0,
                                       lesInfos._cdir,
                                       -1,
                                       ior)

                res = regist.add(infos)

        self._notifSupplier = NOTIFICATION_Supplier_Swig(instanceName, notif)

    #-------------------------------------------------------------------------

    def _get_instanceName(self):
        MESSAGE(  "SALOME_ComponentPy_i::_get_instanceName" )
        return self._instanceName
    
    #-------------------------------------------------------------------------

    def _get_interfaceName(self):
        MESSAGE(  "SALOME_ComponentPy_i::_get_interfaceName" )
        return self._interfaceName

    #-------------------------------------------------------------------------

    def ping(self):
        MESSAGE(  "SALOME_ComponentPy_i::ping() pid " + str(os.getpid()) )
        
    #-------------------------------------------------------------------------

    def setProperties(self, dico):
        self._fieldsDict = dico
    
    #-------------------------------------------------------------------------

    def getProperties(self):
        return self._fieldsDict

    #-------------------------------------------------------------------------

    def destroy(self):
        MESSAGE(  "SALOME_ComponentPy_i::destroy" )
        self._poa.deactivate_object(self)
        CORBA.release(self._poa)
        
    #-------------------------------------------------------------------------

    def GetContainerRef(self):
        MESSAGE(  "SALOME_ComponentPy_i::GetContainerRef" )
        #corbaObj_ptr = self._poa.id_to_reference(self._contId)
        #return corbaObj_ptr._narrow(Engines.Container)
        return self._contId._narrow(Engines.Container)
                
    #-------------------------------------------------------------------------

    def beginService(self , serviceName ):
        MESSAGE(  "Send BeginService notification for " + str(serviceName) + " for graph/node " + str(self._graphName) + " " + str(self._nodeName) )
        MESSAGE(  "Component instance : " + str ( self._instanceName ) )
        self._serviceName = str(serviceName)
        self._ThreadId = get_ident()
        self._StartUsed = 0
        self._StartUsed = self.CpuUsed_impl()
        self._ThreadCpuUsed = 0
        self._Executed = 1
        MESSAGE( "SALOME_ComponentPy_i::beginService _StartUsed " + str( self._ThreadId ) + " " + str( self._StartUsed ) )
        

    #-------------------------------------------------------------------------

    def endService(self , serviceName ):
        MESSAGE(  "Send EndService notification for " + str( self._ThreadId ) + " " + str(serviceName) + " for graph/node " + str(self._graphName) + " " + str(self._nodeName) + " CpuUsed " + str( self.CpuUsed_impl() ) )
        MESSAGE(  "Component instance : " + str(self._instanceName) )

    #-------------------------------------------------------------------------

    def sendMessage(self , event_type , message ):
        self._notifSupplier.Send(self.graphName(), self.nodeName(), event_type, message)

    #-------------------------------------------------------------------------

    def Names(self , GraphName , NodeName ):
        MESSAGE(  "SALOME_ComponentPy_i::Names" + str(GraphName) + str(NodeName) )
        self._graphName = GraphName
        self._nodeName = NodeName
        
    #-------------------------------------------------------------------------

    def graphName(self):
        return self._graphName

    #-------------------------------------------------------------------------

    def nodeName(self):
        return self._nodeName

    #-------------------------------------------------------------------------

    def Killer(self, ThreadId, signum):
        #if ThreadId > 0:
            #if signum == 0:
                #if pthread_cancel(ThreadId): <- from C++
                #   return 0
                #else:
                #   MESSAGE()
            #else:
                #if pthread_kill(ThreadId): <- from C++
                #   return 0
                #else:
                #   MESSAGE()
        return 1                 
    
    #-------------------------------------------------------------------------

    def Kill_impl(self):
        MESSAGE(  "SALOME_ComponentPy_i::Kill_impl" )
        RetVal = 0
        if self._ThreadId > 0 & self._ThreadId != get_ident():
            RetVal = Killer(self._ThreadId,0)
            self._ThreadId = 0
        return RetVal

    #-------------------------------------------------------------------------

    def Stop_impl(self):
        MESSAGE(  "SALOME_ComponentPy_i::Stop_impl" )
        RetVal = 0
        if self._ThreadId > 0 & self._ThreadId != get_ident():
            RetVal = Killer(self._ThreadId,0)
            self._ThreadId = 0
        return RetVal

    #-------------------------------------------------------------------------

    def Suspend_impl(self):
        MESSAGE(  "SALOME_ComponentPy_i::Suspend_impl" )
        global _Sleeping
        RetVal = 0
        if self._ThreadId > 0 & self._ThreadId != get_ident():
            if _Sleeping > 0:
                return 0
            else:
                RetVal = Killer(self._ThreadId, signal.SIGINT)
                if RetVal > 0:
                    _Sleeping = 1
        return RetVal

    #-------------------------------------------------------------------------

    def Resume_impl(self):
        MESSAGE(  "SALOME_ComponentPy_i::Resume_impl" )
        global _Sleeping
        RetVal = 0
        if self._ThreadId > 0 & self._ThreadId != get_ident():
            if _Sleeping > 0:
                _Sleeping = 0
                RetVal = 1
            else:
                RetVal = 0
        return RetVal

    #-------------------------------------------------------------------------

    def CpuUsed_impl(self):
        if ( self._ThreadId | self._Executed ) :
            if self._ThreadId == get_ident() :
                cpu = time.clock()
                self._ThreadCpuUsed = int(cpu) - self._StartUsed
                MESSAGE( "SALOME_ComponentPy_i::CpuUsed_impl " + self._serviceName + " " + str( int(cpu) ) + " - " + str( self._StartUsed ) + " = " + str( self._ThreadCpuUsed ) )
                return self._ThreadCpuUsed
            MESSAGE( "SALOME_ComponentPy_i::CpuUsed_impl " + self._serviceName + " " + str( self._ThreadCpuUsed ) )
            return self._ThreadCpuUsed
        MESSAGE( "SALOME_ComponentPy_i::CpuUsed_impl self._StartUsed " + self._serviceName + " " + str(self._StartUsed) )
        return 0

    #-------------------------------------------------------------------------
   
    def DumpPython(self, theStudy, isPublished):
        aBuffer = "def RebuildData(theStudy): pass\n\0"
	return (aBuffer, 1)

    #-------------------------------------------------------------------------    

    def getStudyId(self):
        return self._studyId
