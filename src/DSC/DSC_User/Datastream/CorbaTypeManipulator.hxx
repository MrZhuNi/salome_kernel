// Eric Fayolle - EDF R&D
// Modified by : $LastChangedBy$
// Date        : $LastChangedDate: 2007-02-07 18:26:44 +0100 (mer, 07 fév 2007) $
// Id          : $Id$

#ifndef _TYPE_MANIPULATION_HXX_
#define _TYPE_MANIPULATION_HXX_

#include <iostream>
#include <CORBA.h>

using namespace std;

// Classes manipulation
// -------------------
//
// Ces diff�rentes classes permettent d'unifier la manipulation des
// diff�rents types de donn�es dans un port datastream
// Les donn�es sont mani�es par valeur ou par pointeur 
// pour �viter les recopies de gros volume de donn�es 

// Les classes pr�sentes quatre m�thodes :
// - clone
// - get_data
// - delete_data
// - dump
// et
// deux type :
// - Type   : Le type CORBA de la donn�e manipul�e
// - InType : Le mapping CORBA pour un param�tre IN du type manipul�
 

// Cette classe permet de manipuler des types CORBA 
// any, struct, union et sequence (utiliser plut�t les seq_manipulator)
// Ces types sont manipul�s par pointeur.
// Les donn�es re�ues de CORBA sont syst�matiquement
// dupliqu�es pour �tre conserv�es.
// Quelque soit le type de donn�e, les donn�es sont consid�r�es 
// comme une donn�e unique (retour de size() == 1)
template <typename T >
class user_type_manipulation
{
public:
  typedef T *       Type;
  // correspond au mapping corba des type any, struct, 
  //                  union, s�quence en param�tre IN
  typedef const T & CorbaInType; 
  typedef T         InnerType;

  // Operation de recuperation des donnees venant de l'ORB et
  //  creation d'une copie (memoire sp�cialement allouee)
  static inline Type get_data(CorbaInType data) {
    return new T(data);
  }

  // Pb si ownerShip == True car appel par l'utilisateur de relPointer !
  static inline InnerType * const getPointer(Type data, bool ownerShip = false) {
    return data;
  }

  static inline void relPointer(InnerType * dataPtr) {
    delete dataPtr;
  }

  // Operation de clonage : par defaut, creation d'une copie en memoire allouee pour l'occasion
  static inline Type clone(Type data) { 
    return new T (* data);
  } 
  static inline Type clone(CorbaInType data) {
    return new T (data);
  }

  // Operation de cr�ation
  static inline Type create (size_t size=1) { 
    return new T();
  } 

  // Operation de destruction d'une donnee
  static inline void delete_data(Type data) {
    delete data;
  }
  
  // Renvoie la taille de la donn�e
  static inline size_t size(Type data) { 
    return 1;
  } 

  // Dump de l'objet pour deboguage: neant car on ne connait pas sa structure
  static inline void dump (CorbaInType data) {}
};


// G�re les types CORBA atomiques ('Int', 'Char', 'Float', ...)
// G�re les types enums
// G�re les r�f�rences d'objets CORBA
// Ces types sont manipul�s par valeur
// Les m�thodes getPointer ... ne devrait pas �tre utilis�e
// pour ce types de donn�es
template <typename T>
class atom_manipulation
{
public:
  typedef T Type;
  // correspond au mapping corba des types simples en param�tre IN
  typedef T CorbaInType; 
  typedef T InnerType; 

    
  // Operation de recuperation des donnees venant de l'ORB : une copie par affectation simple
  static inline Type get_data(CorbaInType data) {
    return data;
  }

 static inline InnerType * const getPointer(Type & data, bool getOwnerShip = false) {
//    InnerType * ptr;
//     if (getOwnerShip) {
//       ptr =new InnerType[1];*ptr=data;
//       return ptr;
//     } else
//      return &data;
   return &data;
 }

//   static inline void relPointer(InnerType * dataPtr) {
//     return;
//         delete[] dataPtr;
//   }

// Je ne sais pas comment l'impl�menter sans faire
// d'allocation heap
//static inline InnerType * allocPointer(size_t size=1) {
//    return  new InnerType[1];
  //}

  // Operation de clonage : une copie par affectation simple
  static inline Type clone(Type data) {
    return data;
  }

