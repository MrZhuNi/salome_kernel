//  Copyright (C) 2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : ParallelDSC_i.hxx
//  Author : Andr� RIBES (EDF)
//  Module : KERNEL

#ifndef _PARALLEL_DSC_COMPONENT_I_HXX_
#define _PARALLEL_DSC_COMPONENT_I_HXX_

#include <iostream>
#include <map>

#include "DSC_EnginesPaCO_Engines_Parallel_DSC_server.h"
#include "DSC_interface.hxx"
#include "SALOME_ParallelComponent_i.hxx"

class Engines_ParallelDSC_i: 
  public virtual Engines_Parallel_Component_i,
  public virtual Engines::Parallel_DSC_serv,
  public virtual Engines_DSC_interface 
{
public:
  Engines_ParallelDSC_i(CORBA::ORB_ptr orb, char * ior,
			PortableServer::POA_ptr poa,
			PortableServer::ObjectId * contId,
			const char *instanceName,
			const char *interfaceName,
			bool notif = false);

  virtual ~Engines_ParallelDSC_i();

  
  virtual CORBA::Boolean init_service(const char* service_name) {return true;};

  /*!
   * \see Engines::DSC::add_provides_port
   */
  virtual void add_provides_port(Ports::Port_ptr ref, 
				 const char* provides_port_name,
				 Ports::PortProperties_ptr port_prop) 
    throw (Engines::DSC::PortAlreadyDefined,
	   Engines::DSC::NilPort,
	   Engines::DSC::BadProperty) {
      Engines_DSC_interface::add_provides_port(ref, 
					       provides_port_name,
					       port_prop);
    }

  /*!
   * \see Engines::DSC::add_uses_port
   */
  virtual void add_uses_port(const char* repository_id, 
			     const char* uses_port_name,
			     Ports::PortProperties_ptr port_prop)
    throw (Engines::DSC::PortAlreadyDefined,
	   Engines::DSC::BadProperty) {
      Engines_DSC_interface::add_uses_port(repository_id, 
					   uses_port_name,
					   port_prop);
    }

  /*!
   * \see Engines::DSC::get_provides_port
   */
  virtual Ports::Port_ptr get_provides_port(const char* provides_port_name,
					    const CORBA::Boolean connection_error) 
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected) {
      return Engines_DSC_interface::get_provides_port(provides_port_name,
						      connection_error);
    }

  /*!
   * \see Engines::DSC::get_uses_port
   */
  virtual Engines::DSC::uses_port * get_uses_port(const char* uses_port_name)
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected) {
      return Engines_DSC_interface::get_uses_port(uses_port_name);
    }

  /*!
   * \see Engines::DSC::connect_provides_port
   */
  virtual void connect_provides_port(const char* provides_port_name)
    throw (Engines::DSC::PortNotDefined) {
      Engines_DSC_interface::connect_provides_port(provides_port_name);
    }

  /*!
   * \see Engines::DSC::connect_uses_port
   */
  virtual void connect_uses_port(const char* uses_port_name,
				 Ports::Port_ptr provides_port_ref) 
  throw (Engines::DSC::PortNotDefined,
	 Engines::DSC::BadPortType,
	 Engines::DSC::NilPort) {
    Engines_DSC_interface::connect_uses_port(uses_port_name,
					     provides_port_ref);
  }

  /*!
   * \see Engines::DSC::is_connected
   */
  virtual CORBA::Boolean is_connected(const char* port_name)
    throw (Engines::DSC::PortNotDefined) {
      return Engines_DSC_interface::is_connected(port_name);
    }

   /*!
   * \see Engines::DSC::disconnect_provides_port
   */
  virtual void disconnect_provides_port(const char* provides_port_name,
					const Engines::DSC::Message message)
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected) {
      Engines_DSC_interface::disconnect_provides_port(provides_port_name,
						      message);
    }

   /*!
   * \see Engines::DSC::disconnect_uses_port
   */
  virtual void disconnect_uses_port(const char* uses_port_name,
				    Ports::Port_ptr provides_port_ref,
				    const Engines::DSC::Message message)
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected,
	   Engines::DSC::BadPortReference) {
      Engines_DSC_interface::disconnect_uses_port(uses_port_name,
						  provides_port_ref,
						  message);
    }

  virtual Ports::PortProperties_ptr get_port_properties(const char* port_name)
    throw (Engines::DSC::PortNotDefined) {
      return Engines_DSC_interface::get_port_properties(port_name);
    }
  
  // PaCO++ specific code

  // This method is used to registry the proxy of the parallel port into
  // all the nodes of the parallel component.
  virtual void set_paco_proxy(CORBA::Object_ptr ref, 
			      const char* provides_port_name,
			      Ports::PortProperties_ptr port_prop);

  // This method by the node that want to add the parallel proxy port.
  virtual CORBA::Boolean add_parallel_provides_proxy_port(const CORBA::Object_ptr ref, 
							  const char * provides_port_name);

  // This method is used that the parallel componet node 
  // knows the CORBA reference of the parallel port.
  virtual CORBA::Boolean add_parallel_provides_proxy_wait(const char * provides_port_name);

  // Permits to add a parallel node of a parallel port.
  virtual CORBA::Boolean add_parallel_provides_node_port(Ports::Port_PaCO_ptr ref, 
							 const char* provides_port_name);
  // Used to get the proxy of the parallel port.
  virtual const char * get_proxy(const char* provides_port_name);

};

#endif
