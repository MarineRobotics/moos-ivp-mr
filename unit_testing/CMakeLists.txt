cmake_minimum_required(VERSION 3.5)
project(unit_testing)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
SET(CMAKE_INCLUDE_CURRENT_DIR ON)


# TODO: CODE SECTION UNDERNEATH THIS COMMENT COMES FROM DIFFERENT CMAKE, NEEDS TO BE ADAPTED TO THIS PROJECT
# THIS MEANS IT NEEDS TO AUTOMATICALLY INCLUDE ALL THE HEADERS FROM THE MOOS TREE SRC FOLDER AND ONLY BUILD main-test.cpp IF NECESSARY


file(GLOB_RECURSE Test_SOURCES "./*.cpp")
#file(GLOB_RECURSE LOCAL_HEADER_DIRS "../src/*.hpp")

ADD_EXECUTABLE(unit_test main-test.cpp factorialTest/factorial-test.cpp ../src/factorialtest/factorial.cpp)

INCLUDE_DIRECTORIES(../src/factorialtest)
#GET_PROPERTY(DIRECTORY )

###################################################################################################################################
# link_directories(/usr/local/lib)
# set(SOURCE_FILES main.cpp i2c_pwm.cpp i2c_pwm.h adafruit_motorhat.cpp adafruit_motorhat.h )
# add_executable(motorhat_cpp_conversion ${SOURCE_FILES})

# target_link_libraries(motorhat_cpp_conversion wiringPi)
# if(UNIX AND NOT APPLE)
#     target_link_libraries(motorhat_cpp_conversion rt)
# endif()

#####################################################################################################################################
