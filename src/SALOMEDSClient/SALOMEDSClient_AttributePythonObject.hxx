//  File   : SALOMEDSClient_AttributePythonObject.hxx
//  Author : Sergey RUIN
//  Module : SALOME

#ifndef SALOMEDSClient_AttributePythonObject_HeaderFile
#define SALOMEDSClient_AttributePythonObject_HeaderFile

#include "SALOMEDSClient_GenericAttribute.hxx" 

class SALOMEDSClient_AttributePythonObject: public virtual SALOMEDSClient_GenericAttribute
{
public:

  virtual void SetObject(const char* theSequence, bool IsScript) = 0;
  virtual char* GetObject() = 0;
  virtual bool IsScript() = 0;

};




#endif
