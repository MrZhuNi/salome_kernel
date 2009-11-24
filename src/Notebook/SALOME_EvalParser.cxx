//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  File   : SALOME_EvalParser.cxx
//  Author : Peter KURNEV
//  Module : SALOME

#include <SALOME_EvalParser.hxx>

#include <stack>
#include <list>

bool          contains(const SALOME_StringList& aLS, const SALOME_String& aS);
SALOME_String trimmed(const SALOME_String& str);
SALOME_String trimmed(const SALOME_String& str, const char aWhat);
bool          isSpace(const char aC);
int           indexOf(const SALOME_StringList& aLS, const SALOME_String& aS);

//=======================================================================
//function :
//purpose  :
//=======================================================================
SALOME_EvalParser::SALOME_EvalParser()
: myAutoDel( false )
{
  setError( EvalExpr_OK );
}
//=======================================================================
//function : ~
//purpose  :
//=======================================================================
SALOME_EvalParser::~SALOME_EvalParser()
{
  if (autoDeleteOperationSets()) {
    //qDeleteAll( mySets );// !!!
    SALOME_ListOfEvalSet::const_iterator aIt = mySets.begin();
    for (; aIt != mySets.end() ; ++aIt )  {
      SALOME_EvalSet* pEvalSet=*aIt;
      if (pEvalSet) {
        delete pEvalSet;
        pEvalSet=NULL;
      }
    }
  }
}
//=======================================================================
//function : operationSets
//purpose  :
//=======================================================================
SALOME_ListOfEvalSet SALOME_EvalParser::operationSets() const
{
  return mySets;
}
//=======================================================================
//function : operationSet
//purpose  :
//=======================================================================
SALOME_EvalSet* SALOME_EvalParser::operationSet(const SALOME_String& theName) const
{
  SALOME_EvalSet* pSet;
  //
  pSet = NULL;
  SALOME_ListOfEvalSet::const_iterator it = mySets.begin();
  for (; it != mySets.end() && !pSet; ++it )  {
    if ( (*it)->name()==theName ) {
      pSet = *it;
    }
  }
  return pSet;
}
//=======================================================================
//function : insertOperationSet
//purpose  :
//=======================================================================
void SALOME_EvalParser::insertOperationSet(SALOME_EvalSet* theSet,
                                       const int idx)
{
  if (SALOME_EvalSet::contains(mySets, theSet)) {
    return;
  }
  //
  int iSize=(int)mySets.size();
  int index = idx < 0 ? iSize : idx;
  if (index>iSize) {
    index=iSize;
  }
  SALOME_EvalSet::insert(mySets, index, theSet);
}
//=======================================================================
//function : removeOperationSet
//purpose  :
//=======================================================================
void SALOME_EvalParser::removeOperationSet(SALOME_EvalSet* theSet)
{
  //mySets.removeAll( theSet );
  mySets.remove(theSet);
}
//=======================================================================
//function : autoDeleteOperationSets
//purpose  :
//=======================================================================
bool SALOME_EvalParser::autoDeleteOperationSets() const
{
  return myAutoDel;
}
//=======================================================================
//function : setAutoDeleteOperationSets
//purpose  :
//=======================================================================
void SALOME_EvalParser::setAutoDeleteOperationSets( const bool theOn )
{
  myAutoDel = theOn;
}
//=======================================================================
//function : search
//purpose  :
//=======================================================================
int SALOME_EvalParser::search(const SALOME_StringList& aList, const SALOME_String& aStr, int offset,  int& matchLen, int& listind )
{
  int min = -1;
  int ind = 0;
  int pos;
  //static const basic_string <char>::size_type npos = -1;
  //
  SALOME_StringList::const_iterator aIt = aList.begin(), aLast = aList.end();
  for (ind = 0; aIt != aLast; aIt++, ind++ )
  {
    const SALOME_String& aStrX=*aIt;

    pos=(int)aStr.find(aStrX, offset);
    if ( pos >= 0 &&
         ( min < 0 || min > pos ||
          ( min == pos && matchLen < (int)aStrX.length())
         )
       )  {
      min = pos;
      listind = ind;
      matchLen = (int)aStrX.length();
    }

  }
  if ( min < 0 ){
    matchLen = 0;
  }
  return min;
}
//=======================================================================
//function : note
//purpose  :
//=======================================================================
SALOME_String SALOME_EvalParser::note(const SALOME_String& aStr, int pos, int len )
{
  SALOME_String aStr1;
  //
  aStr1=aStr.substr(pos, len);
  aStr1=trimmed(aStr1);

  return aStr1;
}

