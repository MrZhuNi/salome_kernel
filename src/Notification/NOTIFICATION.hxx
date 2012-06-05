// Copyright (C) 2007-2012  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License.
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

//  SALOME Notification : wrapping of Notification service services
//  File   : NOTIFICATION.hxx
//  Author : Laurent DADA / Francis KLOSS
//  Module : SALOME
//
#ifndef NOTIFICATION_HXX
#define NOTIFICATION_HXX

#include "SALOME_NOTIFICATION.hxx"

#define NOTIF_WARNING "Warning"
#define NOTIF_STEP    "Step"
#define NOTIF_TRACE   "Trace"
#define NOTIF_VERBOSE "Verbose"

#define NOTIFICATION_ChannelName "EventChannel"

#include "utilities.h"

// kloss #include <omnithread.h>
#include "CosNotifyShorthands.h"

#include "NOTIFICATION_Supplier.hxx"
#include "NOTIFICATION_Consumer.hxx"

NOTIFICATION_EXPORT char* NOTIFICATION_date();
NOTIFICATION_EXPORT CosNA_EventChannel_ptr NOTIFICATION_channel();

#endif
