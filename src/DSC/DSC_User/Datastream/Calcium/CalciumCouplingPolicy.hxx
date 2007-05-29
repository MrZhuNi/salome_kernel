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
//  File   : CalciumCouplingPolicy.hxx
//  Author : Eric Fayolle (EDF)
//  Module : KERNEL
// Id          : $Id$

#ifndef __CALCIUM_COUPLING_POLICY__ 
#define __CALCIUM_COUPLING_POLICY__

#include <vector>
#include <map>

#include "DisplayPair.hxx"
#include "CouplingPolicy.hxx"
#include "AdjacentFunctor.hxx"
#include <boost/lambda/lambda.hpp>
#include "CalciumTypes.hxx"
#include "CalciumException.hxx"

class CalciumCouplingPolicy : public CouplingPolicy  {


public:

  template <typename T_TIME, typename T_TAG >        class InternalDataIdContainer;
  template <typename T_TIME, typename T_TAG > friend class InternalDataIdContainer;
  template <typename DataManipulator >        friend class BoundedDataIdProcessor;
  template <typename DataManipulator >        friend class EraseDataIdProcessor;
  template <typename DataManipulator >        friend class DisconnectProcessor;

  typedef CalciumTypes::DependencyType       DependencyType;
  typedef CalciumTypes::DateCalSchem         DateCalSchem;
  typedef CalciumTypes::InterpolationSchem   InterpolationSchem;
  typedef CalciumTypes::ExtrapolationSchem   ExtrapolationSchem;
  typedef CalciumTypes::DisconnectDirective  DisconnectDirective;  

private:

  DependencyType      _dependencyType;
  size_t              _storageLevel;
  DateCalSchem        _dateCalSchem;
  InterpolationSchem  _interpolationSchem;
  ExtrapolationSchem  _extrapolationSchem;
  double              _alpha;
  double              _deltaT;
  DisconnectDirective _disconnectDirective;

public:
  CalciumCouplingPolicy();

  void           setDependencyType (DependencyType dependencyType);
  DependencyType getDependencyType () const;
 
  void   setStorageLevel   (size_t storageLevel);
  size_t getStorageLevel   () const;

  void         setDateCalSchem   (DateCalSchem   dateCalSchem);
  DateCalSchem getDateCalSchem () const;

  void   setAlpha(double alpha);
  double getAlpha() const ;

  void   setDeltaT(double deltaT );
  double getDeltaT() const ;

  void setInterpolationSchem (InterpolationSchem interpolationSchem);
  void setExtrapolationSchem (ExtrapolationSchem extrapolationSchem);
  InterpolationSchem getInterpolationSchem () const ;
  ExtrapolationSchem getExtrapolationSchem () const ;

  // Classe DataId rassemblant les param�tres de la m�thode PORT::put 
  // qui identifient l'instance d'une donn�e pour Calcium
  // Rem : Le DataId doit pouvoir �tre une key dans une map stl
  typedef double TimeType;
  typedef long   TagType;
  typedef std::pair< TimeType , TagType >     DataId;
  typedef InternalDataIdContainer < TimeType , TagType >  DataIdContainer;
  typedef std::vector< DataId >::iterator  iterator;

  template <typename T_TIME, typename T_TAG >  
  struct InternalDataIdContainer;

  inline TimeType getTime(const DataId &dataId) const { return dataId.first;}
  inline TagType  getTag (const DataId &dataId) const { return dataId.second;}

  // TODO : V�rifier l'application pour tous les types de donn�es
  template <typename DataManipulator>  struct BoundedDataIdProcessor;
  template <typename DataManipulator>  struct EraseDataIdProcessor;
  template <typename DataManipulator>  struct DisconnectProcessor;

  // Renvoie isEqual si le dataId attendu est trouv� dans storedDataIds :
  //   - l'it�rateur wDataIt1 pointe alors sur ce dataId
  // Renvoie isBounded si le dataId attendu n'est pas trouv� mais encadrable et 
  // que la politique de couplage g�re ce cas de figure 
  //   - l'it�rateur wDataIt1 est tel que wDataIt1->first < wdataId < (wDataIt1+1)->first
  // Le container doit �tre associatif
  template < typename AssocContainer >
  bool isDataIdConveniant( AssocContainer & storedDatas, 
			   const typename AssocContainer::key_type & expectedDataId,
			   bool & isEqual, bool & isBounded, 
			   typename AssocContainer::iterator & wDataIt1) const;

  TimeType getEffectiveTime(TimeType ti, TimeType tf);

  void disconnect(bool provideLastGivenValue);

}; //Fin de CalciumCouplingPolicy