//=======================================================================
//function : prepare
//purpose  :
//=======================================================================
bool SALOME_EvalParser::prepare(const SALOME_String& expr, Postfix& post)
{
  int pos = 0;
  int len =(int)expr.length();
  stack<int> aBracketStack;
  SALOME_StringList anOpers, anOpenBr, aCloseBr;
  if ( !checkOperations() )
    return false;

  bracketsList( anOpenBr, true );
  bracketsList( aCloseBr, false );
  operationList( anOpers );

  while ( pos < len && error() == EvalExpr_OK )  {
    PostfixItem item;
    char aC=expr[pos];
    //
    while ( pos < len && isSpace(aC) ) {
      pos++;
    }
    //
    if ( pos >= len ) {
      break;
    }
    //
    int mBrLen = 0, mLen = 0, br_ind = -1, op_ind = -1;
    int oPos = search( anOpenBr, expr, pos, mBrLen, br_ind );
    int cPos = oPos == pos ? -1 : search( aCloseBr, expr, pos, mBrLen, br_ind );
    int opPos = search( anOpers, expr, pos, mLen, op_ind );
    //
    if ( aC=='\'')  {
      int vpos = pos + 1;
      while ( vpos < (int)expr.length() && expr[vpos] != '\'' ) {
        vpos++;
      }
      //
      mLen = vpos - pos + 1;
      //
      int res = createValue( note( expr, pos, mLen ), item.myValue );
      item.myType = res ? Value : Param;
      post.push_back( item );
      pos = vpos + 1;
      continue;
    }
    //
    if ( oPos == pos )  {
      aBracketStack.push( br_ind );
      item.myValue = note( expr, pos, mBrLen );
      item.myType = Open;
      post.push_back( item );
    }
    else if ( cPos == pos )  {
      if ( aBracketStack.size() == 0 )  {
        setError( EvalExpr_ExcessClose );
        break;
      }
      if ( br_ind != aBracketStack.top() )  {
        setError( EvalExpr_BracketsNotMatch );
        break;
      }
      else {
        aBracketStack.pop();
        item.myValue = note( expr, pos, mBrLen );
        item.myType = Close;
        post.push_back( item );
      }
    }
    else {
      mBrLen = 0;
    }
    //
    if ( opPos == pos ) {
      mBrLen = 0;
      item.myValue = note( expr, pos, mLen );
      item.myType = Binary;

      if ( oPos == pos ) {
        insert(post, (int)(post.size()-1), item);
      }
      else
        post.push_back( item );
    }
    else  {
      mLen = 0;
      if ( oPos != pos && cPos != pos )   {
	      int i;
        for ( i = pos + 1; i < (int)expr.length(); i++ )  {
          if ( isSpace(expr[i]) ) {
            break;
          }
        }
        //
        int vpos = i;
        if ( oPos >= 0 && oPos < vpos ) {
          vpos = oPos;
        }
        if ( cPos >= 0 && cPos < vpos ){
          vpos = cPos;
        }
        if ( opPos >= 0 && opPos < vpos ){
          vpos = opPos;
        }

        while( vpos < (int)expr.length() &&
               ( isalpha(expr[vpos]) ||
                 isdigit(expr[vpos]) ||
                 expr[vpos]=='_' )
                ) {
          vpos++;
        }
        //
        mLen = vpos - pos;
        bool res = createValue( note( expr, pos, mLen ), item.myValue );
        item.myType = res ? Value : Param;
        post.push_back( item );
      }
    }
    pos += mBrLen + mLen;
  }

  //Bracket checking
  int brValue = 0;
  for ( Postfix::iterator anIt = post.begin(); anIt != post.end(); ++anIt ) {
    if ( (*anIt).myType == Open ){
      brValue++;
    }
    else if ( (*anIt).myType == Close ) {
      if ( brValue > 0 ){
        brValue--;
      }
      else {
        setError( EvalExpr_ExcessClose );
        break;
      }
    }
  }
  //
  if ( brValue > 0 )
  {
    setError( EvalExpr_CloseExpected );
  }
  //
  return error() == EvalExpr_OK;
}
//=======================================================================
//function : setOperationTypes
//purpose  :
//=======================================================================
bool SALOME_EvalParser::setOperationTypes( Postfix& post )
{
  if ( !checkOperations() )
    return false;

  SALOME_StringList anOpen, aClose;
  //
  bracketsList( anOpen, true );
  bracketsList( aClose, false );

  Postfix::iterator aPrev, aNext;
  Postfix::iterator anIt = post.begin();
  for (; anIt != post.end(); ++anIt )  {
    aPrev = anIt;
    aPrev--;
    aNext = anIt;
    aNext++;
    if ( (*anIt).myType != Binary ){
      continue;
    }
    //
    if ( ( anIt == post.begin() || (*aPrev).myType == Open ||
           (*aPrev).myType == Pre || (*aPrev).myType == Binary ) &&  aNext != post.end() &&
         ( (*aNext).myType == Value || (*aNext).myType == Param ||
           (*aNext).myType == Open  || (*aNext).myType == Binary ) )
      (*anIt).myType = Pre;
    //
    else if ( anIt != post.begin() && ( (*aPrev).myType == Close || (*aPrev).myType == Param ||
                                        (*aPrev).myType == Value || (*aPrev).myType == Pre ||
                                        (*aPrev).myType == Post || (*aPrev).myType == Binary ) &&
              ( aNext == post.end() || (*aNext).myType == Close ) )
      (*anIt).myType = Post;
    //
    SALOME_EvalVariant& aRV=(*anIt).myValue;
    SALOME_String  aRVS=aRV.toString();
    //
    if ( contains(anOpen, aRVS) ) {
      (*anIt).myType = Pre;
    }
    else if ( contains(aClose, aRVS) ) {
      (*anIt).myType = Post;
    }
  }

  return error() == EvalExpr_OK;
}
//=======================================================================
//function : globalBrackets
//purpose  :
//=======================================================================
int SALOME_EvalParser::globalBrackets(const Postfix& post,
                                  int f,
                                  int l )
{
  int i;
  int start_br = 0;
  int fin_br = 0;
  int br = 0;
  int br_num = 0;
  int min_br_num = (l-f+1)*5;

  PostfixItem *pPost;
  pPost=new PostfixItem[post.size()];
  //
  Postfix::const_iterator aIt = post.begin();
  for (i=0; aIt != post.end(); ++aIt, ++i)  {
    const PostfixItem& aPostfixItem=*aIt;
    pPost[i]=aPostfixItem;
  }

  for( i=f; i<=l; i++ )
    if( pPost[ i ].myType==SALOME_EvalParser::Open )
      start_br++;
    else
      break;
  for( i=l; i>=f; i-- )
    if( pPost[ i ].myType==SALOME_EvalParser::Close )
      fin_br++;
    else
      break;

  br = start_br<fin_br ? start_br : fin_br;
  for( i=f+br; i<=l-br; i++ )
  {
    if( pPost[i].myType==SALOME_EvalParser::Open )
      br_num++;
    else if( pPost[i].myType==SALOME_EvalParser::Close )
      br_num--;
    if( br_num<min_br_num )
      min_br_num = br_num;
  }

  //delete pPost;
  delete [] (PostfixItem *)pPost;
  return br+min_br_num;
}
//=======================================================================
//function : sort
//purpose  :
//=======================================================================
bool SALOME_EvalParser::sort( const Postfix& post, Postfix& res, const SALOME_StringList& anOpen,
                              const SALOME_StringList& aClose, int f, int l )
{
  if ( l < f ){
    return true;
  }
  if ( f < 0 ) {
    f = 0;
  }
  if ( l < 0 ) {
    l = (int)(post.size() - 1);
  }

  int br = globalBrackets( post, f, l );
  f += br;
  l -= br;

  if ( f == l && f >= 0 ) {
    res.push_back(at(post,f));
  }
  if ( l <= f ) {
    return true;
  }

  if ( !checkOperations() ) {
    return false;
  }

  int min = -1;
  list<int> argmin;
  list<PostfixItemType> min_types;
  //
  for ( int i = 0, j = f; j <= l; i++, j++ )  {
    const PostfixItem& item = at(post,j);//!!!
    PostfixItemType tt = item.myType;
    if ( tt == Binary || tt == Pre || tt == Post )  {
      int cur_pr = priority( item.myValue.toString(), tt == Binary );
      if ( cur_pr > 0 ) {
        if ( min < 0 || min >= cur_pr ) {
          if ( min == cur_pr ) {
            argmin.push_back( f + i );
            min_types.push_back( tt );
          }
          else {
            min = cur_pr;
            argmin.clear();
            argmin.push_back( f + i );
            min_types.clear();
            min_types.push_back( tt );
          }
        }
      }
      else {
        setError( EvalExpr_InvalidOperation );
        break;
      }
    }
    else if ( tt == Open ){
      SALOME_String opBr = item.myValue.toString();
      int ind, brValue = 0;
      ind = indexOf(anOpen, opBr );
      while ( j <= l ) {
        const PostfixItem& anItem = at(post,j);
        if ( anItem.myType == Open )
          brValue++;

        if ( anItem.myType == Close ) {
          brValue--;
          SALOME_String clBr = anItem.myValue.toString();
          if ( indexOf(aClose, clBr ) == ind && brValue == 0 ){
            break;
          }
        }
        i++; j++;
      }
      //
      if ( brValue > 0 )      {
        setError( EvalExpr_CloseExpected );
        break;
      }
    }
  }
  //
  if ( error() == EvalExpr_OK ) {
    if ( min >= 0 )  {
      Postfix one;
      list<Postfix> parts;
      list<int>::const_iterator anIt = argmin.begin(), aLast = argmin.end();
      bool ok = sort( post, one, anOpen, aClose, f, *anIt - 1 );
      parts.push_back( one );
      one.clear();
      for ( ; anIt != aLast && ok; anIt++ )      {
        list<int>::const_iterator aNext = anIt; aNext++;
        ok = sort( post, one, anOpen, aClose, *anIt + 1, aNext == aLast ? l : *aNext - 1 );
        parts.push_back( one );
        one.clear();
      }
      //
      if ( !ok ){
        return false;
      }
      //
      stack<PostfixItem> aStack;
      list<Postfix>::const_iterator aPIt = parts.begin();
      list<PostfixItemType>::const_iterator aTIt = min_types.begin();
      //
      append(res,*aPIt);
      aPIt++;
      anIt = argmin.begin();
      for ( ; anIt != aLast; anIt++, aPIt++, aTIt++ )  {
        if ( *aTIt == Pre ) {
          if (!contains(anOpen, at(post,*anIt).myValue.toString())) {
            append(res,*aPIt);
            aStack.push( at(post, *anIt) );
          }
          else {
            res.push_back( at(post, *anIt) );
            append(res,*aPIt);
          }
        }
        else {
          append(res,*aPIt);
          while (!aStack.empty() ) {
            res.push_back( aStack.top() );
            aStack.pop();
          }
          res.push_back( at(post, *anIt) );
        }
      }
      while ( !aStack.empty() ) {
        res.push_back( aStack.top() );
        aStack.pop();
      }
    }
    else { //there are no operations
      for ( int k = f; k <= l; k++ ) {
        const PostfixItem& aItem=at(post, k);
        if ( aItem.myType==Value || aItem.myType == Param ){
          res.push_back( aItem );
        }
      }
    }
  }
  return error() == EvalExpr_OK;
}
//=======================================================================
//function : parse
//purpose  :
//=======================================================================
bool SALOME_EvalParser::parse( const SALOME_String& expr )
{
  myPostfix.clear();

  if ( !checkOperations() )
    return false;

  Postfix p;
  SALOME_StringList opens, closes;

  setError( EvalExpr_OK );
  bracketsList( opens, true );
  bracketsList( closes, false );

  return prepare( expr, p ) && setOperationTypes( p ) && sort( p, myPostfix, opens, closes );
}
//=======================================================================
//function : calculate
//purpose  :
//=======================================================================
bool SALOME_EvalParser::calculate(const SALOME_String& op, SALOME_EvalVariant& v1, SALOME_EvalVariant& v2 )
{
  SALOME_EvalExprError aErr;
  SALOME_EvalVariantType aType1, aType2;
  //
  aType1=v1.type();
  aType2=v2.type();
  //
  aErr = isValid( op, aType1, aType2 );
  if ( aErr == EvalExpr_OK ){
    aErr=calculation( op, v1, v2 );
    setError( aErr );
  }
  else{
    setError( aErr );
  }
  return error() == EvalExpr_OK;
}
//=======================================================================
//function : calculate
//purpose  :
//=======================================================================
SALOME_EvalVariant SALOME_EvalParser::calculate()
{
  if ( !checkOperations() )
    return SALOME_EvalVariant();

  setError( EvalExpr_OK );
  //
  SALOME_StringList anOpen, aClose;
  PostfixItemType aType;
  //
  bracketsList( anOpen, true );
  bracketsList( aClose, false );

  stack<SALOME_EvalVariant> aStack;
  Postfix::iterator anIt = myPostfix.begin(), aLast = myPostfix.end();
  for ( ; anIt != aLast && error() == EvalExpr_OK; anIt++ )  {
    const PostfixItem& aPostfixItem=*anIt;
    aType=aPostfixItem.myType;
    SALOME_String nn = aPostfixItem.myValue.toString();
    //
    if ( aType == Param ) {
      if ( hasParameter( nn ) )  {
        SALOME_EvalVariant& v = myParams[nn];
        if ( v.isValid() ) {
          aStack.push( v );
        }
        else {
          setError( EvalExpr_InvalidToken );
        }
      }
      else {
        setError( EvalExpr_InvalidToken );
      }
    }
    //
    else if ( aType == Value ) {
      aStack.push( (*anIt).myValue );
    }
    //
    else if ( aType == Pre || aType == Post )  {
      if ( contains(anOpen, nn ) )  {
        SALOME_EvalVariant inv;
        if ( calculate( nn, inv, inv ) ) {
          aStack.push( SALOME_EvalVariant() );
        }
      }
      else if ( contains(aClose, nn ) )  {
        SALOME_ListOfEvalVariant aSet;
        while ( true ) {
          if ( aStack.empty() ) {
            setError( EvalExpr_StackUnderflow );
            break;
          }
          if ( aStack.top().isValid() ) {
            aSet.push_back( aStack.top() );
            aStack.pop();
          }
          else {
            aStack.pop();
            break;
          }
        }

        SALOME_EvalVariant qSet = aSet, inv;
        if ( calculate( nn, qSet, inv ) ) {
          aStack.push( aSet );
        }
      }
      else if ( aStack.size() >= 1 )  {
        SALOME_EvalVariant inv;
        SALOME_EvalVariant *v1, *v2;
        v1=&aStack.top();
        v2 = &inv; //"post-" case
        if ( aType == Pre ) {
          v2 = &aStack.top();
          v1 = &inv;
        }
        calculate( nn, *v1, *v2 );
      }
      else {
        setError( EvalExpr_StackUnderflow );
      }
    }//else if ( aType== Pre || aType == Post )  {
    else if ( aType == Binary )  {
      if ( aStack.size() >= 2 )  {
        SALOME_EvalVariant v2= aStack.top();
        aStack.pop();
        calculate( nn, aStack.top(), v2 );
      }
      else {
        setError( EvalExpr_StackUnderflow );
      }
    }
  }
  //
  SALOME_EvalVariant res;
  if ( error() == EvalExpr_OK )  {
    int count;

    count = (int)aStack.size();
    if ( count == 0 ) {
      setError( EvalExpr_StackUnderflow );
    }
    else if( count == 1 ) {
      res = aStack.top();
    }
    else{
      setError( EvalExpr_ExcessData );
    }
  }
  return res;
}
//=======================================================================
//function : calculate
//purpose  :
//=======================================================================
SALOME_EvalVariant SALOME_EvalParser::calculate( const SALOME_String& expr )
{
  setExpression( expr );
  return calculate();
}
//=======================================================================
//function : setExpression
//purpose  :
//=======================================================================
bool SALOME_EvalParser::setExpression( const SALOME_String& expr )
{
  return parse( expr );
}
//=======================================================================
//function : hasParameter
//purpose  :
//=======================================================================
bool SALOME_EvalParser::hasParameter( const SALOME_String& name ) const
{
  bool bRet;
  SALOME_String aStr;
  ParamMap::const_iterator aIt;
  aStr=trimmed(name);
  aIt=myParams.find(aStr);
  bRet=(aIt!=myParams.end());
  return bRet;
}
//=======================================================================
//function : setParameter
//purpose  :
//=======================================================================
void SALOME_EvalParser::setParameter( const SALOME_String& name, const SALOME_EvalVariant& value )
{
  SALOME_String aStr;
  //
  aStr=trimmed(name);
  myParams[aStr] = value;
}
//=======================================================================
//function : removeParameter
//purpose  :
//=======================================================================
bool SALOME_EvalParser::removeParameter( const SALOME_String& name )
{
  int iRet;
  SALOME_String aStr;
  //
  aStr=trimmed(name);
  iRet=(int)myParams.erase(aStr);
  return iRet? true : false;

}
//=======================================================================
//function : parameter
//purpose  :
//=======================================================================
SALOME_EvalVariant SALOME_EvalParser::parameter( const SALOME_String& theName ) const
{
  SALOME_EvalVariant res;
  ParamMap::const_iterator aIt;
  SALOME_String aStr;
  aStr=trimmed(theName);
  aIt=myParams.find(theName);
  if (aIt!=myParams.end()) {
    const PairParamMap& aVT=*aIt;
    const SALOME_EvalVariant& aRV=aVT.second;
    res=aRV;
  }
  return res;
}
//=======================================================================
//function : firstInvalid
//purpose  :
//=======================================================================
bool SALOME_EvalParser::firstInvalid( SALOME_String& name ) const
{
  ParamMap::const_iterator aIt = myParams.begin();
  for (; aIt != myParams.end(); aIt++ )  {
    const PairParamMap& aVT=*aIt;
    const SALOME_EvalVariant& aRV=aVT.second;
    if ( !aRV.isValid() )  {
      name = aVT.first;
      return true;
    }
  }
  return false;
}
//=======================================================================
//function : removeInvalids
//purpose  :
//=======================================================================
void SALOME_EvalParser::removeInvalids()
{
  ParamMap aM;
  //
  ParamMap::const_iterator aIt = myParams.begin();
  for (; aIt != myParams.end(); aIt++ )  {
    const PairParamMap& aVT=*aIt;
    const SALOME_EvalVariant& aRV=aVT.second;
    if (aRV.isValid() )  {
      aM.insert(aVT);
    }
  }
  myParams.clear();
  myParams=aM;
}
//=======================================================================
//function : error
//purpose  :
//=======================================================================
SALOME_EvalExprError SALOME_EvalParser::error() const
{
  return myError;
}
//=======================================================================
//function : setError
//purpose  :
//=======================================================================