  // Inutile car Type == CorbaInType
  //   static inline Type clone(CorbaInType data) {
  //     return data;
  //   }

  // Operation de cr�ation
//   static inline Type create(size_t size=1,InnerType * data=NULL,
// 			    bool giveOwnerShip=false) {
//     Type dummy;
//     if (dataPtr)
//       return *data;
//     else
//       return dummy;
//   } 
    
  // Operation de destruction d'une donnee: rien a faire car pas de memoire a liberer
  static inline void delete_data(Type data) {}
    // Renvoie la taille de la donn�e

  static inline size_t size(Type data) { 
    return 1;
  } 

  // Dump de l'objet pour deboguage : Affiche la donnee
  static void inline dump (CorbaInType data) {
    cerr << "[atom_manipulation] Data : " << data << endl;
  }
};


// G�re un type sequence de taille illimitee (Unbounded)
// Ces types sont manipul�s par pointeur
template <typename seq_T,typename elem_T>
class seq_u_manipulation {
  
public:
  typedef seq_T * Type;
  // correspond au mapping corba de la s�quence en param�tre IN
  typedef const seq_T & CorbaInType; 
  typedef elem_T  InnerType;

 
  // Operation de recuperation des donnees venant de l'ORB
  // Remarque : On a un param�tre d'entr�e de type const seq_T &
  //            et en sortie un seq_T *
  static inline Type get_data(CorbaInType data) {
    CORBA::Long len = data.length();
    CORBA::Long max = data.maximum();
    // R�cup�re et devient propri�taire des donn�es re�ues dans la s�quence. 
    // La s�quence sera d�sallou� (mais pas le buffer) au retour 
    // de la m�thode put (car mapping de type IN : const seq & )
    // ATTENTION TESTER p184 si le pointeur est null
    // ATTENTION TESTER Si le flag release si la sequence contient des chaines
    // ou des object refs
    std::cout << "----seq_u_manipulation::get_data(..)-- MARK 1 ------------------" << std::endl;
    InnerType * p_data = const_cast<seq_T &>(data).get_buffer(true); 
    std::cout << "----seq_u_manipulation::get_data(..)-- MARK 2 ------"<<  p_data <<"------------" << std::endl;

    // Cr�e une nouvelle sequence propri�taire des donn�es du buffer (pas de recopie)
    // Les donn�es seront automatiquement d�sallou�es par appel interne � la m�thode freebuf
    // lors de la destruction de l'objet par appel � delete_data.
    return  new seq_T (max, len, p_data, true);
  }

  static inline size_t size(Type data) { 
    return data->length();
  } 

  // Operation de destruction d'une donnee
  static inline void delete_data(Type data) {
    delete data;
  }

  // Operation de clonage : par defaut creation d'une copie en memoire allouee pour l'occasion
  // Utilisation du constructeur du type seq_T
  static inline Type clone(Type data) {
    return new seq_T (*data) ;
  }
  static inline Type clone(CorbaInType data) {
    return new seq_T (data);
  }

  // Permet de d�sallouer le buffer dont on d�tient le pointeur par appel
  // � la m�thode getPointer avec ownerShip=True si la s�quence contenante
  // � �t� d�truite.
  static inline InnerType * const getPointer(Type data, bool ownerShip = false) {
    InnerType * p_data;
    if (ownerShip) {
      p_data = data->get_buffer(true);
      delete_data(data);
    } else
      p_data = data->get_buffer(false);
    return p_data;
  }

  // Permet de d�sallouer le buffer dont on d�tient le pointeur par appel
  // � la m�thode getPointer avec ownerShip=True si la s�quence contenante
  // � �t� d�truite.
  static inline void relPointer(InnerType * dataPtr) {
    seq_T::freebuf(dataPtr);
  }

  // Permet d'allouer un buffer pour la s�quence
  static inline InnerType *  allocPointer(size_t size ) {
    return seq_T::allocbuf(size);
  }

  // Operation de cr�ation du type corba soit
  // - Vide et de taille size
  // - Utilisant les donn�es du pointeur *data de taille size 
  // (g�n�ralement pas de recopie qlq soit l'ownership )
  // data doit avoir �t� allou� par allocPointer si giveOwnerShip = true  
  static inline Type create(size_t size, InnerType * const data = NULL,
			    bool giveOwnerShip = false ) { 
    Type tmp;
    if (!data) {
      tmp = new seq_T();
      tmp->length(size);
    } else {
      tmp = new seq_T(size,size,data,giveOwnerShip); 
    }
    return tmp;
  } 
  
