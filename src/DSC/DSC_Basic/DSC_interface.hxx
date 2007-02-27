
// Andr� Ribes EDF R&D - 2006
// 
#ifndef _DSC_INTERFACE_HXX_
#define _DSC_INTERFACE_HXX_

#include <iostream>
#include <map>
#include <string.h>
#include <assert.h>

#include "DSC_Callbacks.hxx"

/*! \class Engines_DSC_interface
 *  \brief This class implements the interface Engines::DSC
 */
class Engines_DSC_interface: 
  public DSC_Callbacks
{
public:
  Engines_DSC_interface();
  virtual ~Engines_DSC_interface();

  /*!
   * \see Engines::DSC::add_provides_port
   */
  virtual void add_provides_port(Ports::Port_ptr ref, 
				 const char* provides_port_name,
				 Ports::PortProperties_ptr port_prop) 
    throw (Engines::DSC::PortAlreadyDefined,
	   Engines::DSC::NilPort,
	   Engines::DSC::BadProperty);

  /*!
   * \see Engines::DSC::add_uses_port
   */
  virtual void add_uses_port(const char* repository_id, 
			     const char* uses_port_name,
			     Ports::PortProperties_ptr port_prop)
    throw (Engines::DSC::PortAlreadyDefined,
	   Engines::DSC::BadProperty);

  /*!
   * \see Engines::DSC::get_provides_port
   */
  virtual Ports::Port_ptr get_provides_port(const char* provides_port_name,
					    const CORBA::Boolean connection_error) 
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected,
	   Engines::DSC::BadPortType);

  /*!
   * \see Engines::DSC::get_uses_port
   */
  virtual Engines::DSC::uses_port * get_uses_port(const char* uses_port_name)
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected,
	   Engines::DSC::BadPortType);

  /*!
   * \see Engines::DSC::connect_provides_port
   */
  virtual void connect_provides_port(const char* provides_port_name)
    throw (Engines::DSC::PortNotDefined);

  /*!
   * \see Engines::DSC::connect_uses_port
   */
  virtual void connect_uses_port(const char* uses_port_name,
				 Ports::Port_ptr provides_port_ref) 
  throw (Engines::DSC::PortNotDefined,
	 Engines::DSC::BadPortType,
	 Engines::DSC::NilPort);

  /*!
   * \see Engines::DSC::is_connected
   */
  virtual CORBA::Boolean is_connected(const char* port_name)
    throw (Engines::DSC::PortNotDefined);

   /*!
   * \see Engines::DSC::disconnect_provides_port
   */
  virtual void disconnect_provides_port(const char* provides_port_name,
					const Engines::DSC::Message message)
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected);

   /*!
   * \see Engines::DSC::disconnect_uses_port
   */
  virtual void disconnect_uses_port(const char* uses_port_name,
				    Ports::Port_ptr provides_port_ref,
				    const Engines::DSC::Message message)
    throw (Engines::DSC::PortNotDefined,
	   Engines::DSC::PortNotConnected,
	   Engines::DSC::BadPortReference);

  virtual Ports::PortProperties_ptr get_port_properties(const char* port_name)
    throw (Engines::DSC::PortNotDefined);

protected:

  /*-------------------------------------------------*/
  /* Definition des types pour le stockage des ports */
  
  enum port_type {uses, provides, none};

  struct port_t {
    port_type type;
    int connection_nbr;
    
    // Specifique aux uses port
    Engines::DSC::uses_port uses_port_refs;
    std::string repository_id;

    // Specifique aux provides port;
    Ports::Port_ptr provides_port_ref;

    Ports::PortProperties_ptr port_prop;
  };

  typedef std::map<std::string, port_t *> ports;

  /*-------------------------------------------------*/
  /*-------------------------------------------------*/
 
  ports my_ports;
  ports::iterator my_ports_it;
};

#endif
