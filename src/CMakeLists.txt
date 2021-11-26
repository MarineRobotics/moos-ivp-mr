##############################################################################
# FILE:        moos-ivp-mr/src/CMakeLists.txt
# DATE:        2021/09/02
# DESCRIPTION: CMakeLists.txt file for the moos-ivp-mr source directory
##############################################################################


#============================================================================
# Add the libraries in the current directory to the include path
#============================================================================
FILE(GLOB LOCAL_LIBRARY_DIRS ./lib_*)
INCLUDE_DIRECTORIES(${LOCAL_LIBRARY_DIRS})
#============================================================================
# List the subdirectories to build...
#============================================================================
ADD_SUBDIRECTORY(lib_behaviors-test)
ADD_SUBDIRECTORY(lib_sock_utils)
ADD_SUBDIRECTORY(iSailBoat)

##############################################################################
#                           END of CMakeLists.txt
##############################################################################

