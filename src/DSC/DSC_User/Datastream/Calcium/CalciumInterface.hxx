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
//  File   : CalciumInterface.hxx
//  Author : Eric Fayolle (EDF)
//  Module : KERNEL
// Modified by : $LastChangedBy$
// Date        : $LastChangedDate: 2007-03-01 13:27:58 +0100 (jeu, 01 mar 2007) $
// Id          : $Id$

#ifndef _CALCIUM_INTERFACE_HXX_
#define _CALCIUM_INTERFACE_HXX_

#include <string>
#include <vector>
#include <iostream>
#include "Superv_Component_i.hxx"
#include "CalciumException.hxx"
#include "CalciumTypes.hxx"
#include "CalciumGenericUsesPort.hxx"
#include "Copy2UserSpace.hxx"
#include "Copy2CorbaSpace.hxx"
#include "CalciumPortTraits.hxx"

#include <stdio.h>

//#define _DEBUG_

// D�placer cette information dans CorbaTypeManipulator
// G�rer en m�me temps la recopie profonde.

template <typename T1, typename T2>
struct IsSameType {
  static const bool value = false;
};
template <typename T1>
struct IsSameType<T1,T1> {
  static const bool value = true;
};




//class CalciumInterface {
namespace CalciumInterface {
//public :


  static void
  ecp_fin (Superv_Component_i & component, bool provideLastGivenValue)
  { 
    std::vector<std::string> usesPortNames;
    std::vector<std::string>::const_iterator it;
    component.get_uses_port_names(usesPortNames);    
    
    //r�cup�rer le type de r�el du port est un peu difficile
    //car l'interface nous donne aucune indication

    //     uses_port *myUsesPort;
    calcium_uses_port* myCalciumUsesPort;
      
    for (it=usesPortNames.begin(); it != usesPortNames.end(); ++it) {
      try {

	myCalciumUsesPort= 
	  component.Superv_Component_i::get_port< calcium_uses_port >((*it).c_str());

// 	component.Superv_Component_i::get_port(myUsesPort,(*it).c_str());
// 	calcium_uses_port* myCalciumUsesPort=
// 	  dynamic_cast<calcium_uses_port*>(myUsesPort);

#ifdef _DEBUG_
	std::cerr << "-------- CalciumInterface(ecp_fin) MARK 1 -|"<< *it <<"|----"<< 
	  //	  typeid(myUsesPort).name() <<"-------------" <<
	  typeid(myCalciumUsesPort).name() <<"-------------" << std::endl;
#endif
	
// 	if ( !myCalciumUsesPort )
// 	  throw Superv_Component_i::BadCast(LOC(OSS()<<"Impossible de convertir le port "
// 						<< *it << " en port de type calcium_uses_port." ));

	myCalciumUsesPort->disconnect(provideLastGivenValue);

      } catch ( const Superv_Component_i::BadCast & ex) {
#ifdef _DEBUG_
 	std::cerr << ex.what() << std::endl;
#endif
 	throw (CalciumException(CalciumTypes::CPTPVR,ex));
      } catch ( const DSC_Exception & ex) {
#ifdef _DEBUG_
	std::cerr << ex.what() << std::endl;
#endif
	// Exception venant de SupervComponent :
	//   PortNotDefined(CPNMVR), PortNotConnected(CPLIEN)  
	// ou du port uses : Dsc_Exception
	// On continue � traiter la deconnexion des autres ports uses
      } catch (...) {
 	throw (CalciumException(CalciumTypes::CPATAL,"Exception innatendue"));
	// En fonction du mode de gestion des erreurs throw;
      }
    }
  }


  // Uniquement appel� par l'utilisateur s'il a pass� un pointeur de donn�es NULL
  // � l'appel de ecp_lecture (demande de 0 copie)
  template <typename T1, typename T2> static void
  ecp_free ( T1 * dataPtr )
  {
    typedef typename ProvidesPortTraits<T2>::PortType     PortType;
    typedef typename PortType::DataManipulator            DataManipulator;
    typedef typename DataManipulator::Type                DataType; // Attention != T
    typedef typename DataManipulator::InnerType           InnerType;

    DeleteTraits<IsSameType<T1,InnerType>::value >::apply(dataPtr);
  }

