// Andr� Ribes - EDF R&D
#include "Superv_Component_i.hxx"

DSC_EXCEPTION_CXX(Superv_Component_i,BadFabType);
DSC_EXCEPTION_CXX(Superv_Component_i,BadType);
DSC_EXCEPTION_CXX(Superv_Component_i,BadCast);
DSC_EXCEPTION_CXX(Superv_Component_i,UnexpectedState);
DSC_EXCEPTION_CXX(Superv_Component_i,PortAlreadyDefined);
DSC_EXCEPTION_CXX(Superv_Component_i,PortNotDefined);
DSC_EXCEPTION_CXX(Superv_Component_i,PortNotConnected);
DSC_EXCEPTION_CXX(Superv_Component_i,NilPort);
DSC_EXCEPTION_CXX(Superv_Component_i,BadProperty);

Superv_Component_i::Superv_Component_i(CORBA::ORB_ptr orb,
				       PortableServer::POA_ptr poa,
				       PortableServer::ObjectId * contId,
				       const char *instanceName,
				       const char *interfaceName,
				       bool notif) : 
  Engines_DSC_i(orb, poa, contId, instanceName, interfaceName) 
{
  _my_basic_factory = new basic_port_factory();
  _my_palm_factory = new palm_port_factory();
  _my_calcium_factory = new calcium_port_factory();
}

  
Superv_Component_i::~Superv_Component_i() {
  delete _my_basic_factory;
}

// Dans ces m�thodes, on analyse le type pour conna�tre
// quel constructeur appel�.
// Le nom du port est sous la forme :
// TYPE_name, exemple : BASIC_short
//                    : PALM_sequence_long
//                    : CALCIUM_long
//                    : etc ...

provides_port *
Superv_Component_i::create_provides_data_port(const char* port_fab_type)
  throw (BadFabType)
{ 
  assert(port_fab_type);

  provides_port * rtn_port = NULL;
  string the_type(port_fab_type);
  int search_result;

  search_result = the_type.find("BASIC_");
  if (search_result == 0) {
    rtn_port = _my_basic_factory->create_data_servant(the_type.substr(search_result+6, 
								      the_type.length()));
  }
  search_result = the_type.find("PALM_");
  if (search_result == 0) {
    rtn_port = _my_palm_factory->create_data_servant(the_type.substr(search_result+5, 
								     the_type.length()));
  }

  search_result = the_type.find("CALCIUM_");
  if (search_result == 0) {
    rtn_port = _my_calcium_factory->create_data_servant(the_type.substr(search_result+8, the_type.length()));
  }

  if (rtn_port == NULL)
    throw  BadFabType( LOC(OSS()<< "Impossible d'acc�der � la fabrique "
			   <<port_fab_type));

  return rtn_port;
}

uses_port *
Superv_Component_i::create_uses_data_port(const char* port_fab_type) 
throw (BadFabType)
{
  assert(port_fab_type);

  uses_port * rtn_proxy = NULL;
  string the_type(port_fab_type);
  int search_result;

  search_result = the_type.find("BASIC_");
  if (search_result == 0) {
    rtn_proxy = _my_basic_factory->create_data_proxy(the_type.substr(search_result+6, 
								     the_type.length()));
  }
  
  search_result = the_type.find("CALCIUM_");
  if (search_result == 0) {
    std::cout << "---- Superv_Component_i::create_uses_data_port : MARK 1 ----  " << the_type.substr(search_result+8, the_type.length()) << "----" << std::endl;
    rtn_proxy = _my_calcium_factory->create_data_proxy(the_type.substr(search_result+8, the_type.length()));
  }
  
  if (rtn_proxy == NULL)
   throw BadFabType( LOC(OSS()<< "Impossible d'acc�der � la fabrique "
			  <<port_fab_type));

  return rtn_proxy;
}

void
Superv_Component_i::add_port(const char * port_fab_type,
			     const char * port_type,
			     const char * port_name)
  throw (PortAlreadyDefined, BadFabType, BadType, BadProperty)
{
  assert(port_fab_type);
  assert(port_type);
  assert(port_name);

  std::string s_port_type(port_type);
  if (s_port_type == "provides") {
    provides_port * port = create_provides_data_port(port_fab_type);
    add_port(port, port_name);
  }
  else if (s_port_type == "uses") {
    std::cout << "---- Superv_Component_i::add_port : MARK 1 ---- "  << std::endl;
    uses_port * port = create_uses_data_port(port_fab_type);
    std::cout << "---- Superv_Component_i::add_port : MARK 2 ---- "  << std::endl;
    add_port(port, port_name);
  }
  else
    throw BadType( LOC(OSS()<< "Le port_type doit �tre soit 'provides' soit 'uses' not "
		       << port_type));

}