void SALOME_EvalParser::setError(SALOME_EvalExprError err)
{
  myError = err;
}

//=======================================================================
//function : isMonoParam
//purpose  :
//=======================================================================
bool SALOME_EvalParser::isMonoParam() const
{
  return myError == EvalExpr_OK && myPostfix.size()==1 && myPostfix.begin()->myType == Param;
}

//=======================================================================
//function : substitute
//purpose  :
//=======================================================================
void SALOME_EvalParser::substitute( const SALOME_String& theParamName, 
                                    SALOME_EvalParser* theNewExpr )
{
  Postfix::iterator it = myPostfix.begin(), last = myPostfix.end();
  for( ; it!=last; it++ )
  {
    //printf( "ELEM: %s\n", it->myValue.toString().c_str() );
    if( it->myType == Param && it->myValue.toString() == theParamName )
    {
      Postfix::iterator removed = it; it++;
      myPostfix.erase( removed );
      //printf( "REMOVE:\n" );
      Postfix::iterator nit = theNewExpr->myPostfix.begin(), nlast = theNewExpr->myPostfix.end();
      for( ; nit!=nlast; nit++ )
      {
        myPostfix.insert( it, *nit );
        //printf( "INSERT: %s\n", nit->myValue.toString().c_str() );
      }
      it--;
    }
  }
}