  template <typename T1> static void
  ecp_free ( T1 * dataPtr )
  {
    ecp_free<T1,T1> ( dataPtr );
  }

  template <typename T1 > static void
  ecp_lecture ( Superv_Component_i & component,
	       CalciumTypes::DependencyType dependencyType,
	       double        & ti,
	       double const  & tf,
	       long          & i,
	       const string  & nomVar, 
	       size_t bufferLength,
	       size_t & nRead, 
	       T1 * &data )
  {
    ecp_lecture<T1,T1> (component,dependencyType,ti,tf,
			i,nomVar,bufferLength,nRead,data);
  
  }

  template <typename T1, typename T2 > static void
  ecp_lecture ( Superv_Component_i & component,
	       CalciumTypes::DependencyType dependencyType,
	       double        & ti,
	       double const  & tf,
	       long          & i,
	       const string  & nomVar, 
	       size_t bufferLength,
	       size_t & nRead, 
	       T1 * &data )
  {

    assert(&component);

    typedef typename ProvidesPortTraits<T2>::PortType     PortType;
    typedef typename PortType::DataManipulator            DataManipulator;
    typedef typename DataManipulator::Type                CorbaDataType; // Attention != T
    typedef typename DataManipulator::InnerType           InnerType;

    CorbaDataType     corbaData;
    long         ilong;

#ifdef _DEBUG_
    std::cerr << "-------- CalciumInterface(ecp_lecture) MARK 1 ------------------" << std::endl;
#endif

    if (nomVar.empty())
      throw CalciumException(CalciumTypes::CPNMVR,
				LOC("Le nom de la variable est <nul>"));
    PortType * port;
#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecp_lecture) MARK 2 ------------------" << std::endl;
#endif

    try {
      port  = component.Superv_Component_i::get_port< PortType > (nomVar.c_str());
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 3 ------------------" << std::endl;
#endif
    } catch ( const Superv_Component_i::PortNotDefined & ex) {
#ifdef _DEBUG_
      std::cerr << ex.what() << std::endl;
#endif
      throw (CalciumException(CalciumTypes::CPNMVR,ex));
    } catch ( const Superv_Component_i::PortNotConnected & ex) {
#ifdef _DEBUG_
      std::cerr << ex.what() << std::endl;;
#endif
      throw (CalciumException(CalciumTypes::CPLIEN,ex)); 
      // VERIFIER LES CAS DES CODES : CPINARRET, CPSTOPSEQ, CPCTVR, CPLIEN
    } catch ( const Superv_Component_i::BadCast & ex) {
#ifdef _DEBUG_
      std::cerr << ex.what() << std::endl;
#endif
      throw (CalciumException(CalciumTypes::CPTPVR,ex));
    }
  
    // mode == mode du port 
    CalciumTypes::DependencyType portDependencyType = port->getDependencyType();

    if ( portDependencyType == CalciumTypes::UNDEFINED_DEPENDENCY )
      throw CalciumException(CalciumTypes::CPIT,
			     LOC(OSS()<<"Le mode de d�pendance de la variable " 
				 << nomVar << " est ind�fini."));

    if ( ( portDependencyType != dependencyType ) && 
	 ( dependencyType != CalciumTypes::SEQUENCE_DEPENDENCY ) ) 
      throw CalciumException(CalciumTypes::CPITVR,
			     LOC(OSS()<<"Le mode de d�pendance de la variable " 
				 << nomVar << " ne correspond pas au mode demand�."));

  
    if ( dependencyType == CalciumTypes::TIME_DEPENDENCY ) {
      corbaData = port->get(ti,tf, 0);
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 5 ------------------" << std::endl;
#endif
    } 
    else if ( dependencyType == CalciumTypes::ITERATION_DEPENDENCY ) {
      corbaData = port->get(0, i);
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 6 ------------------" << std::endl;
#endif
    } else {
      // Lecture en s�quence
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 7 ------------------" << std::endl;
#endif
      corbaData = port->next(ti,i);
    }
 
#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecp_lecture) MARK 8 ------------------" << std::endl;
#endif
    size_t corbaDataSize = DataManipulator::size(corbaData);
#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecp_lecture) corbaDataSize : " << corbaDataSize << std::endl;
#endif
   
    // V�rifie si l'utilisateur demande du 0 copie
    if ( data == NULL ) {
      if ( bufferLength != 0 ) {
	MESSAGE("bufferLength devrait valoir 0 pour l'utilisation du mode sans copie (data==NULL)");
      }
      nRead = corbaDataSize;
      // Si les types T et InnerType sont diff�rents, il faudra effectuer tout de m�me une recopie
      if (!IsSameType<T1,InnerType>::value) data = new T1[nRead];
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 9 ------------------" << std::endl;
#endif
      // On essaye de faire du 0 copy si les types T et InnerType sont les m�mes
      Copy2UserSpace< IsSameType<T1,InnerType>::value >::apply(data,corbaData,nRead);
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 10 ------------------" << std::endl;
#endif
      // Attention : Seul CalciumCouplingPolicy via eraseDataId doit d�cider de supprimer ou non
      // la donn�e corba associ�e � un DataId ! Ne pas effectuer la desallocation suivante :
      //  old : Dans les deux cas la structure CORBA n'est plus utile 
      //  old : Si !IsSameType<T1,InnerType>::value l'objet CORBA est d�truit avec son contenu
      //  old : Dans l'autre cas seul la coquille CORBA est d�truite 
      //  tjrs correct : Dans les deux cas l'utilisateur devra appeler ecp_free (version modifi�e)
      // DataManipulator::delete_data(corbaData);
   } else {
      nRead = std::min < size_t > (corbaDataSize,bufferLength);
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 11 ------------------" << std::endl;
#endif
      Copy2UserSpace<false>::apply(data,corbaData,nRead);
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecp_lecture) MARK 12 ------------------" << std::endl;
#endif
      // Attention : Seul CalciumCouplingPolicy via eraseDataId doit d�cider de supprimer ou non
      // la donn�e corba associ�e � un DataId ! Ne pas effectuer la desallocation suivante :
      //      DataManipulator::delete_data(corbaData);
   }
#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecp_lecture), Valeur de data : " << std::endl;
    std::copy(data,data+nRead,std::ostream_iterator<T1>(std::cout," "));
    std::cout << "Ptr :" << data << std::endl;