//*************   DEFINITION DES METHODES ET OBJETS TEMPLATES *************// 



// D�finition du container de DataId pour r�pondre au concept
// de mode de couplage
template <typename T_TIME, typename T_TAG > 
struct CalciumCouplingPolicy::InternalDataIdContainer : public std::vector< std::pair< T_TIME,T_TAG> >  {
  typedef std::vector < DataId >        DataIdVect;
    
  InternalDataIdContainer(const DataId & dataId, 
			  const CalciumCouplingPolicy & policy
			  ):std::vector< std::pair< T_TIME,T_TAG> >() {
    // Ignore les param�tres qui ne sont pas en rapport avec le type de d�pendance
    switch (policy._dependencyType) {
    case CalciumTypes::TIME_DEPENDENCY:
      this->push_back(DataId(dataId.first,0));
      break;
    case CalciumTypes::ITERATION_DEPENDENCY:
      this->push_back(DataId(0,dataId.second));
      break;
    default:
      throw(CalciumException(CalciumTypes::CPIT,LOC("The dependency type must be set by setDependencyType before calling DataIdContainer contructor")));
      break;
    }
  };
};


// TODO : V�rifier l'application pour tous les types de donn�es
// DESACTIVER POUR ?BOOL? et CHAR *
template <typename DataManipulator>
struct CalciumCouplingPolicy::BoundedDataIdProcessor{
    
  const CalciumCouplingPolicy & _couplingPolicy;
    
  BoundedDataIdProcessor(const CalciumCouplingPolicy &couplingPolicy):
    _couplingPolicy(couplingPolicy) {};
    
  // M�thode impl�mentant l'interpolation temporelle
  template < typename MapIterator > 
  void inline apply (typename iterator_t<MapIterator>::value_type & data,
		     const DataId & dataId, const MapIterator & it1) const {
      
    typedef typename iterator_t<MapIterator>::value_type value_type;
    typedef typename DataManipulator::InnerType InnerType;
    typedef typename DataManipulator::Type Type;

    MapIterator it2=it1; ++it2;
    size_t   dataSize1 = DataManipulator::size(it1->second);
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Taille de donn�e dataId1 : " << dataSize1 << std::endl;
 
    // G�rer dans calcium la limite de la taille du buffer donn�e par
    // l'utilisateur.
    size_t   dataSize2 = DataManipulator::size(it2->second);
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Taille de donn�e dataId2 : " << dataSize2 << std::endl;

    size_t   dataSize  = std::min< size_t >( dataSize1, dataSize2 );
    DataId   dataId2 = it2->first;
    DataId   dataId1 = it1->first;
    TimeType t2      = dataId2.first;
    TimeType t1      = dataId1.first;
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Valeur de t1 : " << t1 << std::endl;
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Valeur de t2 : " << t2 << std::endl;
    TimeType t       = dataId.first;
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Valeur de t : " << t << std::endl;
    TimeType timeDiff  = t2-t1;
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Valeur de timeDiff : " << timeDiff << std::endl;
    TimeType coeff   = (t2-t)/timeDiff;
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Valeur de coeff : " << coeff << std::endl;

    InnerType const * const InIt1 = DataManipulator::getPointer(it1->second);
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Donn�es � t1 : " << std::endl;
    std::copy(InIt1,InIt1+dataSize1,std::ostream_iterator<InnerType>(std::cout," "));
    std::cout << std::endl;
    InnerType const * const InIt2 = DataManipulator::getPointer(it2->second);
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Donn�es � t2 : " << std::endl;
    std::copy(InIt2,InIt2+dataSize2,std::ostream_iterator<InnerType>(std::cout," "));
    std::cout << std::endl;
    Type              dataOut = DataManipulator::create(dataSize);
    InnerType * const OutIt   = DataManipulator::getPointer(dataOut);
 
    if ( timeDiff == 0.0 ||  _couplingPolicy._interpolationSchem == CalciumTypes::L0_SCHEM ) {
      std::copy(InIt1,InIt1+dataSize,OutIt);
    } else {

      boost::lambda::placeholder1_type _1;
      boost::lambda::placeholder2_type _2;
      // REM : Pour des buffers de type int
      // le compilo indiquera warning: converting to `long int' from `Double'
      std::transform(InIt1,InIt1+dataSize,InIt2,OutIt,
       		     ( _1 - _2 ) * coeff + _2 );
//       for(size_t i =0;  i < dataSize3; ++i) {
// 	OutIt[i]=(InIt1[i] - InIt2[i]) * coeff + InIt2[i];
//       }

    }
    std::cout << "-------- CalciumCouplingPolicy::BoundedDataIdProcessor : Donn�es calcul�es � t : " << std::endl;
    std::copy(OutIt,OutIt+dataSize,std::ostream_iterator<InnerType>(std::cout," "));
    std::cout << std::endl;
    data = dataOut;
    
  }
};