//=======================================================================
//class    : RebultExprItem
//purpose  : Reconstruction of expression by postfix form
//=======================================================================
class RebultExprItem
{
public:
  RebultExprItem( const SALOME_EvalParser* theParser, const SALOME_String& theExpr = "" )
    : myExpr( theExpr ), myParser( theParser )
  {
  }

  int priority() const
  {
    return myOp.length() > 0 ? myParser->priority( myOp, myOpBin ) : 100000;
  }

  RebultExprItem concat( const SALOME_String& theOp, const RebultExprItem& theItem, bool theIsBin ) const
  {
    int pleft = priority(), pright = theItem.priority(),
      pOp = myParser->priority( theOp, theIsBin );

    //printf( "concat: '%s' (%i) and '%s' (%i) with op = %s (%i)\n",
    //        myExpr.c_str(), pleft, theItem.myExpr.c_str(), pright,
    //        theOp.c_str(), pOp );

    SALOME_String aNewExpr = expression( pleft<pOp );
    aNewExpr += theOp;
    aNewExpr += theItem.expression( pright<pOp );
    RebultExprItem aRes( myParser );
    aRes.myExpr = aNewExpr;
    aRes.myOp = theOp;
    aRes.myOpBin = theIsBin;
    return aRes;
  }

  SALOME_String expression( bool theIsAddBrackets ) const
  {
    if( theIsAddBrackets )
      return "(" + myExpr + ")";
    else
      return myExpr;
  }

private:
  const SALOME_EvalParser* myParser;
  SALOME_String myExpr, myOp;
  bool myOpBin;
};