    std::cout << "-------- CalciumInterface(ecp_lecture) MARK 13 ------------------" << std::endl;
#endif
 
  
    return;
  }


  template <typename T1> static void
  ecp_ecriture ( Superv_Component_i & component,
		 CalciumTypes::DependencyType dependencyType,
		 double const  & t,
		 long   const  & i,
		 const string  & nomVar, 
		 size_t bufferLength,
		 T1  & data ) {
    ecp_ecriture<T1,T1> (component,dependencyType,t,i,nomVar,bufferLength,data); 
  }

  template <typename T1, typename T2> static void
  ecp_ecriture ( Superv_Component_i & component,
		 CalciumTypes::DependencyType dependencyType,
		 double const  & t,
		 long   const  & i,
		 const string  & nomVar, 
		 size_t bufferLength,
		 T1  & data ) 
  {
    
    assert(&component);

    //typedef typename StarTrait<TT>::NonStarType           T;
    typedef typename UsesPortTraits<T2>::PortType          PortType;
    typedef typename ProvidesPortTraits<T2>::PortType      ProvidesPortType;
    typedef typename ProvidesPortType::DataManipulator     DataManipulator;
    // Verifier que l'on peut d�finir UsesPortType::DataManipulator
    //    typedef typename PortType::DataManipulator            DataManipulator;
    typedef typename DataManipulator::Type                CorbaDataType; // Attention != T1
    typedef typename DataManipulator::InnerType           InnerType;

#ifdef _DEBUG_
    std::cerr << "-------- CalciumInterface(ecriture) MARK 1 ------------------" << std::endl;
#endif
    if ( nomVar.empty() ) throw CalciumException(CalciumTypes::CPNMVR,
						    LOC("Le nom de la variable est <nul>"));
    PortType * port;
#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecriture) MARK 2 ------------------" << std::endl;
#endif

    try {
      port  = component.Superv_Component_i::get_port< PortType > (nomVar.c_str());
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecriture) MARK 3 ------------------" << std::endl;
#endif
    } catch ( const Superv_Component_i::PortNotDefined & ex) {
#ifdef _DEBUG_
      std::cerr << ex.what() << std::endl;
#endif
      throw (CalciumException(CalciumTypes::CPNMVR,ex));
    } catch ( const Superv_Component_i::PortNotConnected & ex) {
#ifdef _DEBUG_
      std::cerr << ex.what() << std::endl;;
#endif
      throw (CalciumException(CalciumTypes::CPLIEN,ex)); 
      // VERIFIER LES CAS DES CODES : CPINARRET, CPSTOPSEQ, CPCTVR, CPLIEN
    } catch ( const Superv_Component_i::BadCast & ex) {
#ifdef _DEBUG_
      std::cerr << ex.what() << std::endl;
#endif
      throw (CalciumException(CalciumTypes::CPTPVR,ex));
    }
 
    // mode == mode du port 
    // On pourrait cr�er la m�thode CORBA dans le mode de Couplage CALCIUM.
    // et donc ajouter cette cette m�thode uniquement dans l'IDL calcium !

