# Copyright (C) 2012  CEA/DEN, EDF R&D, OPEN CASCADE
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

ADD_SUBDIRECTORY(Palm)
ADD_SUBDIRECTORY(Calcium)

INCLUDE_DIRECTORIES(
  ${BOOST_INCLUDE_DIRS}
  ${CMAKE_BINARY_DIR}/salome_adm
  ${CMAKE_SOURCE_DIR}/src/DSC/DSC_User
  ${CMAKE_BINARY_DIR}/src/DSC/DSC_User/Datastream/Palm
  ${CMAKE_BINARY_DIR}/src/DSC/DSC_User/Datastream/Calcium
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

# This local variable defines the list of dependant libraries common to all target in this package.
SET(COMMON_LIBS
  SalomePalm
  SalomeCalcium
  SalomeDSCSuperv
)

ADD_LIBRARY(SalomeDatastream SHARED fake.cc)
SET_TARGET_PROPERTIES(SalomeDatastream PROPERTIES COMPILE_FLAGS "${BOOST_DEFINITIONS}")
TARGET_LINK_LIBRARIES(SalomeDatastream ${COMMON_LIBS})
INSTALL(TARGETS SalomeDatastream DESTINATION ${KERNEL_salomelib_LIBS})

SET(COMMON_HEADERS_HXX
  AdjacentFunctor.hxx
  ConstTraits.hxx
  CorbaTypeManipulator.hxx
  CouplingPolicy.hxx
  DataIdFilter.hxx
  DisplayPair.hxx
  FindKeyPredicate.hxx
  GenericPort.hxx
  GenericProvidesPort.hxx
  GenericUsesPort.hxx
  IteratorTraits.hxx
)

INSTALL(FILES ${COMMON_HEADERS_HXX} DESTINATION ${KERNEL_salomeinclude_HEADERS})