// Renvoie isEqual si le dataId attendu est trouv� dans storedDataIds :
//   - l'it�rateur wDataIt1 pointe alors sur ce dataId
// Renvoie isBounded si le dataId attendu n'est pas trouv� mais encadrable et 
// que la politique de couplage g�re ce cas de figure 
//   - l'it�rateur wDataIt1 est tel que wDataIt1->first < wdataId < (wDataIt1+1)->first
// Le container doit �tre associatif
template < typename AssocContainer >
bool CalciumCouplingPolicy::isDataIdConveniant( AssocContainer & storedDatas, const typename AssocContainer::key_type & expectedDataId,
						bool & isEqual, bool & isBounded, typename AssocContainer::iterator & wDataIt1) const {
 
  // Rem : le type key_type == DataId
  typedef typename AssocContainer::key_type key_type;
  AdjacentFunctor< key_type > af(expectedDataId);
  if ( _dependencyType == CalciumTypes::TIME_DEPENDENCY )
  {
    std::cout << "-------- time expected : " << expectedDataId.first << std::endl;
    std::cout << "-------- time expected corrected : " << expectedDataId.first*(1.0-_deltaT) << std::endl;
    af.setMaxValue(key_type(expectedDataId.first*(1.0-_deltaT),0));
  }
  isBounded = false;

  // Rem 1 :
  // L'algo adjacent_find ne peut �tre utilis� avec l'AdjacentPredicate 
  //   - si la table contient un seul �l�ment l'algorithme adjacent_find retourne end()
  //     que se soit l'�l�ment attendu ou non
  //   - si la table contient deux �l�ments dont le dernier est celui recherch�
  //     l'algorithme adjacent_find retourne end() aussi
  //   d'ou la necessit� d'effectuer  un find avant ou d'�crire un algorithme ad hoc
 
  // Rem 2 :
  //
  // L'algo find_if ne peut �tre utilis� car il recopie l'AdjacentFunctor
  // qui ne peut alors pas m�moriser ses �tats pr�c�dents
  //	
 
  // Un codage en reverse serait plus efficace
  typename AssocContainer::iterator prev    = storedDatas.begin();
  typename AssocContainer::iterator current = prev;
  while ( (current != storedDatas.end()) && !af(current->first)  ) 
  {
    std::cout << "------- stored time : " << current->first << std::endl;
    //  if ( af(current->first) ) break;
    prev = current++;
  }

  isEqual = af.isEqual();
    
  // On consid�re qu'il n'est pas possible d'encadrer en d�pendance it�rative,
  // on se veut pas calculer d'interpolation. 
  if  ( _dependencyType == CalciumTypes::TIME_DEPENDENCY)  isBounded = af.isBounded();

  if ( isEqual ) wDataIt1 = current;
  else 
    if (isBounded) wDataIt1 = prev;
    else
      wDataIt1 = storedDatas.end();

  std::cout << "-------- isDataIdConvenient : isEqual : " << isEqual << " , isBounded " << isBounded << std::endl;

  return isEqual || isBounded;
}

// TODO :PAS ENCORE TESTE AVEC UN NIVEAU POSITIONNE
// Supprime les DataId et les donn�es associ�es
// du container associatif quand le nombre
// de donn�es stock�es d�passe le niveau CALCIUM.
// Cette m�thode est appel�e de GenericPort::get et GenericPort::next 
// TODO : Elle devrait �galement �tre appel�e dans GenericPort::Put
// mais il faut �tudier les interactions avec GenericPort::Get et GenericPort::next
template < typename DataManipulator > 
struct CalciumCouplingPolicy::EraseDataIdProcessor {

  CalciumCouplingPolicy &_couplingPolicy;
    
  EraseDataIdProcessor(CalciumCouplingPolicy &couplingPolicy):
    _couplingPolicy(couplingPolicy) {};