//     CalciumTypes::DependencyType portDependencyType;
//     try {
//       portDependencyType = port->getDependencyType();
//       std::cout << "-------- CalciumInterface(ecriture) MARK 4 ------------------" << std::endl;
//     } catch ( const DSC_Exception & ex ) {
//       std::cerr << ex.what() << std::endl;;
//       throw (CalciumException(CalciumTypes::CPIT,ex));
//     }

    if ( dependencyType == CalciumTypes::UNDEFINED_DEPENDENCY )
      throw CalciumException(CalciumTypes::CPIT,
				LOC(OSS()<<"Le mode de d�pendance demand� pour la variable " 
				    << nomVar << " est ind�fini."));

    if ( dependencyType == CalciumTypes::SEQUENCE_DEPENDENCY )
      throw CalciumException(CalciumTypes::CPIT,
				LOC(OSS()<<"Le mode de d�pendance SEQUENCE_DEPENDENCY pour la variable " 
				    << nomVar << " est impossible en �criture."));

    // Il faudrait que le port provides g�n�re une exception si le mode donn�e n'est pas
    // le bon. La seule fa�on de le faire est d'envoyer -1 en temps si on n'est en it�ration
    // et vice-versa pour informer les provides port du mode dans lequel on est. Sinon il faut
    // modifier l'interface IDL pour y ajouter un mode de d�pendance !
    // ---->
//     if ( portDependencyType != dependencyType ) 
//       throw CalciumException(CalciumTypes::CPITVR,
// 				LOC(OSS()<<"Le mode de d�pendance de la variable " 
// 				    << nomVar << " ne correspond pas au mode demand�."));

  
    if ( bufferLength < 1 )
      throw CalciumException(CalciumTypes::CPNTNULL,
				LOC(OSS()<<"Le buffer a envoyer est de taille nulle "));


#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecriture) MARK 4 ------------------" << std::endl;
#endif
    CorbaDataType corbaData;

    
    // Si les types Utilisateurs et CORBA sont diff�rents
    // il faut effectuer une recopie sinon on utilise directement le
    // buffer data pour constituer la s�quence
    // TODO : 
    // - Attention en mode asynchrone il faudra eventuellement
    //   faire une copie des donn�es m�me si elles sont de m�me type.
    // - En cas de collocalisation (du port provide et du port uses)
    //   il est necessaire d'effectuer une recopie du buffer car la
    //   s�quence est envoy�e au port provide par une r�f�rence sur 
    //   la s�quence locale. Or la m�thode put r�cup�re le buffer directement
    //   qui est alors le buffer utilisateur. Il pourrait alors arriv� que :
    //     * Le recepteur efface le buffer emetteur
    //     * Le port lui-m�me efface le buffer de l'ulisateur !
    //   Cette copie est effectu�e dans GenericPortUses::put 
    //   en fonction de la collocalisation ou non.
    // - En cas de connection multiples d'un port uses distant vers plusieurs port provides
    //   collocalis�s les ports provides partagent la m�me copie de la donn�e ! 
    //   Il faut effectuer une copie dans le port provides.
    //   Cette copie est effectu�e dans GenericPortUses::put 
    //   en fonction de la collocalisation ou non.
    Copy2CorbaSpace<IsSameType<T1,InnerType>::value >::apply(corbaData,data,bufferLength);
 
    //TODO : GERER LES EXCEPTIONS ICI : ex le port n'est pas connect�
    if ( dependencyType == CalciumTypes::TIME_DEPENDENCY ) {
      try
      {
        port->put(*corbaData,t, -1); 
      }
      catch ( const DSC_Exception & ex) 
      {
        throw (CalciumException(CalciumTypes::CPATAL,ex.what()));
      }
      //Le -1 peut �tre trait� par le cst DataIdContainer et transform� en 0 
      //Etre oblig� de mettre une �toile ds (*corbadata) va poser des pb pour les types <> seq
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecriture) MARK 5 ------------------" << std::endl;
#endif
    } 
    else if ( dependencyType == CalciumTypes::ITERATION_DEPENDENCY ) {
      try
      {
        port->put(*corbaData,-1, i);
      }
      catch ( const DSC_Exception & ex) 
      {
        throw (CalciumException(CalciumTypes::CPATAL,ex.what()));
      }
#ifdef _DEBUG_
      std::cout << "-------- CalciumInterface(ecriture) MARK 6 ------------------" << std::endl;
#endif
    } 

    
