//  SALOME ModuleCatalog : implementation of ModuleCatalog server which parsers xml description of modules
//
//  Copyright (C) 2003  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
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
//  See http://www.opencascade.org/SALOME/ or email : webmaster.salome@opencascade.org 
//
//
//
//  File   : SALOME_ModuleCatalog_Handler.cxx
//  Author : Estelle Deville
//  Module : SALOME
//  $Header$

#define WRITE_CATA_COMPONENT
#include "SALOME_ModuleCatalog_Handler.hxx"
using namespace std;

//----------------------------------------------------------------------
// Function : SALOME_ModuleCatalog_Handler
// Purpose  : Constructor
//----------------------------------------------------------------------
SALOME_ModuleCatalog_Handler::SALOME_ModuleCatalog_Handler()
{
  MESSAGE("SALOME_ModuleCatalog_Handler creation")
  // XML Tags initialisation
  // Used in the function endElement
  test_path_prefix_name = "path-prefix-name";
  test_computer_name = "computer-name" ;
  test_path_prefix = "path-prefix" ;

  test_component_name = "component-name";
  test_component_username = "component-username";
  test_component_type = "component-type" ;
  test_component_multistudy="component-multistudy";
  test_component_icone="component-icone" ;

  test_interface_name = "component-interface-name" ;
  
  test_service_name = "service-name";
  test_defaultservice = "service-by-default";

  test_inParameter_type="inParameter-type";
  test_inParameter_name="inParameter-name";
  test_inParameter="inParameter";
  test_inParameter_list="inParameter-list";

  test_outParameter_type="outParameter-type";
  test_outParameter_name="outParameter-name";
  test_outParameter="outParameter";
  test_outParameter_list="outParameter-list";

  test_inDataStreamParameter_type="inDataStreamParameter-type";
  test_inDataStreamParameter_dependency="inDataStreamParameter-dependency";
  test_inDataStreamParameter_name="inDataStreamParameter-name";
  test_inDataStreamParameter="inDataStreamParameter";
  test_inDataStreamParameter_list="inDataStreamParameter-list";

  test_outDataStreamParameter_type="outDataStreamParameter-dependency";
  test_outDataStreamParameter_type="outDataStreamParameter-type";
  test_outDataStreamParameter_name="outDataStreamParameter-name";
  test_outDataStreamParameter="outDataStreamParameter";
  test_outDataStreamParameter_list="outDataStreamParameter-list";

  test_service= "component-service";
  test_service_list="component-service-list";
  test_interface_list="component-interface-list";

  test_constraint="constraint";

  test_component="component";
}

//----------------------------------------------------------------------
// Function : ~SALOME_ModuleCatalog_Handler
// Purpose  : Destructor
//----------------------------------------------------------------------
SALOME_ModuleCatalog_Handler::~SALOME_ModuleCatalog_Handler()
{
  MESSAGE("SALOME_ModuleCatalog_Handler destruction")
}

//----------------------------------------------------------------------
// Function : startDocument
// Purpose  : overload handler function
//----------------------------------------------------------------------
bool SALOME_ModuleCatalog_Handler::startDocument()
{
  MESSAGE("Begin parse document")
  // Empty the private elements
  _pathList.resize(0);
  _pathPrefix.listOfComputer.resize(0);
  _serviceList.resize(0);
  _interfaceList.resize(0);
  _moduleList.resize(0);
  _inDataStreamParamList.resize(0);
  _outDataStreamParamList.resize(0);
  _inParamList.resize(0);
  _outParamList.resize(0);
  return true;
}

//----------------------------------------------------------------------
// Function : startElement
// Purpose  : overload handler function
//----------------------------------------------------------------------
bool SALOME_ModuleCatalog_Handler::startElement(const QString&, 
						const QString &,
						const QString& qName, 
						const QXmlAttributes& atts)
{
  SCRUTE(qName);
  return true;
} 