  // Dump de l'objet pour deboguage
  static void inline dump (CorbaInType data) {
    // Affiche la longueur des donnees
    cerr << "[seq_u_manipulation] Data length: " << data.length() << endl;
    // Affiche la longueur des donnees
    cerr << "[seq_u_manipulation] Data max: " << data.maximum() << endl;
  }
};


// G�re un type sequence de taille limit�e (bounded)
// Ces types sont manipul�s par pointeur
// Cette classe diff�re de la seq_u_manipulation
// par la signature du constructeur de la s�quence
// utilis� dans le methode get_data
template <typename seq_T,typename elem_T>
class seq_b_manipulation {
  
public:
  typedef seq_T *       Type;
  typedef const seq_T & CorbaInType;
  typedef elem_T        InnerType;


  // Operation de recuperation des donnees venant de l'ORB
  // Sans op�ration de notre part, ces donn�es seraient perdues
  // au retour de la m�thode put de GenericPort.
  // Remarque : On a un param�tre d'entr�e de type const seq_T &
  //            et en sortie un seq_T *
  static inline Type get_data(CorbaInType data) {
    CORBA::Long len = data.length();
    // R�cup�re et devient propri�taire des donn�es re�ues dans la s�quence 
    // la s�quence sera d�sallou� (mais pas le buffer)
    // au retour de la m�thode put (car mapping de type IN : const seq & )
    InnerType * p_data = const_cast<seq_T &>(data).get_buffer(true);
    // Cr�e une nouvelle sequence propri�taire des donn�es du buffer (g�n�ralement pas de recopie)
    // Les donn�es seront automatiquement d�sallou�es par appel interne � la m�thode freebuf
    // lors de la destruction de l'objet par appel � delete_data.
    return new seq_T (len, p_data, true);
  }

  static inline size_t size(Type data) { 
    return data->length();
  } 

  // Operation de clonage : par defaut creation d'une copie en memoire allouee pour l'occasion
  // Utilisation du constructeur du type seq_T  
  static inline Type clone(Type data) {
    return new seq_T (* data);
  }
  static inline Type clone(CorbaInType data) {
    return new seq_T (data);
  }

  // Operation de destruction d'une donnee CORBA
  static inline void delete_data(Type data) {
    delete data;
  }

  // R�cup�re un pointeur sur les donn�es de type InnerType contenue dans la s�quence
  // si ownership=True, l'utilisateur devra appeler relPointer
  // si ownership=False, l'utilisateur devra appeler delete_data sur la s�quence contenante
  static inline InnerType * const getPointer(Type data, bool getOwnerShip = false) {
    InnerType * p_data;
    if (getOwnerShip) {
      p_data = data->get_buffer(true);
      delete_data(data);
    } else
      p_data = data->get_buffer(false);
    return p_data;
  }

  // Permet de d�sallouer le buffer dont on d�tient le pointeur par appel
  // � la m�thode getPointer avec ownerShip=True si la s�quence contenante
  // � �t� d�truite.
  static inline void relPointer(InnerType * dataPtr) {
    seq_T::freebuf(dataPtr);
  }

  // Permet d'allouer un buffer pour la s�quence
  static inline InnerType *  allocPointer(size_t size ) {
    return seq_T::allocbuf(size);
  }

  // Operation de cr�ation du type corba soit
  // - Vide et de taille size
  // - Utilisant les donn�es du pointeur *data de taille size 
  // (g�n�ralement pas de recopie qlq soit l'ownership )
  // data doit avoir �t� allou� par allocPointer si giveOwnerShip = true  
  static inline Type create(size_t size, InnerType * const data = NULL,
			    bool giveOwnerShip = false ) { 
    Type tmp;
    if (!data) {
      tmp = new seq_T();
      tmp->length(size);
    } else {
      tmp = new seq_T(size,data,giveOwnerShip); 
    }
    return tmp;
  } 

  
  // Dump de l'objet pour deboguage
  static inline void dump (CorbaInType data) {
    // Affiche la longueur des donnees
    cerr << "[seq_b_manipulation] Data length: " << data.length() << endl;
  }
};

#endif
