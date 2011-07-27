#  Copyright (C) 2007-2011  CEA/DEN, EDF R&D, OPEN CASCADE
#
#  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
#  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
#  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

# ------

MESSAGE(STATUS "Check for libxml2 ...")

# ------

IF(LIBXML2_IS_MANDATORY STREQUAL 0)
  SET(LIBXML2_IS_MANDATORY 0)
  SET(LIBXML2_IS_OPTIONAL 1)
ENDIF(LIBXML2_IS_MANDATORY STREQUAL 0)
IF(LIBXML2_IS_OPTIONAL STREQUAL 0)
  SET(LIBXML2_IS_MANDATORY 1)
  SET(LIBXML2_IS_OPTIONAL 0)
ENDIF(LIBXML2_IS_OPTIONAL STREQUAL 0)
IF(NOT LIBXML2_IS_MANDATORY AND NOT LIBXML2_IS_OPTIONAL)
  SET(LIBXML2_IS_MANDATORY 1)
  SET(LIBXML2_IS_OPTIONAL 0)
ENDIF(NOT LIBXML2_IS_MANDATORY AND NOT LIBXML2_IS_OPTIONAL)

# ------

SET(LIBXML2_STATUS 1)
IF(WITHOUT_LIBXML2 OR WITH_LIBXML2 STREQUAL 0)
  SET(LIBXML2_STATUS 0)
  MESSAGE(STATUS "libxml2 disabled from command line.")
ENDIF(WITHOUT_LIBXML2 OR WITH_LIBXML2 STREQUAL 0)

# ------

IF(LIBXML2_STATUS)
  IF(WITH_LIBXML2)
    SET(LIBXML2_ROOT_USER ${WITH_LIBXML2})
  ELSE(WITH_LIBXML2)
    SET(LIBXML2_ROOT_USER $ENV{LIBXML2_ROOT})
  ENDIF(WITH_LIBXML2)
ENDIF(LIBXML2_STATUS)

# -----

IF(LIBXML2_STATUS)
  IF(LIBXML2_ROOT_USER)
    SET(LIBXML2_FIND_PATHS_OPTION NO_DEFAULT_PATH)
  ELSE(LIBXML2_ROOT_USER)
    SET(LIBXML2_FIND_PATHS_OPTION)
  ENDIF(LIBXML2_ROOT_USER)
ENDIF(LIBXML2_STATUS)

# -----

IF(LIBXML2_STATUS)
  IF(LIBXML2_ROOT_USER)
    SET(LIBXML2_INCLUDE_PATHS ${LIBXML2_ROOT_USER}/include)
  ELSE(LIBXML2_ROOT_USER)
    SET(LIBXML2_INCLUDE_PATHS /usr/include/libxml2)
  ENDIF(LIBXML2_ROOT_USER)
  SET(LIBXML2_INCLUDE_TO_FIND libxml/parser.h)
  FIND_PATH(LIBXML2_INCLUDE_DIR ${LIBXML2_INCLUDE_TO_FIND} PATHS ${LIBXML2_INCLUDE_PATHS} ${LIBXML2_FIND_PATHS_OPTION})
  IF(LIBXML2_INCLUDE_DIR)
    SET(LIBXML2_INCLUDES -I${LIBXML2_INCLUDE_DIR})
    MESSAGE(STATUS "${LIBXML2_INCLUDE_TO_FIND} found in ${LIBXML2_INCLUDE_DIR}")
  ELSE(LIBXML2_INCLUDE_DIR)
    SET(LIBXML2_STATUS 0)
    IF(LIBXML2_ROOT_USER)
      MESSAGE(STATUS "${LIBXML2_INCLUDE_TO_FIND} not found in ${LIBXML2_INCLUDE_PATHS}, check your LIBXML2 installation.")
    ELSE(LIBXML2_ROOT_USER)
      MESSAGE(STATUS "${LIBXML2_INCLUDE_TO_FIND} not found on system, try to use WITH_LIBXML2 option or LIBXML2_ROOT environment variable.")
    ENDIF(LIBXML2_ROOT_USER)
  ENDIF(LIBXML2_INCLUDE_DIR)
ENDIF(LIBXML2_STATUS)

# ----

IF(LIBXML2_STATUS)
  IF(LIBXML2_ROOT_USER)
    SET(LIBXML2_LIB_PATHS ${LIBXML2_ROOT_USER}/lib)
  ELSE(LIBXML2_ROOT_USER)
    SET(LIBXML2_LIB_PATHS)
  ENDIF(LIBXML2_ROOT_USER)
ENDIF(LIBXML2_STATUS)

IF(LIBXML2_STATUS)
  IF(WINDOWS)
    FIND_LIBRARY(LIBXML2_LIB libxml2 PATHS ${LIBXML2_LIB_PATHS} ${LIBXML2_FIND_PATHS_OPTION})
  ELSE(WINDOWS)
    FIND_LIBRARY(LIBXML2_LIB xml2 PATHS ${LIBXML2_LIB_PATHS} ${LIBXML2_FIND_PATHS_OPTION})
  ENDIF(WINDOWS)
  SET(LIBXML2_LIBS)
  IF(LIBXML2_LIB)
    SET(LIBXML2_LIBS ${LIBXML2_LIBS} ${LIBXML2_LIB})
    MESSAGE(STATUS "libxml2 lib found: ${LIBXML2_LIB}")
  ELSE(LIBXML2_LIB)
    SET(LIBXML2_STATUS 0)
    IF(LIBXML2_ROOT_USER)
      MESSAGE(STATUS "libxml2 lib not found in ${LIBXML2_LIB_PATHS}, check your LIBXML2 installation.")
    ELSE(LIBXML2_ROOT_USER)
      MESSAGE(STATUS "libxml2 lib not found on system, try to use WITH_LIBXML2 option or LIBXML2_ROOT environment variable.")
    ENDIF(LIBXML2_ROOT_USER)
  ENDIF(LIBXML2_LIB)
ENDIF(LIBXML2_STATUS)

# ----

IF(WINDOWS)
  SET(ICONV_ROOT $ENV{ICONV_ROOT})
  IF(ICONV_ROOT)
    FIND_PATH(ICONV_INCLUDE_DIR iconv.h ${ICONV_ROOT}/include)
    FIND_LIBRARY(ICONV_LIBS iconv ${ICONV_ROOT}/lib)
    SET(LIBXML2_INCLUDES ${LIBXML2_INCLUDES} -I${ICONV_INCLUDE_DIR})
    SET(LIBXML2_LIBS ${LIBXML2_LIBS} ${ICONV_LIBS})
  ENDIF(ICONV_ROOT)
ENDIF(WINDOWS)

# ------
IF(LIBXML2_STATUS)
  SET(LIBXML_INCLUDES ${LIBXML2_INCLUDES})
  SET(LIBXML_CPPFLAGS ${LIBXML2_INCLUDES})
  SET(LIBXML_LIBS ${LIBXML2_LIBS})
ELSE(LIBXML2_STATUS)
  IF(LIBXML2_IS_MANDATORY)
    MESSAGE(FATAL_ERROR "libxml2 not found ... mandatory ... abort")
  ELSE(LIBXML2_IS_MANDATORY)
    MESSAGE(STATUS "libxml2 not found ... optional ... disabled")
  ENDIF(LIBXML2_IS_MANDATORY)
ENDIF(LIBXML2_STATUS)

# ------
