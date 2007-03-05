// Eric Fayolle - EDF R&D
// Modified by : $LastChangedBy$
// Date        : $LastChangedDate: 2007-02-13 11:09:09 +0100 (mar, 13 fév 2007) $
// Id          : $Id$

#ifndef _COPY_TO_USER_SPACE_HXX_
#define _COPY_TO_USER_SPACE_HXX_

#include <string>
#include <iostream>
#include "CalciumPortTraits.hxx"

//Cas du zero copie
template <bool zerocopy >
struct Copy2UserSpace{
  
  template <class T1, class T2>
  static void apply( T1 * & data, T2 & corbaData, size_t nRead ){

    typedef typename ProvidesPortTraits<T1>::PortType PortType;
    typedef typename PortType::DataManipulator        DataManipulator;
    typedef typename DataManipulator::InnerType       InnerType;

    // Devient propri�taire des donn�es contenues dans la structure CORBA
    // (allou�es par allocbuff() pour une s�quence)
    InnerType * dataPtr  = DataManipulator::getPointer(corbaData,true);

    // Cette ligne poserait uun probl�me dans la m�thode appelante, si elle
    // ne testait pas que les types utilisateurs et CORBA sont identiques :
    // ex :  InnerType == Corba::Long et d'un T == int
    data = dataPtr; 

    // L'utilisateur a la charge de la desallocation
    // il devra appeler la m�thode ecp_free pour d�sallouer le contenu de la s�quence CORBA
    // La structure CORBA sera d�sallouer le cas �ch�ant dans la m�thode appelante
  }
};

// Cas o� il faut effectuer une recopie
template <>
struct Copy2UserSpace<false>{

  //Recopie le contenu de la donn�e CORBA dans le buffer utilisateur de longueur
  //nRead
  template <class T1, class T2>
  static void apply( T1 * &data, T2 & corbaData, size_t nRead){

    typedef typename ProvidesPortTraits<T1>::PortType  PortType;
    typedef typename PortType::DataManipulator         DataManipulator;
    typedef typename DataManipulator::InnerType        InnerType;

    size_t corbaDataSize = DataManipulator::size      (corbaData);
    // R�cup�re un pointeur sur les donn�es contenues dans la structure CORBA sans en devenir propri�taire
    InnerType * dataPtr  = DataManipulator::getPointer(corbaData,false);

    // Attention : Pour les chaines ou tout autre object complexe il faut utiliser une recopie profonde !
    // Recopie des donn�es dans le buffer allou�e par l'utilisateur 
    // OU 
    // Recopie des donn�es dans le buffer allou�e la m�thode appelante
    // dans le cas d'une demande utilisateur 0 copie mais sur 
    // des types utilisateurs et CORBA incompatibles.
    std::cerr << "-------- Copy2UserSpace<false> MARK 1 --(dataPtr : " <<dataPtr<<")----------------" << std::endl;
    
    std::copy(dataPtr,dataPtr+nRead,data);
 
    std::cerr << "-------- Copy2UserSpace<false> MARK 2 --(nRead: "<<nRead<<")-------------" << std::endl;
 
    std::cerr << "-------- Copy2UserSpace<false> MARK 3  --(data : " <<data<<") :";
    std::copy(data,data+nRead,std::ostream_iterator<T1>(std::cout," "));
    std::cout << std::endl;

 }
};


template <bool rel>
struct DeleteTraits {
  template <typename T> 
  static void apply(T * dataPtr) {

    typedef typename ProvidesPortTraits<T>::PortType     PortType;
    typedef typename PortType::DataManipulator          DataManipulator;
    //typedef typename DataManipulator::Type         DataType; // Attention != T
    
    // Attention : Seul CalciumCouplingPolicy via eraseDataId doit d�cider de supprimer ou non
    // la donn�e corba associ�e � un DataId ! Ne pas effectuer la desallocation suivante :
    // DataManipulator::relPointer(dataPtr);
  }
};

// Cas o� une recopie avait �t� effectu�e
template <>
struct DeleteTraits<false>{
  template <typename T> 
  static void apply(T * dataPtr) {
    delete[] dataPtr;
  }
};

#endif
