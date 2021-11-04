#--------------------------------------------------------
# The CMakeLists.txt for:                       iSailBoat
# Author(s):                              Vincent Vandyck                             
#--------------------------------------------------------

SET(SRC
  SailBoat.cpp
  SailBoat_Info.cpp
  main.cpp
)

ADD_EXECUTABLE(iSailBoat ${SRC})

TARGET_LINK_LIBRARIES(iSailBoat
  ${MOOS_LIBRARIES}
  ${MOOSGeodesy_LIBRARIES}
  apputil
  sock_util
  mbutil
  m
  pthread)

