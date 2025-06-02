# Comes form project edunetgames
# - Try to find RakNet
# Once done this will define
#
#  RakNet_FOUND - system has RakNet
#  RakNet_INCLUDES - the RakNet include directory
#  RakNet_LIBRARY - Link these to use RakNet

FIND_LIBRARY (RakNet_LIBRARY_RELEASE NAMES RakNetLibStatic
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib
    $ENV{RAKNET_ROOT}/lib
    ${RAKNET_ROOT}/lib
    )
  
FIND_LIBRARY (RakNet_LIBRARY_DEBUG NAMES RakNetLibStaticd
    PATHS
    ENV LD_LIBRARY_PATH
    ENV LIBRARY_PATH
    /usr/lib64
    /usr/lib
    /usr/local/lib64
    /usr/local/lib
    /opt/local/lib
    $ENV{RAKNET_ROOT}/lib
    ${RAKNET_ROOT}/lib
    )  
  
  

FIND_PATH (RakNet_INCLUDES raknet/RakPeer.h
    ENV CPATH
    /usr/include
    /usr/local/include
    /opt/local/include
    $ENV{RAKNET_ROOT}/include
    ${RAKNET_ROOT}/include
    )
 
MESSAGE(STATUS ${RakNet_INCLUDES})
MESSAGE(STATUS ${RakNet_LIBRARY_RELEASE})
MESSAGE(STATUS ${RakNet_LIBRARY_DEBUG})
 
IF(RakNet_INCLUDES AND RakNet_LIBRARY_RELEASE)
  SET(RakNet_FOUND TRUE)
ELSE(RakNet_INCLUDES AND RakNet_LIBRARY_RELEASE)
  IF(RakNet_INCLUDES AND RakNet_LIBRARY_DEBUG)
    SET(RakNet_FOUND TRUE)
    SET(RakNet_LIBRARY_RELEASE ${RakNet_LIBRARY_DEBUG})
  ENDIF(RakNet_INCLUDES AND RakNet_LIBRARY_DEBUG)
ENDIF(RakNet_INCLUDES AND RakNet_LIBRARY_RELEASE)

IF(RakNet_FOUND)
  SET(RakNet_INCLUDES ${RakNet_INCLUDES}/raknet)
  
  
  IF (CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
    IF(WIN32)
      IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY optimized ${RakNet_LIBRARY_RELEASE} debug ${RakNet_LIBRARY_DEBUG} ws2_32.lib)
        SET(RakNet_LIBRARY_RELEASE ${RakNet_LIBRARY_DEBUG})
      ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY optimized ${RakNet_LIBRARY_DEBUG} debug ${RakNet_LIBRARY_DEBUG} ws2_32.lib)
      ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
    ELSE(WIN32)
      IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY optimized ${RakNet_LIBRARY_RELEASE} debug ${RakNet_LIBRARY_DEBUG})
        SET(RakNet_LIBRARY_RELEASE ${RakNet_LIBRARY_DEBUG})
      ELSE(RakNet_LIBRARY_RELEASE AND "${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        SET(RakNet_LIBRARY optimized ${RakNet_LIBRARY_DEBUG} debug ${RakNet_LIBRARY_DEBUG})
      ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
    ENDIF(WIN32)
  ELSE(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
    # if there are no configuration types and CMAKE_BUILD_TYPE has no value
    # then just use the release libraries
    IF(WIN32)
      IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY ${RakNet_LIBRARY_DEBUG} ws2_32.lib)
        SET(RakNet_LIBRARY_RELEASE ${RakNet_LIBRARY_DEBUG})
      ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY ${RakNet_LIBRARY_RELEASE} ws2_32.lib)
      ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
    ELSE(WIN32)
      IF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY ${RakNet_LIBRARY_DEBUG})
        SET(RakNet_LIBRARY_RELEASE ${RakNet_LIBRARY_DEBUG})
      ELSE("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
        SET(RakNet_LIBRARY ${RakNet_LIBRARY_RELEASE})
      ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND NOT RakNet_LIBRARY_RELEASE)
    ENDIF(WIN32)
  ENDIF(CMAKE_CONFIGURATION_TYPES OR CMAKE_BUILD_TYPE)
  
  IF(NOT RakNet_FIND_QUIETLY)
    MESSAGE(STATUS "Found RakNet_LIBRARY_RELEASE: ${RakNet_LIBRARY_RELEASE}")
    MESSAGE(STATUS "Found RakNet_LIBRARY_DEBUG: ${RakNet_LIBRARY_DEBUG}")
    MESSAGE(STATUS "Found RakNet_INCLUDES: ${RakNet_INCLUDES}")
  ENDIF(NOT RakNet_FIND_QUIETLY)
ELSE(RakNet_FOUND)
  IF(RakNet_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find RakNet")
  ENDIF(RakNet_FIND_REQUIRED)
ENDIF(RakNet_FOUND)