//=======================================================================
//function : rebuildExpression
//purpose  :
//=======================================================================
SALOME_String SALOME_EvalParser::rebuildExpression() const
{
  std::stack<RebultExprItem> aStack;

  Postfix::const_iterator it = myPostfix.begin(), last = myPostfix.end();
  RebultExprItem anItem1( this ), anItem2( this );
  for( ; it!=last; it++ )
  {
    SALOME_String aValue = it->myValue.toString();
    switch( it->myType )
    {
    case Value:
    case Param:
      aStack.push( RebultExprItem( this, aValue ) );
      break;

    case Pre:
      anItem1 = aStack.top(); aStack.pop();
      anItem2 = RebultExprItem( this ).concat( aValue, anItem1, false );
      aStack.push( anItem2 );
      break;

    case Post:
      anItem1 = aStack.top(); aStack.pop();
      anItem2 = anItem1.concat( aValue, RebultExprItem( this ), false );
      aStack.push( anItem2 );
      break;

    case Binary:
      anItem2 = aStack.top(); aStack.pop();
      anItem1 = aStack.top(); aStack.pop();
      anItem2 = anItem1.concat( aValue, anItem2, true );
      aStack.push( anItem2 );
      break;
    }
  }

  return aStack.top().expression( false );
}

//=======================================================================
//function : dump
//purpose  :
//=======================================================================
SALOME_String SALOME_EvalParser::dump() const
{
  return dump( myPostfix );
}
//=======================================================================
//function : dump
//purpose  :
//=======================================================================
SALOME_String SALOME_EvalParser::dump( const Postfix& post ) const
{
  SALOME_String res;
  //
  if ( !checkOperations() ) {
    return res;
  }
  //
  Postfix::const_iterator anIt = post.begin();
  for (; anIt != post.end(); anIt++ )  {
    if ((*anIt).myType == Value &&
        (*anIt).myValue.type() == SALOME_EvalVariant_String ) {
      res += "'" + (*anIt).myValue.toString() + "'";
    }
    else{
      res += (*anIt).myValue.toString();
    }
    if ( (*anIt).myType == Pre ){
      res += "(pre)";
    }
    else if ( (*anIt).myType == Post ){
      res += "(post)";
    }
    else if ( (*anIt).myType == Binary ){
      res += "(bin)";
    }

    res += "_";
  }

  return res;
}
//=======================================================================
//function : parameters
//purpose  :
//=======================================================================
SALOME_StringList SALOME_EvalParser::parameters() const
{
  SALOME_StringList lst;
  Postfix::const_iterator anIt = myPostfix.begin();
  for (; anIt != myPostfix.end(); anIt++ )  {
    if ( (*anIt).myType == Param )    {
      SALOME_String name = (*anIt).myValue.toString();
      if ( !contains(lst, name ) ) {
        lst.push_back( name );
      }
    }
  }
  return lst;
}
//=======================================================================
//function : clearParameters
//purpose  :
//=======================================================================
void SALOME_EvalParser::clearParameters()
{
  myParams.clear();
}
//=======================================================================
//function : toString
//purpose  :
//=======================================================================
SALOME_String SALOME_EvalParser::toString( const SALOME_ListOfEvalVariant& theList )
{
  SALOME_String res = "set : [ ";
  SALOME_ListOfEvalVariant::const_iterator aIt = theList.begin();
  for ( ; aIt != theList.end(); aIt++ )
    res += (*aIt).toString() + " ";
  res += "]";
  return res;
}
//=======================================================================
//function : operationList
//purpose  :
//=======================================================================
void SALOME_EvalParser::operationList( SALOME_StringList& theList ) const
{
  SALOME_ListOfEvalSet::const_iterator aIt = mySets.begin();
  for (; aIt != mySets.end(); ++aIt )  {
    SALOME_StringList custom;
    SALOME_EvalSet* aSet = *aIt;
    aSet->operationList( custom );
    SALOME_StringList::const_iterator sIt = custom.begin();
    for ( ; sIt != custom.end(); ++sIt )    {
      if ( !contains(theList, *sIt ) ) {
        theList.push_back( *sIt );
      }
    }
  }
}
//=======================================================================
//function : operationList
//purpose  :
//=======================================================================
void SALOME_EvalParser::bracketsList( SALOME_StringList& theList, bool open ) const
{
  SALOME_ListOfEvalSet::const_iterator it = mySets.begin();
  for (; it != mySets.end(); ++it ) {
    SALOME_StringList custom;
    SALOME_EvalSet* aSet = *it;
    aSet->bracketsList( custom, open );
    SALOME_StringList::const_iterator sIt = custom.begin();
    for (; sIt != custom.end(); ++sIt )    {
      if ( !contains(theList, *sIt ) )
        theList.push_back( *sIt );
    }
  }
}
//=======================================================================
//function : createValue
//purpose  :
//=======================================================================
bool SALOME_EvalParser::createValue( const SALOME_String& str, SALOME_EvalVariant& val ) const
{
  bool ok = false;
  //
  SALOME_ListOfEvalSet::const_iterator it = mySets.begin();
  for (; it != mySets.end() && !ok; ++it ) {
    ok = (*it)->createValue( str, val );
  }
  return ok;
}
//=======================================================================
//function : priority
//purpose  :
//=======================================================================
int SALOME_EvalParser::priority( const SALOME_String& op, bool isBin ) const
{
  int i = 0;
  int priority = 0;
  SALOME_ListOfEvalSet::const_iterator it = mySets.begin();
  for (; it != mySets.end() && priority <= 0; ++it, i++ ){
    priority = (*it)->priority( op, isBin );
  }
  return priority > 0 ? priority + i * 50 : 0;
}
//=======================================================================
//function : isValid
//purpose  :
//=======================================================================
SALOME_EvalExprError SALOME_EvalParser::isValid( const SALOME_String& op, const SALOME_EvalVariantType t1,
                                                 const SALOME_EvalVariantType t2 ) const
{
  SALOME_EvalExprError err = EvalExpr_OK;
  //
  SALOME_ListOfEvalSet::const_iterator it = mySets.begin();
  for (; it != mySets.end(); ++it )  {
    SALOME_EvalSet *pSet=*it;
    err = pSet->isValid( op, t1, t2 );
    if ( err == EvalExpr_OK ){
      break;
    }
  }
  return err;
}
//=======================================================================
//function : calculation
//purpose  :
//=======================================================================
SALOME_EvalExprError SALOME_EvalParser::calculation( const SALOME_String& op, SALOME_EvalVariant& v1, SALOME_EvalVariant& v2 ) const
{
  SALOME_EvalVariant nv1, nv2;
  SALOME_EvalSet *pSet;
  SALOME_EvalVariantType aType1, aType2;
  SALOME_EvalExprError aErr;
  //
  aType1=v1.type();
  aType2=v2.type();
  //
  SALOME_ListOfEvalSet::const_iterator aIt = mySets.begin();
  for (; aIt != mySets.end(); ++aIt )  {
    pSet=*aIt;
    nv1 = v1;
    nv2 = v2;
    aErr=pSet->isValid( op, aType1, aType2);
    if ( aErr == EvalExpr_OK ) {
      aErr = pSet->calculate( op, nv1, nv2 );
      if ( aErr == EvalExpr_OK || aErr == EvalExpr_InvalidResult )  {
        v1 = nv1;
        v2 = nv2;
        return aErr;
      }
    }
  }
  //
  return EvalExpr_InvalidOperation;
}
//=======================================================================
//function : checkOperations
//purpose  :
//=======================================================================
bool SALOME_EvalParser::checkOperations() const
{
  if ( mySets.size() )
    return true;

  SALOME_EvalParser* that = (SALOME_EvalParser*)this;
  that->setError( EvalExpr_OperationsNull );
  return false;
}
//=======================================================================
//function : append
//purpose  :
//=======================================================================
void SALOME_EvalParser::append(Postfix& aL,
                           const Postfix& aL1)
{
  Postfix::const_iterator aIt=aL1.begin();
  for(; aIt!=aL1.end(); ++aIt) {
    aL.push_back(*aIt);
  }
}
//=======================================================================
//function : at
//purpose  :
//=======================================================================
const SALOME_EvalParser::PostfixItem& SALOME_EvalParser::at(const Postfix& aL,
                                                    const int aIndex)
{
  int i;
  Postfix::const_iterator aIt=aL.begin();
  for(i=0; aIt!=aL.end(); ++aIt, ++i) {
    if (i==aIndex) {
      break;
    }
  }
  return *aIt;
}
//=======================================================================
//function : insert
//purpose  :
//=======================================================================
void SALOME_EvalParser::insert(Postfix& aL,
                            const int aIndex,
                            PostfixItem& pS)
{
  int i;
  //
  if (!aIndex) {
    aL.push_front(pS);
  }
  else if (aIndex==aL.size()) {
    aL.push_back(pS);
  }
  else {
    Postfix::iterator aIt=aL.begin();
    for(i=0; aIt!=aL.end(); ++aIt, ++i) {
      if (i==aIndex) {
        aL.insert( aIt, pS );
        break;
      }
    }
  }
}
//=======================================================================
//function : indexOf
//purpose  :
//=======================================================================
int indexOf( const SALOME_StringList& aLS, const SALOME_String& aS )
{
  int i, iRet;
  SALOME_StringList::const_iterator aIt;
  //
  iRet=-1;
  //
  aIt=aLS.begin();
  for (i=0; aIt!=aLS.end(); ++aIt, ++i)
  {
    const SALOME_String aSx=*aIt;
    if (aSx==aS) {
      iRet=i;
      break;
    }
  }
  return iRet;
}
//=======================================================================
//function : contains
//purpose  :
//=======================================================================
bool contains( const SALOME_StringList& aLS, const SALOME_String& aS )
{
  bool bRet;
  //
  bRet=false;
  SALOME_StringList::const_iterator aIt;
  //
  aIt=aLS.begin();
  for (; aIt!=aLS.end(); ++aIt) {
    const SALOME_String aSx=*aIt;
    if (aSx==aS) {
      bRet=!bRet;
      break;
    }
  }
  return bRet;
}


