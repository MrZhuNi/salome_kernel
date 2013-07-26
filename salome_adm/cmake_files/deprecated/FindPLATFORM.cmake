# Copyright (C) 2007-2013  CEA/DEN, EDF R&D, OPEN CASCADE
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

MARK_AS_ADVANCED(ISSUE)
FIND_FILE(ISSUE issue /etc)
IF(ISSUE)
  SET(WINDOWS 0)
ELSE()
  SET(WINDOWS 1)
ENDIF(ISSUE)

IF(WINDOWS)
  SET(MACHINE WINDOWS)
ELSE(WINDOWS)
  SET(MACHINE PCLINUX)
ENDIF(WINDOWS)

SET(CMAKE_INSTALL_PREFIX_ENV $ENV{CMAKE_INSTALL_PREFIX})
IF(CMAKE_INSTALL_PREFIX_ENV)
  SET(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX_ENV} CACHE PATH "installation prefix" FORCE)
ENDIF(CMAKE_INSTALL_PREFIX_ENV)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE $ENV{CMAKE_BUILD_TYPE})
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(WINDOWS)
ELSE(WINDOWS)
  SET(ADD_WERROR ON)
  SET(NAMES ACCEPT_SALOME_WARNINGS ACCEPT_${MODULE}_WARNINGS I_AM_A_TROLL_I_DO_NOT_FIX_${MODULE}_WARNINGS)
  FOREACH(name ${NAMES})
    SET(VAL $ENV{${name}})
    IF(X${VAL} STREQUAL X0)
      SET(ADD_WERROR ON)
    ENDIF(X${VAL} STREQUAL X0)
    IF(X${VAL} STREQUAL X1)
      SET(ADD_WERROR OFF)
    ENDIF(X${VAL} STREQUAL X1)
  ENDFOREACH(name ${NAMES})
  IF(ADD_WERROR)
    SET(CMAKE_C_FLAGS "-Werror")
    SET(CMAKE_CXX_FLAGS "-Werror")
  ENDIF(ADD_WERROR)
ENDIF(WINDOWS)

IF(CMAKE_BUILD_TYPE)
  IF(WINDOWS)
    MARK_AS_ADVANCED(CLEAR CMAKE_CONFIGURATION_TYPES)
    SET(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "compilation types" FORCE)
  ELSE(WINDOWS)
    IF(CMAKE_BUILD_TYPE STREQUAL Debug)
      SET(CMAKE_C_FLAGS_DEBUG "-g")
      SET(CMAKE_CXX_FLAGS_DEBUG "-g")
    ENDIF(CMAKE_BUILD_TYPE STREQUAL Debug)
    IF(CMAKE_BUILD_TYPE STREQUAL Release)
      SET(CMAKE_C_FLAGS_RELEASE "-O1 -DNDEBUG")
      SET(CMAKE_CXX_FLAGS_RELEASE "-O1 -DNDEBUG")
    ENDIF(CMAKE_BUILD_TYPE STREQUAL Release)
  ENDIF(WINDOWS)
ENDIF(CMAKE_BUILD_TYPE)

SET(PLATFORM_CPPFLAGS) # to be removed
SET(PLATFORM_LDFLAGS) # to be removed
SET(PLATFORM_LIBADD) # to be removed

SET(PLATFORM_LIBS)
SET(PLATFORM_DEFINITIONS)

IF(WINDOWS)
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} /W0) # to be removed
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_CRT_SECURE_NO_WARNINGS)  # To disable windows warnings for strcpy, fopen, ... # to be removed
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_SCL_SECURE_NO_WARNINGS)  # To disable windows warnings std::copy, std::transform, ... # to be removed
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -DWNT) # to be removed
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -DPPRO_NT) # For medfile # to be removed
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_USE_MATH_DEFINES) # At least for MEDMEM # to be removed
  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_WIN32_WINNT=0x0500) # Windows 2000 or later API is required # to be removed
  SET(PLATFORM_LIBADD ${PLATFORM_LIBADD} Ws2_32.lib) # to be removed
  SET(PLATFORM_LIBADD ${PLATFORM_LIBADD} Userenv.lib) # At least for GEOM suit # to be removed

  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} /W0")
  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -D_CRT_SECURE_NO_WARNINGS")  # To disable windows warnings for strcpy, fopen, ...
  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -D_SCL_SECURE_NO_WARNINGS")  # To disable windows warnings std::copy, std::transform, ...
  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -DWNT")
  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -DPPRO_NT") # For medfile 
  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -D_USE_MATH_DEFINES") # At least for MEDMEM
  SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -D_WIN32_WINNT=0x0500") # Windows 2000 or later API is required
  SET(PLATFORM_LIBS ${PLATFORM_LIBS} Ws2_32.lib)
  SET(PLATFORM_LIBS ${PLATFORM_LIBS} Userenv.lib) # At least for GEOM suit

  IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")          # if platform is Windows 64 bit 
    # To avoid runtime error during checking iterators  # to be removed
    SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_SECURE_SCL=0)
    SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_SECURE_SCL_THROWS=0) 
    SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_HAS_ITERATOR_DEBUGGING=0) 

    SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -D_SECURE_SCL=0 -D_SECURE_SCL_THROWS=0 -D_HAS_ITERATOR_DEBUGGING=0") # To avoid runtime error during checking iterators 
  ENDIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
ELSE(WINDOWS)
  # SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -Wparentheses)
  # SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -Wreturn-type)
  # SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -Wmissing-declarations)
  # SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -Wunused)
  # SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -Wall)
  SET(PLATFORM_LIBADD ${PLATFORM_LIBADD} -ldl) # to be removed
  SET(PLATFORM_LIBS ${PLATFORM_LIBS} -ldl)
ENDIF(WINDOWS)

SET(SIZE_OF_LONG ${CMAKE_SIZEOF_VOID_P})            # set sizeof(long) the same as size of pointers, because on all memory models (EXCLUDING WINDOWS 64 bit) it is equivalent values
IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8"  AND WINDOWS) # if it platform Windows 64 bit
  SET(SIZE_OF_LONG "4")                             # set sizeof(long) to 4 byte
ENDIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8"  AND WINDOWS)

SET(PLATFORM_CPPFLAGS "${PLATFORM_CPPFLAGS} -DSIZEOF_FORTRAN_INTEGER=4 -DSIZEOF_LONG=${SIZE_OF_LONG} -DSIZEOF_INT=4") # to be removed
SET(PLATFORM_DEFINITIONS "${PLATFORM_DEFINITIONS} -DSIZEOF_FORTRAN_INTEGER=4 -DSIZEOF_LONG=${SIZE_OF_LONG} -DSIZEOF_INT=4")

SET(COMPILATION_WITH_CMAKE ON)

#  SET(PLATFORM_CPPFLAGS)
#  #  #  SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -DWNT -D_CRT_SECURE_NO_WARNINGS)
#  #  SET(RM del)
#ELSE(WINDOWS)
#  #  SET(RM /bin/rm)
#ENDIF(WINDOWS)

### SET(PLATFORM_CPPFLAGS ${PLATFORM_CPPFLAGS} -D_DEBUG_)

##SET(RCP rcp)
##SET(RSH rsh)
##SET(RCP rcp)

## MESSAGE(FATAL_ERROR "ZZZZZZZ")