//----------------------------------------------------------------------
// Function : endElement
// Purpose  : overload handler function
//----------------------------------------------------------------------
bool SALOME_ModuleCatalog_Handler::endElement(const QString&, 
					      const QString &,
					      const QString& qName)
{
  // Path prefix

  // tag test_path_prefix_name
  if((qName.compare(QString(test_path_prefix_name))==0))
    _pathPrefix.path = content;
  // tag test_computer_name
  if((qName.compare(QString(test_computer_name))==0)) 
    _pathPrefix.listOfComputer.push_back(content);
   
  // tag test_path_prefix
  if((qName.compare(QString(test_path_prefix))==0))
    {
      _pathList.push_back(_pathPrefix);
      _pathPrefix.listOfComputer.resize(0);
    }

  // Component identification

  // tag test_component_name
  if((qName.compare(QString(test_component_name))==0)) 
    _aModule.parserComponentName = content ;
  // tag test_component_username
  if((qName.compare(QString(test_component_username))==0)) 
    _aModule.parserComponentUsername = content ;
  // tag test_component_type
   if((qName.compare(QString(test_component_type))==0)) 
     {
       if ((content.compare("MESH") == 0) ||
	   (content.compare("Mesh") == 0) ||
	   (content.compare("mesh") == 0))
	 _aModule.parserComponentType = MESH ;
       else if((content.compare("MED") == 0) ||
	       (content.compare("Med") == 0) ||
	       (content.compare("med") == 0))
	 _aModule.parserComponentType = Med ;
       else if((content.compare("GEOM") == 0) ||
	       (content.compare("Geom") == 0) ||
	       (content.compare("geom") == 0))
	 _aModule.parserComponentType = GEOM ;
       else if((content.compare("SOLVER") == 0) ||
	       (content.compare("Solver") == 0) ||
	       (content.compare("solver") == 0))
	 _aModule.parserComponentType = SOLVER ;
       else if((content.compare("SUPERV") == 0) ||
	       (content.compare("Superv") == 0) ||
	       (content.compare("Supervision") == 0) ||
	       (content.compare("superv") == 0))
	 _aModule.parserComponentType = SUPERV ;
       else if((content.compare("DATA") == 0) ||
	       (content.compare("Data") == 0) ||
	       (content.compare("data") == 0))
	 _aModule.parserComponentType = DATA ; 
       else if((content.compare("VISU") == 0) ||
	       (content.compare("Visu") == 0) ||
	       (content.compare("visu") == 0))
	 _aModule.parserComponentType = VISU ; 
       else if((content.compare("OTHER") == 0) ||
	       (content.compare("Other") == 0) ||
	       (content.compare("other") == 0))                
	 _aModule.parserComponentType = OTHER ;
       else
	 // If it'not in all theses cases, the type is affected to OTHER
	 _aModule.parserComponentType = OTHER ;
     }

   // tag test_component_multistudy
  if((qName.compare(QString(test_component_multistudy))==0)) 
    _aModule.parserComponentMultistudy = atoi(content.c_str()) ;

  // tag test_component_icone
  if((qName.compare(QString(test_component_icone))==0)) 
    _aModule.parserComponentIcon = content ;

   // interface identification

   // tag test_interface_name
   if((qName.compare(QString(test_interface_name))==0)) 
       _aInterface.parserInterfaceName = content ;

   // Service identification

   // tag test_service_name
   if((qName.compare(QString(test_service_name))==0))
     _aService.parserServiceName = content ;
     
   //tag test_defaultservice
   if((qName.compare(QString(test_defaultservice))==0))
     _aService.parserServiceByDefault = atoi(content.c_str()) ;

   // Parameter in

   // tag test_inParameter_type
   if((qName.compare(QString(test_inParameter_type))==0))
     _inParam.parserParamType = content ;
   //tag test_inParameter_name
   if((qName.compare(QString(test_inParameter_name))==0))
     _inParam.parserParamName = content ; 

   //tag test_inParameter
  if((qName.compare(QString(test_inParameter))==0))
    {
      _inParamList.push_back(_inParam) ; 

      // Empty temporary structures
      _inParam.parserParamType = "";
      _inParam.parserParamName = "";
    }
  
   //tag test_inParameter_list
   if((qName.compare(QString(test_inParameter_list))==0))
     {
       _aService.parserServiceInParameter = _inParamList;
       _inParamList.resize(0);
     }

   // Parameter out

   // tag test_outParameter_type
   if((qName.compare(QString(test_outParameter_type))==0))
     _outParam.parserParamType = content ;
   //tag test_outParameter_name
   if((qName.compare(QString(test_outParameter_name))==0))
     _outParam.parserParamName = content ; 
   
   //tag test_outParameter
  if((qName.compare(QString(test_outParameter))==0))
    {
      _outParamList.push_back(_outParam) ; 
     
      // Empty temporary structures
      _outParam.parserParamType = "";
      _outParam.parserParamName = "";
    }
   //tag test_outParameter_list
   if((qName.compare(QString(test_outParameter_list))==0)) 
     {
       _aService.parserServiceOutParameter=_outParamList;
       _outParamList.resize(0);
     }
     


   // DataStreamParameter in

   // tag test_inDataStreamParameter_type
   if((qName.compare(QString(test_inDataStreamParameter_type))==0))
     _inDataStreamParam.parserParamType = content ;
   //tag test_inDataStreamParameter_name
   if((qName.compare(QString(test_inDataStreamParameter_name))==0))
     _inDataStreamParam.parserParamName = content ; 

   //tag test_inDataStreamParameter
  if((qName.compare(QString(test_inDataStreamParameter))==0))
    {
      _inDataStreamParamList.push_back(_inDataStreamParam) ; 

      // Empty temporary structures
      _inDataStreamParam.parserParamType = "";
      _inDataStreamParam.parserParamName = "";
    }
  
   //tag test_inDataStreamParameter_list
   if((qName.compare(QString(test_inDataStreamParameter_list))==0))
     {
       _aService.parserServiceInDataStreamParameter = _inDataStreamParamList;
       _inDataStreamParamList.resize(0);
     }

   // DataStreamParameter out

   // tag test_outDataStreamParameter_type
   if((qName.compare(QString(test_outDataStreamParameter_type))==0))
     _outDataStreamParam.parserParamType = content ;
   //tag test_outDataStreamParameter_name
   if((qName.compare(QString(test_outDataStreamParameter_name))==0))
     _outDataStreamParam.parserParamName = content ; 
   
   //tag test_outDataStreamParameter
  if((qName.compare(QString(test_outDataStreamParameter))==0))
    {
      _outDataStreamParamList.push_back(_outDataStreamParam) ; 
     
      // Empty temporary structures
      _outDataStreamParam.parserParamType = "";
      _outDataStreamParam.parserParamName = "";
    }
   //tag test_outDataStreamParameter_list
   if((qName.compare(QString(test_outDataStreamParameter_list))==0)) 
     {
       _aService.parserServiceOutDataStreamParameter=_outDataStreamParamList;
       _outDataStreamParamList.resize(0);
     }
     

   // tag   test_service
   if((qName.compare(QString(test_service))==0))
     {
       _serviceList.push_back(_aService);

       // Empty temporary structures
       _aService.parserServiceName = "";
       _aService.parserServiceInParameter.resize(0);
       _aService.parserServiceOutParameter.resize(0);
       _aService.parserServiceInDataStreamParameter.resize(0);
       _aService.parserServiceOutDataStreamParameter.resize(0);
     }

   // tag   test_service_list
   if((qName.compare(QString(test_service_list))==0))
     {
       _aInterface.parserInterfaceServiceList = _serviceList ;

       // Empty temporary structures
       _serviceList.resize(0); 
       _interfaceList.push_back(_aInterface);  
       _aInterface.parserInterfaceName ="";    
       _aInterface.parserInterfaceServiceList.resize(0);

     }

   //tag test_interface_list
   if((qName.compare(QString(test_interface_list))==0))
     {
       _aModule.parserListInterface = _interfaceList ;
       SCRUTE(_aModule.parserListInterface[0].parserInterfaceServiceList[0].parserServiceInDataStreamParameter.size());
       _interfaceList.resize(0);
     }

   //tag test_constraint
   if((qName.compare(QString(test_constraint))==0))
     _aModule.parserConstraint = content ;

   // tag test_component
   if((qName.compare(QString(test_component))==0))
     {
       _moduleList.push_back(_aModule) ;
       
       // Empty temporary structures
       _aModule.parserComponentName = "";
       _aModule.parserConstraint = "";
       _aModule.parserComponentIcon="";       
       _aModule.parserListInterface.resize(0);
     }
    
  return true;
}
  