//=======================================================================
//function : isSpace
//purpose  :
//=======================================================================
bool isSpace(const char aC)
{
  bool bRet;
  char aWhat[]={
    '\t', '\n', '\v', '\f', '\r', ' '
  };
  int i, aNb;
  //
  bRet=false;
  aNb=sizeof(aWhat)/sizeof(aWhat[0]);
  for (i=0; i<aNb; ++i) {
    bRet=(aC==aWhat[i]);
    if (bRet) {
      break;
    }
  }
  return bRet;
}
//=======================================================================
//function : trimmed
//purpose  :
//=======================================================================
SALOME_String trimmed(const SALOME_String& str)
{
  char aWhat[]={
    '\t', '\n', '\v', '\f', '\r', ' '
  };
  int i, aNb;
  SALOME_String aRet;
  //
  aRet=str;
  aNb=sizeof(aWhat)/sizeof(aWhat[0]);
  for (i=0; i<aNb; ++i) {
    aRet=trimmed(aRet, aWhat[i]);
  }
  return aRet;
}
//=======================================================================
//function : trimmed
//purpose  :
//=======================================================================
SALOME_String trimmed(const SALOME_String& str, const char aWhat)
{
  char aX[2];
  size_t mylength, i;
  SALOME_String aRet;
  //
  const char* mystring=str.c_str();
  if(!mystring) {
    return aRet;
  }
  //
  mylength=strlen(mystring);
  if (!mylength) {
    return aRet;
  }
  //
  aX[1]=0;
  for (i=0; i<mylength; ++i) {
    char aC=mystring[i];
    if (aC != aWhat) {
      aX[0]=aC;
      aRet.append(aX);
    }
  }
  return aRet;
}