#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecriture), Valeur de corbaData : " << std::endl;
    for (int i = 0; i < corbaData->length(); ++i)
      cout << "-------- CalciumInterface(ecriture), corbaData[" << i << "] = " << (*corbaData)[i] << endl;
#endif
    
    //    if ( !IsSameType<T1,InnerType>::value ) delete corbaData;
    // Supprime l'objet CORBA avec eventuellement les donn�es qu'il contient (case de la recopie)
    delete corbaData;

#ifdef _DEBUG_
    std::cout << "-------- CalciumInterface(ecriture) MARK 7 ------------------" << std::endl;
#endif
   
    return;
  }

};

// Interface C/C++

// En CALCIUM l'utilisation de donn�es de type double
// implique des dates de type double, pour les autres
// types de donn�es les dates sont de type float
template <class T> struct CalTimeType {
  typedef float TimeType;
};

template <> struct CalTimeType<double> {
  typedef double TimeType;
};

extern "C"  CalciumTypes::InfoType 
ecp_fin_ (void * component, int code) {

  Superv_Component_i * _component = static_cast<Superv_Component_i *>(component); 

  bool provideLastGivenValue = false;
  if (code == CalciumTypes::CP_CONT ) provideLastGivenValue = true;

  try {									
    CalciumInterface::ecp_fin( *_component,				
			       provideLastGivenValue); 
  } catch ( const CalciumException & ex) { //tester l'arr�t par exception
#ifdef _DEBUG_
    std::cerr << ex.what() << std::endl;				
#endif
    return ex.getInfo();						
  }									
  return CalciumTypes::CPOK;
};

#ifdef _DEBUG_
#define DEBTRACE(msg) {std::cerr<<std::flush<<__FILE__<<" ["<<__LINE__<<"] : "<<msg<<std::endl<<std::flush;}
#else
#define DEBTRACE(msg)
#endif


