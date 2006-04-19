##
## Copyright Projet Lagadic / IRISA-INRIA Rennes, 2006
## www: http://www.irisa.fr/lagadic
##
## Author: Fabien Spindler email:Fabien.Spindler@irisa.fr
##
## Try to find libraries for Irisa's Afma 4 ddl cylindrical robot 
## Once run this will define: 
##
## AFMA4_FOUND
## AFMA4_INCLUDE_DIR
## AFMA4_LIBRARIES

IF(NOT UNIX)
  # MESSAGE("FindAFMA4.cmake: Afma4 only available for Unix.")
  SET(AFMA4_FOUND FALSE)
ELSE(NOT UNIX)
  
  FIND_PATH(AFMA4_INCLUDE_DIR main_Afma4.h
    $ENV{AFMA4_HOME}/include
    /udd/fspindle/robot/Afma4/current/include
    /local/soft/Afma4/current/include 
    )
  #MESSAGE("DBG AFMA4_INCLUDE_DIR=${AFMA4_INCLUDE_DIR}")  
  
  FIND_LIBRARY(ROBOTAFMA4_LIBRARY
    NAMES robotAfma4
    PATHS 
    $ENV{AFMA4_HOME}/lib
    /udd/fspindle/robot/Afma4/current/lib
    /local/soft/Afma4/current/lib
    )
  FIND_LIBRARY(TOOLSAFMA4_LIBRARY
    NAMES toolsAfma4
    PATHS 
    $ENV{AFMA4_HOME}/lib
    /udd/fspindle/robot/Afma4/current/lib
    /local/soft/Afma4/current/lib
    )
  FIND_LIBRARY(UPRIMAFMA4_LIBRARY
    NAMES uprimAfma4
    PATHS 
    $ENV{AFMA4_HOME}/lib
    /udd/fspindle/robot/Afma4/current/lib
    /local/soft/Afma4/current/lib
    )
  FIND_LIBRARY(BIT3AFMA4_LIBRARY
    NAMES bit3Afma4
    PATHS 
    $ENV{AFMA4_HOME}/lib
    /udd/fspindle/robot/Afma4/current/lib
    /local/soft/Afma4/current/lib
    )
  FIND_LIBRARY(SERVOLENSAFMA4_LIBRARY
    NAMES servolensAfma4
    PATHS 
    $ENV{AFMA4_HOME}/lib
    /udd/fspindle/robot/Afma4/current/lib
    /local/soft/Afma4/current/lib
    )
  #MESSAGE("DBG AFMA4_LIBRARY=${AFMA4_LIBRARY}")
  
  ## --------------------------------
  
  IF(ROBOTAFMA4_LIBRARY AND TOOLSAFMA4_LIBRARY AND UPRIMAFMA4_LIBRARY 
      AND BIT3AFMA4_LIBRARY AND SERVOLENSAFMA4_LIBRARY)
    SET(AFMA4_LIBRARIES ${ROBOTAFMA4_LIBRARY} ${TOOLSAFMA4_LIBRARY}
      ${UPRIMAFMA4_LIBRARY} ${BIT3AFMA4_LIBRARY} ${SERVOLENSAFMA4_LIBRARY})
  ELSE(ROBOTAFMA4_LIBRARY AND TOOLSAFMA4_LIBRARY AND UPRIMAFMA4_LIBRARY 
      AND BIT3AFMA4_LIBRARY AND SERVOLENSAFMA4_LIBRARY)
    MESSAGE(SEND_ERROR "Afma4 library not found.")
  ENDIF(ROBOTAFMA4_LIBRARY AND TOOLSAFMA4_LIBRARY AND UPRIMAFMA4_LIBRARY 
      AND BIT3AFMA4_LIBRARY AND SERVOLENSAFMA4_LIBRARY)
  
  IF(NOT AFMA4_INCLUDE_DIR)
    MESSAGE(SEND_ERROR "Afma4 include dir not found.")
  ENDIF(NOT AFMA4_INCLUDE_DIR)
  
  IF(AFMA4_LIBRARIES AND AFMA4_INCLUDE_DIR)
    SET(AFMA4_FOUND TRUE)
  ELSE(AFMA4_LIBRARIES AND AFMA4_INCLUDE_DIR)
    SET(AFMA4_FOUND FALSE)
  ENDIF(AFMA4_LIBRARIES AND AFMA4_INCLUDE_DIR)
  
  MARK_AS_ADVANCED(
    AFMA4_INCLUDE_DIR
    AFMA4_LIBRARIES
    ROBOTAFMA4_LIBRARY
    TOOLSAFMA4_LIBRARY
    UPRIMAFMA4_LIBRARY
    BIT3AFMA4_LIBRARY
    SERVOLENSAFMA4_LIBRARY
    )
ENDIF(NOT UNIX)