//----------------------------------------------------------------------
// Function : characters
// Purpose  : overload handler function
//----------------------------------------------------------------------
bool SALOME_ModuleCatalog_Handler::characters(const QString& chars)
{
  content = (const char*)chars ;
  return true;
}

//----------------------------------------------------------------------
// Function : endDocument
// Purpose  : overload handler function
//            Print all informations find in the catalog 
//            (only in DEBUG mode!!)
//----------------------------------------------------------------------  
bool SALOME_ModuleCatalog_Handler::endDocument()
{
  BEGIN_OF("endDocument");
  //_pathlist
  for (unsigned int ind = 0; ind < _pathList.size(); ind++)
    {
      MESSAGE("Path :"<<_pathList[ind].path)
      for (unsigned int i = 0; i < _pathList[ind].listOfComputer.size(); i++)
	  MESSAGE("Computer name :" << _pathList[ind].listOfComputer[i])
    }

   // _moduleList
  SCRUTE(_moduleList.size());
  for (unsigned int ind = 0; ind < _moduleList.size(); ind++)
    {
      DebugParserComponent( _moduleList[ind]);
    }

  MESSAGE("Document parsed");
  END_OF("endDocument");
  return true;
}
 
//----------------------------------------------------------------------
// Function : errorProtocol
// Purpose  : overload handler function
//----------------------------------------------------------------------  
QString SALOME_ModuleCatalog_Handler::errorProtocol()
{
  return errorProt ;
}


//----------------------------------------------------------------------
// Function : fatalError
// Purpose  : overload handler function
//----------------------------------------------------------------------  
bool SALOME_ModuleCatalog_Handler::fatalError(const QXmlParseException& exception)
{
    errorProt += QString( "fatal parsing error: %1 in line %2, column %3\n" )
    .arg( exception.message() )
    .arg( exception.lineNumber() )
    .arg( exception.columnNumber() );

  return QXmlDefaultHandler::fatalError( exception );
}