#define CALCIUM_C2CPP_INTERFACE_(_name,_type,_qual)			\
  extern "C" CalciumTypes::InfoType ecp_lecture_##_name (void * component, int dependencyType, \
							 CalTimeType< _type _qual >::TimeType * ti, \
							 CalTimeType< _type _qual >::TimeType * tf, long * i, \
							 const char * const nomvar, size_t bufferLength, \
							 size_t * nRead, _type _qual ** data ) { \
    Superv_Component_i * _component = static_cast<Superv_Component_i *>(component); \
    double         _ti=*ti;						\
    double         _tf=*tf;						\
    size_t         _nRead=0;						\
    size_t         _bufferLength=bufferLength;				\
    CalciumTypes::DependencyType _dependencyType=			\
      static_cast<CalciumTypes::DependencyType>(dependencyType);	\
    									\
    if ( IsSameType< _name , cplx >::value ) _bufferLength*=2;		\
    DEBTRACE( "-------- CalciumInterface(lecture Inter Part) MARK 1 ------------------" ) \
    try {								\
      CalciumInterface::ecp_lecture< _type, _name >( *_component,	\
						     _dependencyType, \
						     _ti, _tf, *i,	\
						     nomvar,		\
						     _bufferLength, _nRead, *data); \
    } catch ( const CalciumException & ex) {				\
      DEBTRACE( "-------- CalciumInterface(lecture Inter Part) MARK 1b ------------------" ) \
      DEBTRACE( ex.what() )				\
      DEBTRACE( "-------- CalciumInterface(lecture Inter Part) MARK 1ter ------------------" ) \
      return ex.getInfo();						\
    }									\
    if ( IsSameType< _name , cplx >::value ) { *nRead=_nRead/2;		\
      DEBTRACE( "-------- CalciumInterface(lecture Inter Part) IsSameType cplx -------------" ) \
      DEBTRACE( "-------- CalciumInterface(lecture Inter Part) _nRead  : " << _nRead ) \
      DEBTRACE( "-------- CalciumInterface(lecture Inter Part) *nRead  : " << *nRead ) \
    } else *nRead = _nRead;						\
    if (_dependencyType == CalciumTypes::CP_SEQUENTIEL ) \
        *ti=(CalTimeType< _type _qual >::TimeType)(_ti);			\
    DEBTRACE( "-------- CalciumInterface(lecture Inter Part), Data Ptr :" << *data ) \
    /* \
    for (int i=0; i<_nRead;++i)						\
      printf("-------- CalciumInterface(lecture Inter Part), Valeur de data (typage entier) data[%d] : %d \n",i,(*data)[i]); \
      */ \
    DEBTRACE( "-------- CalciumInterface(lecture Inter Part), Data Ptr :" << *data ) \
    return CalciumTypes::CPOK;						\
  };									\
  extern "C" void ecp_lecture_##_name##_free ( _type _qual * data) {	\
    CalciumInterface::ecp_free< _type, _name >(data);			\
  };		                                                        \
  extern "C" CalciumTypes::InfoType ecp_ecriture_##_name (void * component, int dependencyType, \
							  CalTimeType< _type _qual >::TimeType *t, \
							  long  i,	\
							  const char * const nomvar, size_t bufferLength, \
							  _type _qual * data ) { \
    Superv_Component_i * _component = static_cast<Superv_Component_i *>(component); \
    /* Je ne sais pas pourquoi, je n'arrive pas � passer t par valeur : corruption de la pile*/ \
    double         _t=*t;						\
    size_t         _bufferLength=bufferLength;				\
    if ( IsSameType< _name , cplx >::value ) _bufferLength=_bufferLength*2; \
    DEBTRACE( "-------- CalciumInterface(ecriture Inter Part) MARK 1 ------------------" ) \
    try {								\
      /*printf("-------- CalciumInterface(ecriture Inter Part), cp_name : Nom de la var. de type %s : %s\n",#_type,nomvar);*/ \
      std::string essai(nomvar);					\
      DEBTRACE( "----------->-" << nomvar )		\
      CalciumInterface::ecp_ecriture< _type, _name >( *_component,	\
						      static_cast<CalciumTypes::DependencyType>(dependencyType), \
						      _t,i,nomvar,_bufferLength,*data); \
    } catch ( const CalciumException & ex) {				\
      std::cerr << ex.what() << std::endl;				\
      return ex.getInfo();						\
    }									\
    DEBTRACE( "-------- CalciumInterface(ecriture Inter Part), Valeur de data : " ) \
    DEBTRACE( "-------- CalciumInterface(ecriture Inter Part), Ptr(1) :" << data ) \
    /* \
    for (int i=0; i<_bufferLength;++i)					\
      printf("-------- CalciumInterface(ecriture Inter Part), Valeur de data (typage entier) data[%d] : %d \n",i,data[i]); \
      */ \
    DEBTRACE( "-------- CalciumInterface(ecriture Inter Part), Ptr(2) :" << data ) \
    return CalciumTypes::CPOK;						\
  };									\



#define STAR *
CALCIUM_C2CPP_INTERFACE_(int,int,);
CALCIUM_C2CPP_INTERFACE_(float,float, );
CALCIUM_C2CPP_INTERFACE_(double,double,);
CALCIUM_C2CPP_INTERFACE_(bool,bool,);
CALCIUM_C2CPP_INTERFACE_(cplx,float,);

#endif