void 
Superv_Component_i::add_port(provides_port * port, 
			     const char* provides_port_name) 
  throw (PortAlreadyDefined, NilPort, BadProperty)
{
  assert(port);
  assert(provides_port_name);

  try {

    Engines_DSC_interface::add_provides_port(port->get_port_ref(), 
					     provides_port_name,
					     port->get_port_properties());

    superv_port_t * new_superv_port = new superv_port_t();
    new_superv_port->p_ref = port;
    my_superv_ports[provides_port_name] = new_superv_port;

  } 
  catch (const Engines::DSC::PortAlreadyDefined&) {
    throw PortAlreadyDefined( LOC(OSS()<< "Le port provides "
				  << provides_port_name <<" existe d�j�."));
  } 
  catch (const Engines::DSC::NilPort&) {
    throw NilPort( LOC(OSS()<< "Le pointeur sur port provides est nul."));
  }
  catch (const Engines::DSC::BadProperty&) {
    throw BadProperty( LOC(OSS()<< "La propri�t� est mal d�finie"));
  }
}

void
Superv_Component_i::add_port(uses_port * port, 
			     const char* uses_port_name) 
  throw (PortAlreadyDefined, NilPort, BadProperty)
{
  assert(port);
  assert(uses_port_name);

  try {
    Engines_DSC_interface::add_uses_port(port->get_repository_id(), 
					 uses_port_name,
					 port->get_port_properties());
    superv_port_t * new_superv_port = new superv_port_t();
    new_superv_port->u_ref = port;
    my_superv_ports[uses_port_name] = new_superv_port;
  } 
  catch (const Engines::DSC::PortAlreadyDefined&) {
    throw PortAlreadyDefined( LOC(OSS()<< "Le port uses " 
				  << uses_port_name <<" existe d�j�."));
  } 
  catch (const Engines::DSC::NilPort&) {
    throw NilPort( LOC(OSS()<< "Le pointeur sur port uses est nul."));
  }
  catch (const Engines::DSC::BadProperty&) {
    throw BadProperty( LOC(OSS()<< "La propri�t� est mal d�finie"));
  }
}

void
Superv_Component_i::get_port(provides_port *& port,
			     const char * provides_port_name)
  throw (PortNotDefined,PortNotConnected)
{
  assert(provides_port_name);

  try {
    Engines_DSC_interface::get_provides_port(provides_port_name, false);
    port = my_superv_ports[provides_port_name]->p_ref;
  } catch (const Engines::DSC::PortNotDefined&) {
    throw PortNotDefined( LOC(OSS()<< "Le port provides  "
			      << provides_port_name <<" n'existe pas."));
  } catch (const Engines::DSC::PortNotConnected&) {
    throw PortNotConnected( LOC(OSS()<< "Le port provides " << provides_port_name 
				<< " n'est pas connect�."));
  }
}

void
Superv_Component_i::get_port(uses_port *& port,
			     const char * uses_port_name)
  throw (PortNotDefined, PortNotConnected)
{
  assert(uses_port_name);

  try {
    Engines_DSC_i::get_uses_port(uses_port_name);
    port = my_superv_ports[uses_port_name]->u_ref;
  } catch (const Engines::DSC::PortNotDefined&) {    
    throw PortNotDefined( LOC(OSS()<< "Le port uses  "
			      << uses_port_name <<" n'existe pas."));
  } catch (const Engines::DSC::PortNotConnected&) {
    throw PortNotConnected( LOC(OSS()<< "Le port uses " << uses_port_name 
				<< " n'est pas connect�."));
  }
}



void
Superv_Component_i::provides_port_changed(const char* provides_port_name,
					  int connection_nbr,
					  const Engines::DSC::Message message)
{
  // On cherche tout d'abord � savoir si le port
  // est un port que l'on a dans la liste ...
  my_superv_ports_it = my_superv_ports.find(provides_port_name);
  if (my_superv_ports_it !=  my_superv_ports.end())
    my_superv_ports[provides_port_name]->p_ref->provides_port_changed(connection_nbr,
								      message);
}

void
Superv_Component_i::uses_port_changed(const char* uses_port_name,
				      Engines::DSC::uses_port * new_uses_port,
				      const Engines::DSC::Message message)
{
  // On cherche tout d'abord � savoir si le port
  // est un port que l'on a dans la liste ...
  my_superv_ports_it = my_superv_ports.find(uses_port_name);
  if (my_superv_ports_it !=  my_superv_ports.end())
    my_superv_ports[uses_port_name]->u_ref->uses_port_changed(new_uses_port,
							      message);
}



void
Superv_Component_i::get_uses_port_names(std::vector<std::string> & port_names,
					const std::string servicename) const {

  port_names.reserve(my_superv_ports.size());

  superv_ports::const_iterator it;

  for (it=my_superv_ports.begin(); it!=my_superv_ports.end();++it)
    if( (*it).second->p_ref == NULL ) port_names.push_back((*it).first);
}
