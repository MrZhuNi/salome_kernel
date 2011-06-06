# Copyright (C) 2007-2011  CEA/DEN, EDF R&D, OPEN CASCADE
#
# Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
# CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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

# ------

MESSAGE(STATUS "Check for cppunit ...")

# ------

IF(CPPUNIT_IS_MANDATORY STREQUAL 0)
  SET(CPPUNIT_IS_MANDATORY 0)
  SET(CPPUNIT_IS_OPTIONAL 1)
ENDIF(CPPUNIT_IS_MANDATORY STREQUAL 0)
IF(CPPUNIT_IS_OPTIONAL STREQUAL 0)
  SET(CPPUNIT_IS_MANDATORY 1)
  SET(CPPUNIT_IS_OPTIONAL 0)
ENDIF(CPPUNIT_IS_OPTIONAL STREQUAL 0)
IF(NOT CPPUNIT_IS_MANDATORY AND NOT CPPUNIT_IS_OPTIONAL)
  SET(CPPUNIT_IS_MANDATORY 0)
  SET(CPPUNIT_IS_OPTIONAL 1)
ENDIF(NOT CPPUNIT_IS_MANDATORY AND NOT CPPUNIT_IS_OPTIONAL)

# ------

SET(CPPUNIT_STATUS 1)
IF(WITHOUT_CPPUNIT OR WITH_CPPUNIT STREQUAL 0)
  SET(CPPUNIT_STATUS 0)
  MESSAGE(STATUS "cppunit disabled from command line.")
ENDIF(WITHOUT_CPPUNIT OR WITH_CPPUNIT STREQUAL 0)

# ------

IF(CPPUNIT_STATUS)
  IF(WITH_CPPUNIT)
    SET(CPPUNIT_ROOT_USER ${WITH_CPPUNIT})
  ELSE(WITH_CPPUNIT)
    SET(CPPUNIT_ROOT_USER $ENV{CPPUNIT_ROOT})
  ENDIF(WITH_CPPUNIT)
ENDIF(CPPUNIT_STATUS)

# -----

IF(CPPUNIT_STATUS)
  IF(CPPUNIT_ROOT_USER)
    SET(CPPUNIT_FIND_PATHS_OPTION NO_DEFAULT_PATH)
  ELSE(CPPUNIT_ROOT_USER)
    SET(CPPUNIT_FIND_PATHS_OPTION)
  ENDIF(CPPUNIT_ROOT_USER)
ENDIF(CPPUNIT_STATUS)

# -----

IF(CPPUNIT_STATUS)
  IF(CPPUNIT_ROOT_USER)
    SET(CPPUNIT_INCLUDE_PATHS ${CPPUNIT_ROOT_USER} ${CPPUNIT_ROOT_USER}/include)
  ELSE(CPPUNIT_ROOT_USER)
    SET(CPPUNIT_INCLUDE_PATHS)
  ENDIF(CPPUNIT_ROOT_USER)
  SET(CPPUNIT_INCLUDE_TO_FIND cppunit/extensions/HelperMacros.h)
  FIND_PATH(CPPUNIT_INCLUDE_DIR ${CPPUNIT_INCLUDE_TO_FIND} PATHS ${CPPUNIT_INCLUDE_PATHS} ${CPPUNIT_FIND_PATHS_OPTION})
  IF(CPPUNIT_INCLUDE_DIR)
    IF(CPPUNIT_ROOT_USER)
      SET(CPPUNIT_INCLUDES -I${CPPUNIT_INCLUDE_DIR})
    ENDIF(CPPUNIT_ROOT_USER)
    MESSAGE(STATUS "${CPPUNIT_INCLUDE_TO_FIND} found in ${CPPUNIT_INCLUDE_DIR}")
  ELSE(CPPUNIT_INCLUDE_DIR)
    SET(CPPUNIT_STATUS 0)
    IF(CPPUNIT_ROOT_USER)
      MESSAGE(STATUS "${CPPUNIT_INCLUDE_TO_FIND} not found in ${CPPUNIT_INCLUDE_PATHS}, check your CPPUNIT installation.")
    ELSE(CPPUNIT_ROOT_USER)
      MESSAGE(STATUS "${CPPUNIT_INCLUDE_TO_FIND} not found on system, try to use WITH_CPPUNIT option or CPPUNIT_ROOT environment variable.")
    ENDIF(CPPUNIT_ROOT_USER)
  ENDIF(CPPUNIT_INCLUDE_DIR)
ENDIF(CPPUNIT_STATUS)

# ----

IF(CPPUNIT_STATUS)
  IF(CPPUNIT_ROOT_USER)
    SET(CPPUNIT_LIB_PATHS ${CPPUNIT_ROOT_USER}/lib)
  ELSE(CPPUNIT_ROOT_USER)
    SET(CPPUNIT_LIB_PATHS)
  ENDIF(CPPUNIT_ROOT_USER)
ENDIF(CPPUNIT_STATUS)

IF(CPPUNIT_STATUS)
  IF(WINDOWS)
    FIND_LIBRARY(CPPUNIT_LIB cppunitd_dll PATHS ${CPPUNIT_LIB_PATHS} ${CPPUNIT_FIND_PATHS_OPTION})
  ELSE(WINDOWS)
    FIND_LIBRARY(CPPUNIT_LIB cppunit PATHS ${CPPUNIT_LIB_PATHS} ${CPPUNIT_FIND_PATHS_OPTION})
  ENDIF(WINDOWS)
  SET(CPPUNIT_LIBS)
  IF(CPPUNIT_LIB)
    SET(CPPUNIT_LIBS ${CPPUNIT_LIBS} ${CPPUNIT_LIB})
    MESSAGE(STATUS "cppunit lib found: ${CPPUNIT_LIB}")
  ELSE(CPPUNIT_LIB)
    SET(CPPUNIT_STATUS 0)
    IF(CPPUNIT_ROOT_USER)
      MESSAGE(STATUS "cppunit lib not found in ${CPPUNIT_LIB_PATHS}, check your CPPUNIT installation.")
    ELSE(CPPUNIT_ROOT_USER)
      MESSAGE(STATUS "cppunit lib not found on system, try to use WITH_CPPUNIT option or CPPUNIT_ROOT environment variable.")
    ENDIF(CPPUNIT_ROOT_USER)
  ENDIF(CPPUNIT_LIB)
ENDIF(CPPUNIT_STATUS)

# ----

IF(CPPUNIT_STATUS)
  SET(CPPUNIT_IS_OK 1)
  IF(WINDOWS)
    SET(CPPUNIT_INCLUDES ${CPPUNIT_INCLUDES} -DCPPUNIT_DLL)
  ENDIF(WINDOWS)
ELSE(CPPUNIT_STATUS)
  IF(CPPUNIT_IS_MANDATORY)
    MESSAGE(FATAL_ERROR "cppunit not found ... mandatory ... abort")
  ELSE(CPPUNIT_IS_MANDATORY)
    MESSAGE(STATUS "cppunit not found ... optional ... disabled")
  ENDIF(CPPUNIT_IS_MANDATORY)
ENDIF(CPPUNIT_STATUS)

# ------