  template < typename Container >
  void apply(Container & storedDatas, 
	     typename Container::iterator & wDataIt1 ) const {

    typedef typename Container::key_type   key_type;
    typedef typename Container::value_type value_type;
    typedef typename Container::iterator iterator;

    std::cout << "-------- CalciumCouplingPolicy::eraseDataId, storedDatasSize : " << storedDatas.size() << std::endl;
 
    if ( _couplingPolicy._storageLevel == CalciumTypes::UNLIMITED_STORAGE_LEVEL ) return;
 
    size_t storedDatasSize = storedDatas.size();
    long   s = storedDatasSize - _couplingPolicy._storageLevel;
    if (s > 0 ) {
      size_t dist=distance(storedDatas.begin(),wDataIt1);
      for (int i=0; i<s; ++i) {
	DataManipulator::delete_data((*storedDatas.begin()).second);
	storedDatas.erase(storedDatas.begin());
      }
      // Si l'it�rateur pointait sur une valeur que l'on vient de supprimer
      if (dist < s ) {
	throw(CalciumException(CalciumTypes::CPNTNULL,LOC(OSS()<< "La gestion du niveau CALCIUM " 
					    << _couplingPolicy._storageLevel << 
					    " vient d'entra�ner la suppression de la donn�e � renvoyer")));
      }
    }
    std::cout << "-------- CalciumCouplingPolicy::eraseDataId, new storedDatasSize : " << storedDatas.size() << std::endl;
    return;

  }
};


// Lorsque cette m�thode est appel�e depuis GenericPort::Get 
// l'expectedDataId n'a pas �t� trouv� et n'est pas non plus 
// encadr� (en mode temporel).
// Si apply n'effectue pas de traitement particulier la m�thode renvoie false
// Si le port a d�j� re�u une directive de deconnexion STOP une exception est lev�e
// Si le port a d�j� re�u une directive de deconnexion CONTINUE, 
// on donne la derni�re valeur connu et on renvoie true.
template < typename DataManipulator > 
struct CalciumCouplingPolicy::DisconnectProcessor {

  const CalciumCouplingPolicy  & _couplingPolicy;
    
  DisconnectProcessor(const CalciumCouplingPolicy & couplingPolicy):
    _couplingPolicy(couplingPolicy) {};

  template < typename Container, typename DataId >
  bool apply(Container & storedDatas,
	     const DataId & expectedDataId,
	     typename Container::iterator & wDataIt1 ) const {

    typedef typename Container::key_type   key_type;
    typedef typename Container::value_type value_type;
    typedef typename Container::iterator   iterator;

    // Pas de traitement particulier a effectuer
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor MARK1 ("<< _couplingPolicy._disconnectDirective<<") --------" << std::endl;
    if ( (_couplingPolicy._disconnectDirective) == (CalciumTypes::UNDEFINED_DIRECTIVE) ) return false;
  
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor MARK2 --------" << std::endl;

    // TODO : Ds GenericPort::next il faut convertir en CPSTOPSEQ
    if ( _couplingPolicy._disconnectDirective == CalciumTypes::CP_ARRET )
      throw(CalciumException(CalciumTypes::CPINARRET,LOC(OSS()<< "La directive CP_ARRET" 
					   << " provoque l'interruption de toute lecture de donn�es")));
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor MARK3 --------" << std::endl;


    // S'il n'y a plus de donn�es indique que l'on a pas pu effectuer de traitement
    // TODO : Dans la gestion des niveaux il faut peut �tre interdire un niveau ==  0
    if ( storedDatas.empty() ) 
      throw(CalciumException(CalciumTypes::CPNTNULL,LOC(OSS()<< "La directive CP_CONT" 
					  << " est active mais aucune donn�e n'est disponible.")));
    
    // expectedDataId n'a ni �t� trouv� dans storedDataIds ni encadr� mais il se peut
    // qu'en mode it�ratif il ne soit pas plus grand que le plus grand DataId stock� auquel
    // cas on doit renvoyer une expection car on n'est plus connect� et on ne pourra jamais
    // fournir de donn�es pour ce dataId.
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor MARK4  " << expectedDataId <<" --------" << std::endl;

    // >= expectedDataId
    iterator it1 = storedDatas.lower_bound(expectedDataId);
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor MARK5  " << std::endl;
    for (iterator it=storedDatas.begin();it!=storedDatas.end();++it)
      std::cout <<" "<<(*it).first ;
    std::cout <<std::endl;

    // TODO : Il faut en fait renvoyer le plus proche cf IT ou DT
    if (it1 == storedDatas.end())
      throw(CalciumException(CalciumTypes::CPNTNULL,LOC(OSS()<< "La directive CP_CONT" 
					  << " est active mais le dataId demand� est inf�rieur ou �gal au dernier re�u.")));
  
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor MARK6 " << std::endl;

    wDataIt1 = storedDatas.end();
    --wDataIt1;
    std::cout << "-------- CalciumCouplingPolicy::DisconnectProcessor, CP_CONT : " << (*wDataIt1).first << std::endl;

    return true;
  }
};

#endif
